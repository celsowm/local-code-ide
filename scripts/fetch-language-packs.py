#!/usr/bin/env python3
"""Fetch and assemble pinned language-pack payloads for LocalCodeIDE."""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
import shutil
import subprocess
import time
import tarfile
import urllib.request
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Callable


@dataclass(frozen=True)
class DirectAsset:
    url: str
    archive: str  # zip | gz | raw | pses_zip
    matcher: Callable[[Path], bool]
    out_relpath: str


NODE_VERSION = "v25.8.1"
TSLS_VERSION = "5.1.3"
TYPESCRIPT_VERSION = "5.9.3"
PYRIGHT_VERSION = "1.1.408"
VSCODE_LANGSERVERS_EXTRACTED_VERSION = "4.10.0"
CMAKE_LANGUAGE_SERVER_VERSION = "0.1.11"

RUST_ANALYZER_VERSION = "2026-03-23"
CLANGD_VERSION = "21.1.8"
TAPLO_VERSION = "0.10.0"
MARKSMAN_VERSION = "2026-02-08"
POWERSHELL_EDITOR_SERVICES_VERSION = "v4.4.0"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        while True:
            chunk = handle.read(1024 * 1024)
            if not chunk:
                break
            digest.update(chunk)
    return digest.hexdigest()


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def clean_dir(path: Path) -> None:
    if path.exists():
        shutil.rmtree(path)
    ensure_dir(path)


def download(url: str, out_file: Path) -> None:
    ensure_dir(out_file.parent)
    with urllib.request.urlopen(url) as response, out_file.open("wb") as target:
        shutil.copyfileobj(response, target)


def extract_single_asset(archive_file: Path, archive_kind: str, matcher: Callable[[Path], bool], out_file: Path) -> None:
    ensure_dir(out_file.parent)
    if archive_kind == "zip":
        with zipfile.ZipFile(archive_file) as zf:
            names = [Path(name) for name in zf.namelist() if not name.endswith("/")]
            candidates = [name for name in names if matcher(name)]
            if not candidates:
                raise RuntimeError(f"Could not find matching file in zip: {archive_file}")
            chosen = sorted(candidates, key=lambda p: len(str(p)))[0]
            with zf.open(chosen.as_posix()) as src, out_file.open("wb") as dst:
                shutil.copyfileobj(src, dst)
        return

    if archive_kind == "gz":
        with gzip.open(archive_file, "rb") as src, out_file.open("wb") as dst:
            shutil.copyfileobj(src, dst)
        return

    if archive_kind == "raw":
        shutil.copy2(archive_file, out_file)
        return

    raise RuntimeError(f"Unsupported single asset archive kind: {archive_kind}")


def extract_pses_zip(archive_file: Path, out_dir: Path, script_name: str) -> None:
    clean_dir(out_dir)
    temp = out_dir.parent / "_pses_extract_tmp"
    clean_dir(temp)
    with zipfile.ZipFile(archive_file) as zf:
        zf.extractall(temp)
    scripts = list(temp.rglob(script_name))
    if not scripts:
        raise RuntimeError(f"Could not find {script_name} in PowerShellEditorServices bundle.")
    root = scripts[0].parent
    for item in root.iterdir():
        target = out_dir / item.name
        if item.is_dir():
            shutil.copytree(item, target, dirs_exist_ok=True)
        else:
            shutil.copy2(item, target)
    shutil.rmtree(temp, ignore_errors=True)


def run(cmd: list[str], cwd: Path | None = None, env: dict[str, str] | None = None) -> None:
    subprocess.run(cmd, cwd=str(cwd) if cwd else None, env=env, check=True)


def set_executable(path: Path) -> None:
    current = path.stat().st_mode
    path.chmod(current | 0o111)


def node_runtime_url(target_platform: str) -> tuple[str, str]:
    if target_platform == "windows":
        return (
            f"https://nodejs.org/dist/{NODE_VERSION}/node-{NODE_VERSION}-win-x64.zip",
            "zip",
        )
    if target_platform == "linux":
        return (
            f"https://nodejs.org/dist/{NODE_VERSION}/node-{NODE_VERSION}-linux-x64.tar.xz",
            "tar.xz",
        )
    raise RuntimeError(f"Unsupported platform: {target_platform}")


def install_node_runtime(target_platform: str, cache_dir: Path, output_base: Path) -> Path:
    url, archive_type = node_runtime_url(target_platform)
    archive_path = cache_dir / Path(url).name
    download(url, archive_path)
    extract_root = cache_dir / f"node-{target_platform}"
    clean_dir(extract_root)

    if archive_type == "zip":
        with zipfile.ZipFile(archive_path) as zf:
            zf.extractall(extract_root)
    else:
        with tarfile.open(archive_path) as tf:
            tf.extractall(extract_root)

    if target_platform == "windows":
        node_binary_candidates = list(extract_root.rglob("node.exe"))
    else:
        node_binary_candidates = [p for p in extract_root.rglob("node") if p.is_file() and "/bin/" in p.as_posix()]
    if not node_binary_candidates:
        raise RuntimeError(f"Node runtime binary not found after extraction for {target_platform}.")
    node_binary = sorted(node_binary_candidates, key=lambda p: len(str(p)))[0]

    runtime_dir = output_base / "node-runtime"
    clean_dir(runtime_dir)
    if target_platform == "windows":
        out_binary = runtime_dir / "node.exe"
    else:
        out_binary = runtime_dir / "bin" / "node"
    ensure_dir(out_binary.parent)
    shutil.copy2(node_binary, out_binary)
    if target_platform == "linux":
        set_executable(out_binary)
    return out_binary


def install_npm_package(pack_dir: Path, package_specs: list[str]) -> None:
    ensure_dir(pack_dir)
    package_json = pack_dir / "package.json"
    if not package_json.exists():
        package_json.write_text('{"name":"localcodeide-language-pack","private":true}\n', encoding="utf-8")
    npm_executable = shutil.which("npm.cmd") or shutil.which("npm")
    if not npm_executable:
        raise RuntimeError("npm executable was not found in PATH.")
    cmd = [npm_executable, "install", "--prefix", str(pack_dir), "--no-audit", "--no-fund", "--omit=dev", *package_specs]
    last_error: Exception | None = None
    for attempt in range(1, 5):
        try:
            run(cmd)
            return
        except Exception as exc:  # pylint: disable=broad-except
            last_error = exc
            shutil.rmtree(pack_dir / "node_modules", ignore_errors=True)
            if (pack_dir / "package-lock.json").exists():
                (pack_dir / "package-lock.json").unlink(missing_ok=True)
            if attempt < 4:
                time.sleep(2 * attempt)
    raise RuntimeError(f"npm install failed for {pack_dir}: {last_error}") from last_error


def write_text(path: Path, content: str) -> None:
    ensure_dir(path.parent)
    path.write_text(content, encoding="utf-8", newline="\n")


def build_node_entry_wrappers(target_platform: str, pack_root: Path, entry_relpath: str, shim_name: str) -> Path:
    node_dir = pack_root / "node"
    ensure_dir(node_dir)

    if target_platform == "windows":
        shim = node_dir / f"{shim_name}.cmd"
        body = (
            "@echo off\n"
            "setlocal\n"
            "set \"DIR=%~dp0\"\n"
            f"if not exist \"%DIR%node-runtime\\node.exe\" exit /b 1\n"
            f"if not exist \"%DIR%node_modules\\{entry_relpath.replace('/', '\\\\')}\" exit /b 1\n"
            f"\"%DIR%node-runtime\\node.exe\" \"%DIR%node_modules\\{entry_relpath.replace('/', '\\\\')}\" %*\n"
            "exit /b %ERRORLEVEL%\n"
        )
        write_text(shim, body)
        return shim

    shim = node_dir / shim_name
    body = (
        "#!/usr/bin/env sh\n"
        "set -eu\n"
        "DIR=\"$(CDPATH= cd -- \"$(dirname -- \"$0\")\" && pwd)\"\n"
        f"NODE=\"$DIR/node-runtime/bin/node\"\n"
        f"ENTRY=\"$DIR/node_modules/{entry_relpath}\"\n"
        "[ -x \"$NODE\" ] || exit 1\n"
        "[ -f \"$ENTRY\" ] || exit 1\n"
        "exec \"$NODE\" \"$ENTRY\" \"$@\"\n"
    )
    write_text(shim, body)
    set_executable(shim)
    return shim


def install_python_venv_with_package(pack_root: Path, package_spec: str, target_platform: str) -> Path:
    venv_root = pack_root / "python"
    clean_dir(venv_root)
    run(["python", "-m", "venv", str(venv_root)])

    if target_platform == "windows":
        pip_exe = venv_root / "Scripts" / "pip.exe"
        out_binary = venv_root / "Scripts" / "cmake-language-server.exe"
    else:
        pip_exe = venv_root / "bin" / "pip"
        out_binary = venv_root / "bin" / "cmake-language-server"
    run([str(pip_exe), "install", package_spec])
    if target_platform == "linux":
        set_executable(out_binary)
    return out_binary


def qt_qmlls_path_windows() -> Path | None:
    qt_dir = os.environ.get("QT_DIR")
    if qt_dir:
        candidate = Path(qt_dir) / "bin" / "qmlls.exe"
        if candidate.exists():
            return candidate
    base = Path("C:/Qt")
    if not base.exists():
        return None
    for entry in sorted(base.glob("*/msvc2022_64/bin/qmlls.exe"), reverse=True):
        if entry.exists():
            return entry
    return None


def qt_qmlls_path_linux() -> Path | None:
    qt_dir = os.environ.get("QT_DIR")
    if qt_dir:
        candidate = Path(qt_dir) / "bin" / "qmlls"
        if candidate.exists():
            return candidate
    for candidate in (
        Path("/usr/lib/qt6/bin/qmlls"),
        Path("/usr/bin/qmlls"),
    ):
        if candidate.exists():
            return candidate
    return None


def platform_assets(target_platform: str) -> dict[str, DirectAsset]:
    if target_platform == "windows":
        return {
            "rust-analyzer": DirectAsset(
                url=f"https://github.com/rust-lang/rust-analyzer/releases/download/{RUST_ANALYZER_VERSION}/rust-analyzer-x86_64-pc-windows-msvc.zip",
                archive="zip",
                matcher=lambda p: p.name.lower() == "rust-analyzer.exe",
                out_relpath="bin/rust-analyzer.exe",
            ),
            "clangd": DirectAsset(
                url=f"https://github.com/clangd/clangd/releases/download/{CLANGD_VERSION}/clangd-windows-{CLANGD_VERSION}.zip",
                archive="zip",
                matcher=lambda p: p.name.lower() == "clangd.exe",
                out_relpath="bin/clangd.exe",
            ),
            "taplo": DirectAsset(
                url=f"https://github.com/tamasfe/taplo/releases/download/{TAPLO_VERSION}/taplo-windows-x86_64.zip",
                archive="zip",
                matcher=lambda p: p.name.lower() == "taplo.exe",
                out_relpath="bin/taplo.exe",
            ),
            "marksman": DirectAsset(
                url=f"https://github.com/artempyanykh/marksman/releases/download/{MARKSMAN_VERSION}/marksman.exe",
                archive="raw",
                matcher=lambda p: p.name.lower() == "marksman.exe",
                out_relpath="bin/marksman.exe",
            ),
            "powershell-editor-services": DirectAsset(
                url=f"https://github.com/PowerShell/PowerShellEditorServices/releases/download/{POWERSHELL_EDITOR_SERVICES_VERSION}/PowerShellEditorServices.zip",
                archive="pses_zip",
                matcher=lambda p: p.name == "Start-EditorServices.ps1",
                out_relpath="bin/Start-EditorServices.ps1",
            ),
        }
    if target_platform == "linux":
        return {
            "rust-analyzer": DirectAsset(
                url=f"https://github.com/rust-lang/rust-analyzer/releases/download/{RUST_ANALYZER_VERSION}/rust-analyzer-x86_64-unknown-linux-gnu.gz",
                archive="gz",
                matcher=lambda p: p.name == "rust-analyzer",
                out_relpath="bin/rust-analyzer",
            ),
            "clangd": DirectAsset(
                url=f"https://github.com/clangd/clangd/releases/download/{CLANGD_VERSION}/clangd-linux-{CLANGD_VERSION}.zip",
                archive="zip",
                matcher=lambda p: p.name == "clangd",
                out_relpath="bin/clangd",
            ),
            "taplo": DirectAsset(
                url=f"https://github.com/tamasfe/taplo/releases/download/{TAPLO_VERSION}/taplo-linux-x86_64.gz",
                archive="gz",
                matcher=lambda p: p.name == "taplo",
                out_relpath="bin/taplo",
            ),
            "marksman": DirectAsset(
                url=f"https://github.com/artempyanykh/marksman/releases/download/{MARKSMAN_VERSION}/marksman-linux-x64",
                archive="raw",
                matcher=lambda p: p.name == "marksman-linux-x64",
                out_relpath="bin/marksman",
            ),
            "powershell-editor-services": DirectAsset(
                url=f"https://github.com/PowerShell/PowerShellEditorServices/releases/download/{POWERSHELL_EDITOR_SERVICES_VERSION}/PowerShellEditorServices.zip",
                archive="pses_zip",
                matcher=lambda p: p.name == "Start-EditorServices.sh",
                out_relpath="bin/Start-EditorServices.sh",
            ),
        }
    raise RuntimeError(f"Unsupported platform: {target_platform}")


def fetch_direct_assets(target_platform: str, repo_root: Path, cache_dir: Path, checksums: dict[str, dict[str, str]]) -> None:
    assets = platform_assets(target_platform)
    platform_root = repo_root / "language-packs" / target_platform
    for pack_id, spec in assets.items():
        pack_root = platform_root / pack_id
        ensure_dir(pack_root)

        archive_name = Path(spec.url).name
        archive_path = cache_dir / f"{target_platform}-{pack_id}-{archive_name}"
        download(spec.url, archive_path)

        out_file = pack_root / spec.out_relpath
        if spec.archive == "pses_zip":
            out_dir = pack_root / "bin"
            script_name = "Start-EditorServices.ps1" if target_platform == "windows" else "Start-EditorServices.sh"
            extract_pses_zip(archive_path, out_dir, script_name)
        else:
            extract_single_asset(archive_path, spec.archive, spec.matcher, out_file)
        if target_platform == "linux":
            set_executable(out_file)

        checksums.setdefault(target_platform, {})[f"{pack_id}:{spec.out_relpath}"] = sha256_file(out_file)


def fetch_node_based(target_platform: str, repo_root: Path, cache_dir: Path, checksums: dict[str, dict[str, str]]) -> None:
    platform_root = repo_root / "language-packs" / target_platform
    node_packs = {
        "typescript-language-server": (
            [f"typescript-language-server@{TSLS_VERSION}", f"typescript@{TYPESCRIPT_VERSION}"],
            "typescript-language-server/lib/cli.mjs",
            "typescript-language-server",
        ),
        "pyright": (
            [f"pyright@{PYRIGHT_VERSION}"],
            "pyright/index.js",
            "pyright-langserver",
        ),
        "json-language-server": (
            [f"vscode-langservers-extracted@{VSCODE_LANGSERVERS_EXTRACTED_VERSION}"],
            "vscode-langservers-extracted/bin/vscode-json-language-server",
            "vscode-json-language-server",
        ),
        "yaml-language-server": (
            [f"vscode-langservers-extracted@{VSCODE_LANGSERVERS_EXTRACTED_VERSION}"],
            "vscode-langservers-extracted/bin/vscode-yaml-language-server",
            "yaml-language-server",
        ),
    }

    for pack_id, (packages, entry_relpath, shim_name) in node_packs.items():
        pack_root = platform_root / pack_id
        ensure_dir(pack_root)
        clean_dir(pack_root / "node")
        node_binary = install_node_runtime(target_platform, cache_dir, pack_root / "node")
        install_npm_package(pack_root / "node", packages)
        shim = build_node_entry_wrappers(target_platform, pack_root, entry_relpath, shim_name)
        checksums.setdefault(target_platform, {})[f"{pack_id}:node-runtime"] = sha256_file(node_binary)
        checksums.setdefault(target_platform, {})[f"{pack_id}:node-shim"] = sha256_file(shim)


def fetch_python_pack(target_platform: str, repo_root: Path, checksums: dict[str, dict[str, str]]) -> None:
    pack_root = repo_root / "language-packs" / target_platform / "cmake-language-server"
    ensure_dir(pack_root)
    binary = install_python_venv_with_package(pack_root, f"cmake-language-server=={CMAKE_LANGUAGE_SERVER_VERSION}", target_platform)
    checksums.setdefault(target_platform, {})["cmake-language-server:runtime"] = sha256_file(binary)


def fetch_qmlls(target_platform: str, repo_root: Path, checksums: dict[str, dict[str, str]]) -> None:
    pack_root = repo_root / "language-packs" / target_platform / "qmlls"
    out_dir = pack_root / "bin"
    ensure_dir(out_dir)

    if target_platform == "windows":
        src = qt_qmlls_path_windows()
        if src is None:
            raise RuntimeError("Could not locate qmlls.exe. Set QT_DIR or install Qt with qmlls.")
        dst = out_dir / "qmlls.exe"
        shutil.copy2(src, dst)
    else:
        src = qt_qmlls_path_linux()
        if src is None:
            raise RuntimeError("Could not locate qmlls on Linux. Install Qt declarative tooling or set QT_DIR.")
        dst = out_dir / "qmlls"
        shutil.copy2(src, dst)
        set_executable(dst)

    checksums.setdefault(target_platform, {})["qmlls:runtime"] = sha256_file(dst)


def normalize_platform_arg(raw: str) -> list[str]:
    if raw == "all":
        return ["windows", "linux"]
    return [raw]


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Fetch pinned language packs into language-packs/<platform>/...")
    parser.add_argument("--root", default=".", help="Repository root.")
    parser.add_argument("--platform", choices=["windows", "linux", "all"], default="windows")
    parser.add_argument("--cache-dir", default=".cache/language-packs", help="Download cache directory.")
    parser.add_argument("--lock-file", default="resources/language-packs/checksums.lock.json", help="Output checksum lock file.")
    parser.add_argument("--clean", action="store_true", help="Delete platform language-packs/<platform> before fetching.")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    repo_root = Path(args.root).resolve()
    cache_dir = (repo_root / args.cache_dir).resolve()
    ensure_dir(cache_dir)

    checksums: dict[str, dict[str, str]] = {}
    platforms = normalize_platform_arg(args.platform)
    host_platform = "windows" if os.name == "nt" else "linux"

    for target_platform in platforms:
        if args.clean:
            print(f"Preparing clean fetch for {target_platform} (payload directories will be rebuilt).")
        if target_platform != host_platform:
            print(f"Skipping {target_platform}: host platform is {host_platform}.")
            continue

        fetch_direct_assets(target_platform, repo_root, cache_dir, checksums)
        fetch_node_based(target_platform, repo_root, cache_dir, checksums)
        fetch_python_pack(target_platform, repo_root, checksums)
        fetch_qmlls(target_platform, repo_root, checksums)

    lock_path = (repo_root / args.lock_file).resolve()
    ensure_dir(lock_path.parent)
    payload = {
        "schemaVersion": 1,
        "nodeVersion": NODE_VERSION,
        "versions": {
            "typescript-language-server": TSLS_VERSION,
            "typescript": TYPESCRIPT_VERSION,
            "pyright": PYRIGHT_VERSION,
            "vscode-langservers-extracted": VSCODE_LANGSERVERS_EXTRACTED_VERSION,
            "cmake-language-server": CMAKE_LANGUAGE_SERVER_VERSION,
            "rust-analyzer": RUST_ANALYZER_VERSION,
            "clangd": CLANGD_VERSION,
            "taplo": TAPLO_VERSION,
            "marksman": MARKSMAN_VERSION,
            "powershell-editor-services": POWERSHELL_EDITOR_SERVICES_VERSION,
        },
        "checksums": checksums,
    }
    lock_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Wrote checksum lock: {lock_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
