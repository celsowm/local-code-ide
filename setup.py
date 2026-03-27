#!/usr/bin/env python3
"""
LocalCodeIDE Automated Setup

Modern 2024 style:
- Uses vcpkg if available (for C++ dependencies)
- Uses aqt for Qt (pre-built, fast)
- Zero manual configuration
- Cross-platform: Windows and Linux
"""

import os
import sys
import subprocess
import shutil
import time
import json
from datetime import datetime, timezone
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

IS_WINDOWS = sys.platform == "win32"
IS_LINUX = sys.platform == "linux"

# Colors
RED = "\033[91m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
BLUE = "\033[94m"
GRAY = "\033[90m"
RESET = "\033[0m"


def log(msg, color=RESET):
    print(f"{color}{msg}{RESET}")


def check_cmd(cmd):
    return shutil.which(cmd) is not None


def _setup_aqt_linux_env():
    """Auto-detect aqt Qt installation and set environment variables.
    Returns True if aqt Qt was found and configured."""
    qt_dir = os.environ.get("QT_DIR")
    if qt_dir and Path(qt_dir).exists():
        # Already configured — just ensure LD_LIBRARY_PATH
        lib_dir = Path(qt_dir) / "lib"
        if lib_dir.exists():
            os.environ["LD_LIBRARY_PATH"] = (
                str(lib_dir) + ":" + os.environ.get("LD_LIBRARY_PATH", "")
            )
            qml_dir = Path(qt_dir) / "qml"
            if qml_dir.exists():
                os.environ["QML_IMPORT_PATH"] = str(qml_dir)
        return True

    # Scan ~/Qt/*/gcc_* for aqt installs
    qt_home = Path.home() / "Qt"
    if qt_home.exists():
        for ver_dir in sorted(qt_home.iterdir(), reverse=True):
            if ver_dir.is_dir():
                for compiler in ver_dir.glob("gcc*"):
                    cmake_dir = compiler / "lib" / "cmake" / "Qt6"
                    if cmake_dir.is_dir():
                        os.environ["QT_DIR"] = str(compiler)
                        os.environ["LD_LIBRARY_PATH"] = (
                            str(compiler / "lib")
                            + ":"
                            + os.environ.get("LD_LIBRARY_PATH", "")
                        )
                        qml_dir = compiler / "qml"
                        if qml_dir.exists():
                            os.environ["QML_IMPORT_PATH"] = str(qml_dir)
                        return True
    return False


def find_vs():
    if IS_LINUX:
        return None
    paths = [
        r"C:\Program Files\Microsoft Visual Studio\2026\Community\VC\Auxiliary\Build\vcvars64.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
    ]
    for p in paths:
        if os.path.exists(p):
            return p
    return None


def find_qt():
    if os.environ.get("QT_DIR"):
        return os.environ["QT_DIR"]

    if IS_LINUX:
        return _find_qt_linux()

    qt_base = Path(r"C:\Qt")
    if qt_base.exists():
        for version in sorted(qt_base.iterdir(), reverse=True):
            if version.is_dir():
                for compiler in version.glob("msvc*2022_64"):
                    if compiler.is_dir():
                        return str(compiler)
    return None


def _find_qt_linux():
    """Detect Qt6 installed via system packages on Linux.
    Returns the prefix directory (e.g. /usr, $HOME/Qt/6.8.0/gcc_64)."""
    # Check aqt-style local install first
    qt_home = Path.home() / "Qt"
    if qt_home.exists():
        for version in sorted(qt_home.iterdir(), reverse=True):
            if version.is_dir():
                for compiler in version.glob("gcc*"):
                    if compiler.is_dir():
                        cmake_dir = compiler / "lib" / "cmake" / "Qt6"
                        if cmake_dir.exists():
                            return str(compiler)

    # Common cmake config locations for system Qt6
    cmake_candidates = [
        Path("/usr/lib/x86_64-linux-gnu/cmake/Qt6"),
        Path("/usr/lib/cmake/Qt6"),
        Path("/usr/local/lib/cmake/Qt6"),
        Path("/usr/lib64/cmake/Qt6"),
    ]
    for candidate in cmake_candidates:
        if candidate.exists():
            # Return the prefix (go up 3 dirs from cmake/Qt6)
            return str(candidate.parents[2])

    return None


def qt6_cmake_dir(qt_path):
    return str(Path(qt_path) / "lib" / "cmake" / "Qt6")


def qt6_linux_cmake_dirs(prefix):
    """On Linux, given a Qt prefix path, return (cmake_prefix_path, qt6_cmake_dir) tuple."""
    p = Path(prefix)
    cmake_dir = p / "lib" / "cmake" / "Qt6"
    if cmake_dir.exists():
        return str(p), str(cmake_dir)
    # Multiarch fallback
    if p.name == "usr":
        for sub in (p / "lib").iterdir():
            candidate = sub / "cmake" / "Qt6"
            if candidate.is_dir():
                return str(p), str(candidate)
    return str(p), str(p / "lib" / "cmake" / "Qt6")


def find_qmllint():
    if IS_LINUX:
        return _find_qmllint_linux()

    qt_path = find_qt()
    if qt_path:
        candidate = Path(qt_path) / "bin" / "qmllint.exe"
        if candidate.exists():
            return str(candidate)

    direct = shutil.which("qmllint")
    if direct:
        return direct

    qt_base = Path(r"C:\Qt")
    if qt_base.exists():
        for candidate in qt_base.rglob("qmllint.exe"):
            return str(candidate)
    return None


def _find_qmllint_linux():
    """Find qmllint on Linux (aqt, system packages, or PATH)."""
    # Check aqt-style local install first (must match the Qt we build with)
    qt_home = Path.home() / "Qt"
    if qt_home.exists():
        for version in sorted(qt_home.iterdir(), reverse=True):
            if version.is_dir():
                for compiler in version.glob("gcc*"):
                    candidate = compiler / "bin" / "qmllint"
                    if candidate.exists():
                        return str(candidate)

    # Try PATH (typically /usr/bin/qmllint)
    for name in ("qmllint", "qmllint6"):
        found = shutil.which(name)
        if found:
            return found

    # Check known system locations
    search_dirs = [
        Path("/usr/lib/qt6/bin"),
        Path("/usr/bin"),
        Path("/usr/local/bin"),
    ]
    for d in search_dirs:
        candidate = d / "qmllint"
        if candidate.exists():
            return str(candidate)
        candidate = d / "qmllint6"
        if candidate.exists():
            return str(candidate)

    return None


def find_vcpkg():
    if os.environ.get("VCPKG_ROOT"):
        return os.environ["VCPKG_ROOT"]
    if IS_LINUX:
        paths = [
            Path.home() / "vcpkg",
            Path("/opt/vcpkg"),
        ]
        for p in paths:
            if (p / "vcpkg").exists():
                return str(p)
    else:
        paths = [
            Path(r"C:\vcpkg"),
            Path(r"C:\dev\vcpkg"),
            Path.home() / "vcpkg",
        ]
        for p in paths:
            if (p / "vcpkg.exe").exists():
                return str(p)
    return None


def install_qt():
    if IS_LINUX:
        log("Cannot auto-install Qt on Linux.", YELLOW)
        log("Option 1: Install Qt 6.5+ via aqt:", YELLOW)
        log("  pip install aqtinstall", YELLOW)
        log(
            "  python -m aqt install-qt linux desktop 6.8.0 gcc_64 --outputdir $HOME/Qt",
            YELLOW,
        )
        log("  export QT_DIR=$HOME/Qt/6.8.0/gcc_64", YELLOW)
        log("", YELLOW)
        log("Option 2: Install system packages (if your distro has Qt >= 6.5):", YELLOW)
        log(
            "  Debian/Ubuntu: sudo apt-get install qt6-base-dev qt6-declarative-dev qt6-tools-dev qt6-declarative-dev-tools",
            YELLOW,
        )
        return None

    log("Installing Qt 6.8.0 (this may take a while)...", BLUE)
    try:
        subprocess.run(
            [
                sys.executable,
                "-m",
                "aqt",
                "install-qt",
                "windows",
                "desktop",
                "6.8.0",
                "win64_msvc2022_64",
                "-m",
                "qtquick3d",
                "qtquicktimeline",
                "qtcharts",
                "--outputdir",
                r"C:\Qt",
            ],
            check=True,
        )
        return r"C:\Qt\6.8.0\msvc2022_64"
    except subprocess.CalledProcessError as e:
        log(f"Error: {e}", RED)
        return None


def deploy_qt(exe_path, qt_path):
    """Copy Qt DLLs to executable (Windows only)."""
    if IS_LINUX:
        log("Qt deploy not needed on Linux (using system packages).", GRAY)
        return
    log("Deploying Qt dependencies...", BLUE)
    subprocess.run(
        [
            f"{qt_path}\\bin\\windeployqt.exe",
            str(exe_path),
            "--release",
            "--qmldir",
            "qml",
            "--no-translations",
            "--no-opengl-sw",
        ],
        check=True,
    )

    # Ensure QML modules are present (windeployqt may leave an empty qml dir in some setups)
    qml_src = Path(qt_path) / "qml"
    qml_dst = Path("build/Release/qml")
    has_qml_content = qml_dst.exists() and any(qml_dst.rglob("*"))
    if qml_src.exists() and not has_qml_content:
        log("Copying QML modules...", BLUE)
        shutil.copytree(qml_src, qml_dst, dirs_exist_ok=True)

    # Move plugins to correct folder
    plugins_dst = Path("build/Release/plugins")
    if not plugins_dst.exists():
        plugins_dst.mkdir()
        for folder in ["platforms", "iconengines", "imageformats"]:
            src = Path("build/Release") / folder
            if src.exists():
                shutil.move(str(src), str(plugins_dst / folder))

    # Create qt.conf
    qt_conf = Path("build/Release/qt.conf")
    qt_conf.write_text(
        "[Paths]\nPrefix = .\nPlugins = ./plugins\nQml2Imports = ./qml\n"
    )

    # Copy bundled language packs (manifest + optional binaries).
    _copy_language_packs(Path("build/Release/language-packs"))


def _copy_language_packs(language_pack_dst):
    """Copy language packs to the destination directory."""
    language_pack_manifest_src = Path("resources/language-packs")
    language_pack_src = Path("language-packs")
    if language_pack_manifest_src.exists():
        language_pack_dst.mkdir(parents=True, exist_ok=True)
        shutil.copytree(
            language_pack_manifest_src, language_pack_dst, dirs_exist_ok=True
        )
    if language_pack_src.exists():
        language_pack_dst.mkdir(parents=True, exist_ok=True)
        shutil.copytree(language_pack_src, language_pack_dst, dirs_exist_ok=True)
        # Keep Linux launcher scripts executable in cross-platform artifacts.
        for launcher in (language_pack_dst / "linux").rglob("*.sh"):
            current_mode = launcher.stat().st_mode
            launcher.chmod(current_mode | 0o111)


def build_output_dir():
    """Return the build output directory (platform-specific)."""
    if IS_WINDOWS:
        return Path("build/Release")
    return Path("build/bin")


def executable_name():
    """Return the platform-specific executable name."""
    if IS_WINDOWS:
        return "LocalCodeIDE.exe"
    return "LocalCodeIDE"


def required_qt_runtime_paths(exe_path):
    """Return required Qt runtime artifacts expected near the executable."""
    release_dir = exe_path.parent
    if IS_LINUX:
        # On Linux, Qt libraries come from system packages; just check language packs exist
        return [
            release_dir / "language-packs" / "manifest.json",
            release_dir
            / "language-packs"
            / "linux"
            / "rust-analyzer"
            / "rust-analyzer.sh",
        ]
    return [
        release_dir / "Qt6Core.dll",
        release_dir / "Qt6Gui.dll",
        release_dir / "Qt6Qml.dll",
        release_dir / "Qt6Quick.dll",
        release_dir / "plugins" / "platforms" / "qwindows.dll",
        release_dir / "qml" / "QtQuick" / "Controls" / "qtquickcontrols2plugin.dll",
        release_dir / "language-packs" / "manifest.json",
        release_dir
        / "language-packs"
        / "windows"
        / "rust-analyzer"
        / "rust-analyzer.cmd",
        release_dir / "language-packs" / "linux" / "rust-analyzer" / "rust-analyzer.sh",
        release_dir / "qt.conf",
    ]


def needs_qt_deploy(exe_path):
    """Return True when essential Qt runtime artifacts are missing near the executable."""
    required_paths = required_qt_runtime_paths(exe_path)
    return any(not path.exists() for path in required_paths)


def smoke_test_startup(exe_path, timeout_seconds=8):
    """
    Start the app and ensure it stays alive for a short window.
    Returns: (ok: bool, details: str)
    """
    log_path = Path("build") / "doctor-startup.log"
    log_path.parent.mkdir(exist_ok=True)

    env = os.environ.copy()
    env["QT_FORCE_STDERR_LOGGING"] = "1"
    # On Linux without a display, use offscreen platform
    if IS_LINUX and not env.get("DISPLAY") and not env.get("WAYLAND_DISPLAY"):
        env["QT_QPA_PLATFORM"] = "offscreen"

    with log_path.open("w", encoding="utf-8", errors="ignore") as handle:
        proc = subprocess.Popen(
            [str(exe_path)], stdout=handle, stderr=subprocess.STDOUT, env=env
        )
        time.sleep(timeout_seconds)
        return_code = proc.poll()

        if return_code is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=3)
            return True, f"Startup smoke test passed ({timeout_seconds}s)."

        handle.flush()

    tail = ""
    if log_path.exists():
        lines = log_path.read_text(encoding="utf-8", errors="ignore").splitlines()
        tail = "\n".join(lines[-10:]) if lines else "(no startup logs)"
    details = (
        f"App exited early with code {return_code}. "
        f"See {log_path} for full logs.\nLast lines:\n{tail}"
    )
    return False, details


def doctor():
    """Preflight checks similar to a 'health check' command."""
    if IS_LINUX:
        _setup_aqt_linux_env()
    log("\n" + "=" * 60, GREEN)
    log("LocalCodeIDE - Doctor", GREEN)
    log("=" * 60, GREEN)

    out_dir = build_output_dir()
    exe = out_dir / executable_name()
    if not exe.exists():
        log(f"ERROR: Missing {exe}. Run 'python setup.py' first.", RED)
        return 1

    qt_path = find_qt()
    if not qt_path and IS_WINDOWS:
        log("ERROR: Qt installation not found (C:\\Qt or QT_DIR).", RED)
        return 1
    elif not qt_path and IS_LINUX:
        log("WARNING: Qt not found via QT_DIR or system cmake paths.", YELLOW)
        log("Ensure Qt6 dev packages are installed.", YELLOW)

    if IS_WINDOWS and qt_path:
        os.environ["PATH"] = f"{qt_path}\\bin;{os.environ['PATH']}"

    # Run deploy and lint checks concurrently
    log("\n[1/3] Running preflight checks (Deploy + Lint)...", BLUE)

    def check_deploy():
        if needs_qt_deploy(exe):
            if qt_path:
                deploy_qt(exe, qt_path)
        return [
            str(path) for path in required_qt_runtime_paths(exe) if not path.exists()
        ]

    with ThreadPoolExecutor(max_workers=2) as executor:
        deploy_future = executor.submit(check_deploy)
        lint_future = executor.submit(lint)

        missing = deploy_future.result()
        lint_result = lint_future.result()

    if missing:
        log("ERROR: Missing runtime artifacts after deploy:", RED)
        for item in missing:
            log(f"  - {item}", RED)
        return 1
    log("Runtime artifacts: OK", GREEN)

    if lint_result != 0:
        log("ERROR: Lint failed.", RED)
        return lint_result

    log("\n[3/3] Startup smoke test...", BLUE)
    skip_startup_smoke = (
        os.environ.get("LOCALCODEIDE_DOCTOR_SKIP_STARTUP", "").strip() == "1"
    )
    if skip_startup_smoke:
        log("Skipped (LOCALCODEIDE_DOCTOR_SKIP_STARTUP=1).", YELLOW)
    else:
        ok, details = smoke_test_startup(exe)
        if not ok:
            log("ERROR: " + details, RED)
            return 1
        log(details, GREEN)

    log("\nDoctor checks passed.", GREEN)
    return 0


def main():
    if IS_LINUX:
        _setup_aqt_linux_env()

    log("\n" + "=" * 60, GREEN)
    log("LocalCodeIDE - Modern Setup (2024)", GREEN)
    log("=" * 60, GREEN)
    log(f"Platform: {sys.platform}", GRAY)

    # 1. Python
    log(f"\n[1/7] Python: {sys.version.split()[0]}", BLUE)

    # 2. vcpkg (optional)
    log("\n[2/7] Checking vcpkg...", BLUE)
    vcpkg_path = find_vcpkg()
    if vcpkg_path:
        log(f"vcpkg: {vcpkg_path}", GREEN)
        os.environ["CMAKE_TOOLCHAIN_FILE"] = (
            f"{vcpkg_path}/scripts/buildsystems/vcpkg.cmake"
        )
    else:
        log("vcpkg not found (optional)", GRAY)

    # 3. aqt (only on Windows; Linux uses system packages)
    if IS_WINDOWS:
        log("\n[3/7] Checking aqtinstall...", BLUE)
        try:
            subprocess.run(
                [sys.executable, "-m", "aqt", "--version"],
                capture_output=True,
                check=True,
            )
        except subprocess.CalledProcessError:
            log("Installing aqtinstall...", YELLOW)
            subprocess.run(
                [sys.executable, "-m", "pip", "install", "aqtinstall"],
                check=True,
            )
    else:
        log("\n[3/7] aqtinstall: skipped (Linux uses system packages)", GRAY)

    # 4. CMake
    log("\n[4/7] Checking CMake...", BLUE)
    if not check_cmd("cmake"):
        log("ERROR: Install CMake from cmake.org or your package manager.", RED)
        return 1
    result = subprocess.run(["cmake", "--version"], capture_output=True, text=True)
    log(result.stdout.strip().split("\n")[0], GREEN)

    # 5. Compiler
    log("\n[5/7] Checking compiler...", BLUE)
    if IS_WINDOWS:
        vs_path = find_vs()
        if not vs_path:
            log("ERROR: Install VS 2022+ with C++ workload", RED)
            log("winget install Microsoft.VisualStudio.2022.Community", YELLOW)
            return 1
        log(f"VS: {os.path.basename(os.path.dirname(vs_path))}", GREEN)
    else:
        compiler = _check_linux_compiler()
        if not compiler:
            return 1

    # 6. Qt
    log("\n[6/7] Checking Qt...", BLUE)
    qt_path = find_qt()
    if not qt_path:
        log("Qt not found. Attempting install...", YELLOW)
        qt_path = install_qt()
        if not qt_path:
            return 1

    if IS_LINUX:
        cmake_prefix, qt6_dir_path = qt6_linux_cmake_dirs(qt_path)
        log(f"Qt6_DIR: {qt6_dir_path}", GREEN)
        if not Path(qt6_dir_path).exists():
            log(
                f"WARNING: {qt6_dir_path} not found. CMake may fail to find Qt6.",
                YELLOW,
            )
    else:
        log(f"Qt: {Path(qt_path).parent.parent.name}", GREEN)

    # 7. Build
    log("\n[7/7] Building...", BLUE)
    if IS_LINUX and qt_path:
        cmake_prefix, qt6_dir_path = qt6_linux_cmake_dirs(qt_path)
        os.environ["CMAKE_PREFIX_PATH"] = cmake_prefix
        os.environ["Qt6_DIR"] = qt6_dir_path
    elif qt_path:
        os.environ["CMAKE_PREFIX_PATH"] = qt_path
        os.environ["Qt6_DIR"] = qt6_cmake_dir(qt_path)
    if IS_WINDOWS and qt_path:
        os.environ["PATH"] = f"{qt_path}\\bin;{os.environ['PATH']}"

    build_dir = Path("build")
    build_dir.mkdir(exist_ok=True)

    # CMake configure
    if not (build_dir / "CMakeCache.txt").exists():
        log("Configuring CMake...", BLUE)
        cmake_cmd = ["cmake", "-S", ".", "-B", "build"]
        if qt_path:
            cmake_cmd.append(f"-DQt6_DIR={os.environ['Qt6_DIR']}")
        if vcpkg_path:
            cmake_cmd.extend(
                ["-DCMAKE_TOOLCHAIN_FILE=" + os.environ["CMAKE_TOOLCHAIN_FILE"]]
            )
        subprocess.run(cmake_cmd, check=True)

    # CMake build
    log("Compiling...", BLUE)
    cmake_build_cmd = ["cmake", "--build", "build"]
    if IS_WINDOWS:
        cmake_build_cmd.extend(["--config", "Release"])
    subprocess.run(cmake_build_cmd, check=True)

    # Deploy Qt (Windows only)
    out_dir = build_output_dir()
    exe = out_dir / executable_name()
    if IS_WINDOWS:
        deploy_qt(exe, qt_path)
    else:
        # On Linux, copy language packs to build dir
        lp_dst = out_dir / "language-packs"
        _copy_language_packs(lp_dst)

    # Ensure executable is executable on Linux
    if IS_LINUX and exe.exists():
        exe.chmod(exe.stat().st_mode | 0o111)

    # Success!
    log("\n" + "=" * 60, GREEN)
    log("BUILD COMPLETE!", GREEN)
    log("=" * 60, GREEN)
    log("\nTo run:", YELLOW)
    if IS_WINDOWS:
        log("  python setup.py run", BLUE)
        log("\nOr: build\\Release\\LocalCodeIDE.exe", BLUE)
    else:
        log("  python setup.py run", BLUE)
        log("\nOr: ./build/bin/LocalCodeIDE", BLUE)

    return 0


def _check_linux_compiler():
    """Check for a C++20 compiler on Linux."""
    for name in ("g++", "clang++"):
        path = shutil.which(name)
        if path:
            # Get version
            try:
                result = subprocess.run(
                    [path, "--version"], capture_output=True, text=True
                )
                first_line = result.stdout.split("\n")[0]
                log(f"Compiler: {name} ({first_line})", GREEN)
                return path
            except Exception:
                pass

    log("ERROR: No C++ compiler found. Install g++ or clang++:", RED)
    log("  Debian/Ubuntu: sudo apt-get install build-essential", YELLOW)
    log("  Fedora:        sudo dnf groupinstall 'Development Tools'", YELLOW)
    log("  Arch:          sudo pacman -S base-devel", YELLOW)
    return None


def _default_run_env():
    """Apply default run-time observability settings."""
    os.environ.setdefault("LOCALCODEIDE_LOG_LEVEL", "info")
    os.environ.setdefault("LOCALCODEIDE_KEEP_LOG_FILES", "20")
    if IS_WINDOWS:
        os.environ.setdefault("LOCALCODEIDE_ENABLE_MINIDUMP", "0")


def _run_logs_root():
    return Path("build") / "logs"


def _sanitize_keep_logs(value, default=20):
    try:
        keep = int(str(value).strip())
        if keep < 1:
            return default
        return keep
    except (TypeError, ValueError):
        return default


def _rotate_run_sessions(logs_root, keep_count):
    if not logs_root.exists():
        return

    sessions = [entry for entry in logs_root.iterdir() if entry.is_dir()]
    sessions.sort(key=lambda entry: entry.stat().st_mtime, reverse=True)
    for stale in sessions[keep_count:]:
        shutil.rmtree(stale, ignore_errors=True)


def _new_run_session(logs_root):
    logs_root.mkdir(parents=True, exist_ok=True)
    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    session_id = f"{timestamp}-{os.getpid()}-{int(time.time() * 1000) % 1000:03d}"
    session_dir = logs_root / session_id
    session_dir.mkdir(parents=True, exist_ok=False)
    return session_dir


def _tail_text_file(path, line_count=20):
    if not path.exists():
        return "(log file not found)"
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    if not lines:
        return "(log file is empty)"
    return "\n".join(lines[-line_count:])


def _now_iso_utc():
    return datetime.now(timezone.utc).isoformat()


def _normalize_exit_for_parent(exit_code):
    """Normalize return code for sys.exit on Windows (signed 32-bit)."""
    if not IS_WINDOWS:
        return int(exit_code)
    code = int(exit_code) & 0xFFFFFFFF
    if code >= 0x80000000:
        return code - 0x100000000
    return code


def _file_size(path):
    try:
        return path.stat().st_size
    except OSError:
        return 0


def run():
    if IS_LINUX:
        _setup_aqt_linux_env()

    _default_run_env()

    qt_path = find_qt()
    out_dir = build_output_dir()
    exe = out_dir / executable_name()
    if not exe.exists():
        log(f"ERROR: Run 'python setup.py' first (missing {exe})", RED)
        return 1

    if IS_WINDOWS and qt_path:
        os.environ["PATH"] = f"{qt_path}\\bin;{os.environ['PATH']}"
        if needs_qt_deploy(exe):
            log("Qt runtime not deployed to build/Release. Deploying now...", YELLOW)
            try:
                deploy_qt(exe, qt_path)
            except subprocess.CalledProcessError as e:
                log(f"ERROR: Qt deploy failed: {e}", RED)
                return 1
    elif IS_LINUX:
        # Make sure executable is executable
        exe.chmod(exe.stat().st_mode | 0o111)

    logs_root = _run_logs_root()
    keep_count = _sanitize_keep_logs(os.environ.get("LOCALCODEIDE_KEEP_LOG_FILES", "20"))
    _rotate_run_sessions(logs_root, keep_count)
    session_dir = _new_run_session(logs_root)
    runtime_log = session_dir / "runtime.log"
    session_json = session_dir / "session.json"

    env = os.environ.copy()
    env["LOCALCODEIDE_SESSION_DIR"] = str(session_dir.resolve())
    env["LOCALCODEIDE_RUNTIME_LOG"] = str(runtime_log.resolve())
    env["LOCALCODEIDE_APP_LOG_FILE"] = str((session_dir / "app.log").resolve())
    env["QT_FORCE_STDERR_LOGGING"] = "1"

    log_level = env.get("LOCALCODEIDE_LOG_LEVEL", "info").strip().lower()
    if log_level == "debug":
        env["QT_LOGGING_RULES"] = "*.debug=true"
    elif log_level == "warn":
        env["QT_LOGGING_RULES"] = "*.debug=false;*.info=false"
    else:
        env["QT_LOGGING_RULES"] = "*.debug=false"

    start_wall = time.time()
    start_iso = _now_iso_utc()
    exit_code = 1
    interrupted = False

    with runtime_log.open("w", encoding="utf-8", errors="replace") as handle:
        try:
            proc = subprocess.Popen(
                [str(exe)],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                env=env,
                text=True,
                encoding="utf-8",
                errors="replace",
                bufsize=1,
            )
            if proc.stdout is not None:
                for line in proc.stdout:
                    sys.stdout.write(line)
                    handle.write(line)
            exit_code = proc.wait()
        except KeyboardInterrupt:
            interrupted = True
            if "proc" in locals() and proc.poll() is None:
                proc.terminate()
                try:
                    proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait(timeout=3)
            exit_code = 130
            log("\nRun interrupted by user.", YELLOW)
        finally:
            handle.flush()

    end_iso = _now_iso_utc()
    duration_ms = int((time.time() - start_wall) * 1000)
    session_meta = {
        "schema": "localcodeide/run-session-v1",
        "platform": sys.platform,
        "cwd": str(Path.cwd()),
        "command": [str(exe)],
        "started_at_utc": start_iso,
        "ended_at_utc": end_iso,
        "duration_ms": duration_ms,
        "exit_code": exit_code,
        "exit_code_for_parent": _normalize_exit_for_parent(exit_code),
        "interrupted": interrupted,
        "artifacts": {
            "runtime_log": str(runtime_log.resolve()),
            "app_log": str((session_dir / "app.log").resolve()),
            "minidump": str((session_dir / "crash.dmp").resolve()),
        },
        "env": {
            "LOCALCODEIDE_LOG_LEVEL": env.get("LOCALCODEIDE_LOG_LEVEL", ""),
            "LOCALCODEIDE_KEEP_LOG_FILES": env.get("LOCALCODEIDE_KEEP_LOG_FILES", ""),
            "LOCALCODEIDE_ENABLE_MINIDUMP": env.get("LOCALCODEIDE_ENABLE_MINIDUMP", ""),
        },
    }
    session_json.write_text(json.dumps(session_meta, indent=2), encoding="utf-8")

    _rotate_run_sessions(logs_root, keep_count)

    log("\nRun session artifacts:", BLUE)
    log(f"  - Session: {session_dir}", BLUE)
    log(f"  - Runtime log: {runtime_log}", BLUE)
    log(f"  - Session metadata: {session_json}", BLUE)

    if exit_code != 0:
        log("\nApplication exited with an error.", RED)
        log(f"Exit code: {exit_code}", RED)
        log(f"Runtime log: {runtime_log}", RED)
        if _file_size(runtime_log) > 0:
            log("Last runtime log lines:", RED)
            log(_tail_text_file(runtime_log, line_count=20), RED)
        else:
            app_log = session_dir / "app.log"
            if _file_size(app_log) > 0:
                log("Runtime log is empty. Last app log lines:", RED)
                log(_tail_text_file(app_log, line_count=20), RED)
            else:
                log("Runtime and app logs are empty for this crash.", RED)

    return _normalize_exit_for_parent(exit_code)


def lint():
    """Run project lint checks (QML + Python)."""
    if IS_LINUX:
        _setup_aqt_linux_env()
    log("\n" + "=" * 60, GREEN)
    log("LocalCodeIDE - Lint", GREEN)
    log("=" * 60, GREEN)

    # Run Python and QML lint concurrently
    log("\n[1/3] Running Python and QML lint concurrently...", BLUE)

    def run_ruff():
        try:
            subprocess.run(
                [sys.executable, "-m", "ruff", "--version"],
                capture_output=True,
                check=True,
            )
        except (subprocess.CalledProcessError, FileNotFoundError):
            log("ruff not found. Installing...", YELLOW)
            try:
                subprocess.run(
                    [sys.executable, "-m", "pip", "install", "ruff"],
                    check=True,
                    capture_output=True,
                )
            except subprocess.CalledProcessError:
                # PEP 668 — try --user or pipx
                try:
                    subprocess.run(
                        [sys.executable, "-m", "pip", "install", "--user", "ruff"],
                        check=True,
                        capture_output=True,
                    )
                except subprocess.CalledProcessError:
                    log(
                        "WARNING: Could not install ruff (externally-managed environment). Skipping Python lint.",
                        YELLOW,
                    )
                    return 0
        subprocess.run(
            [sys.executable, "-m", "ruff", "check", "setup.py", "version.py"],
            check=True,
        )
        return 0

    def run_qmllint():
        qmllint_path = find_qmllint()
        if not qmllint_path:
            log("ERROR: qmllint not found. Install Qt first.", RED)
            return 1

        qt_path = find_qt()
        env = os.environ.copy()
        if qt_path:
            if IS_LINUX:
                cmake_prefix, qt6_dir_path = qt6_linux_cmake_dirs(qt_path)
                env["CMAKE_PREFIX_PATH"] = cmake_prefix
                env["Qt6_DIR"] = qt6_dir_path
            else:
                env["CMAKE_PREFIX_PATH"] = qt_path
                env["Qt6_DIR"] = qt6_cmake_dir(qt_path)
                if IS_WINDOWS:
                    env["PATH"] = f"{qt_path}\\bin;{env['PATH']}"

        build_dir = Path("build")
        build_dir.mkdir(exist_ok=True)
        if not (build_dir / "CMakeCache.txt").exists():
            log("Configuring CMake (required for module lint)...", BLUE)
            cmake_cmd = ["cmake", "-S", ".", "-B", "build"]
            if qt_path:
                cmake_cmd.append(f"-DQt6_DIR={env['Qt6_DIR']}")
            subprocess.run(cmake_cmd, env=env, check=True)

        # Check qmllint version — module mode (-M) requires Qt 6.5+
        use_module_mode = True
        try:
            result = subprocess.run(
                [qmllint_path, "--version"], capture_output=True, text=True
            )
            version_match = result.stdout
            if (
                "6.4" in version_match
                or "6.3" in version_match
                or "6.2" in version_match
            ):
                use_module_mode = False
                log(
                    "qmllint: module mode (-M) requires Qt 6.5+, running in basic mode.",
                    YELLOW,
                )
        except Exception:
            pass

        if use_module_mode:
            subprocess.run(
                [
                    qmllint_path,
                    "-M",
                    "-I",
                    "build",
                    "LocalCodeIDE",
                    "--unqualified",
                    "info",
                    "--with",
                    "info",
                    "--unused-imports",
                    "warning",
                    "--multiline-strings",
                    "warning",
                    "--max-warnings",
                    "0",
                ],
                env=env,
                check=True,
            )
        else:
            log(
                "qmllint: skipped (basic mode, requires Qt 6.5+ for module lint).", GRAY
            )

        # Incremental strict checks for selected components (requires Qt 6.5+)
        if use_module_mode:
            strict_qml_files = [
                "qml/components/Sidebar.qml",
                "qml/components/SearchPanel.qml",
            ]
            for qml_file in strict_qml_files:
                subprocess.run(
                    [
                        qmllint_path,
                        "--unqualified",
                        "warning",
                        "--max-warnings",
                        "0",
                        qml_file,
                    ],
                    env=env,
                    check=True,
                )
        return 0

    with ThreadPoolExecutor(max_workers=2) as executor:
        ruff_future = executor.submit(run_ruff)
        qmllint_future = executor.submit(run_qmllint)

        r1 = ruff_future.result()
        r2 = qmllint_future.result()

    if r1 != 0 or r2 != 0:
        return 1

    log("\n[4/4] Lint summary", BLUE)
    log("All lint checks passed.", GREEN)
    return 0


def screenshot(output_path=""):
    """Capture a screenshot of LocalCodeIDE window (or desktop fallback)."""
    if IS_WINDOWS:
        return _screenshot_windows(output_path)
    return _screenshot_linux(output_path)


def _screenshot_windows(output_path=""):
    script_path = Path("scripts/capture-localcodeide-screenshot.ps1")
    if not script_path.exists():
        log("ERROR: Screenshot script not found", RED)
        return 1

    command = [
        "powershell",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(script_path),
    ]
    if output_path:
        command.extend(["-OutputPath", output_path])
    command.extend(["-LaunchIfMissing", "-FallbackToFullScreen"])

    subprocess.run(command, check=True)
    return 0


def _screenshot_linux(output_path=""):
    if not output_path:
        output_path = "screenshot.png"

    # Try gnome-screenshot
    if check_cmd("gnome-screenshot"):
        subprocess.run(["gnome-screenshot", "-f", output_path], check=True)
        log(f"Screenshot saved to {output_path}", GREEN)
        return 0

    # Try ImageMagick import
    if check_cmd("import"):
        subprocess.run(["import", "-window", "root", output_path], check=True)
        log(f"Screenshot saved to {output_path}", GREEN)
        return 0

    # Try grim (Wayland)
    if check_cmd("grim"):
        subprocess.run(["grim", output_path], check=True)
        log(f"Screenshot saved to {output_path}", GREEN)
        return 0

    log(
        "ERROR: No screenshot tool found. Install gnome-screenshot, imagemagick, or grim.",
        RED,
    )
    return 1


def build_installer():
    """Build installer (WiX on Windows, AppImage on Linux)."""
    if IS_LINUX:
        return _build_appimage()
    return _build_wix_installer()


def _build_wix_installer():
    """Build WiX MSI installer (Windows only)."""
    from version import get_version

    qt_path = find_qt()
    if not qt_path:
        log("ERROR: Qt not found. Run 'python setup.py' first", RED)
        return 1

    os.environ["PATH"] = f"{qt_path}\\bin;{os.environ['PATH']}"

    # Add WiX to PATH
    wix_path = Path.home() / ".dotnet" / "tools"
    os.environ["PATH"] = f"{wix_path};{os.environ['PATH']}"

    # Check if wix is installed
    try:
        subprocess.run(["wix", "--version"], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        log("WiX Toolset not found. Installing...", YELLOW)
        subprocess.run(["dotnet", "tool", "install", "--global", "wix"], check=True)

    # Run installer build script
    installer_script = Path("installer/build-installer.ps1")
    if not installer_script.exists():
        log("ERROR: Installer script not found", RED)
        return 1

    version = get_version()
    log(f"\nBuilding installer for version {version}...", BLUE)

    subprocess.run(
        [
            "powershell",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(installer_script),
            "-Version",
            version,
        ],
        check=True,
    )

    log("\n" + "=" * 60, GREEN)
    log("INSTALLER BUILT SUCCESSFULLY!", GREEN)
    log("=" * 60, GREEN)
    log(f"\nOutput: build/installer/LocalCodeIDE-{version}.msi", BLUE)

    return 0


def _build_appimage():
    """Build AppImage (Linux only)."""
    script_path = Path("scripts/build-appimage.sh")
    if not script_path.exists():
        log("ERROR: AppImage build script not found at scripts/build-appimage.sh", RED)
        return 1

    from version import get_version

    version = get_version()

    log(f"\nBuilding AppImage for version {version}...", BLUE)

    # Make sure the binary exists
    out_dir = build_output_dir()
    exe = out_dir / executable_name()
    if not exe.exists():
        log("ERROR: Build the project first with 'python setup.py'", RED)
        return 1

    subprocess.run(["bash", str(script_path), version], check=True)

    log("\n" + "=" * 60, GREEN)
    log("APPIMAGE BUILT SUCCESSFULLY!", GREEN)
    log("=" * 60, GREEN)
    log(f"\nOutput: build/LocalCodeIDE-{version}.AppImage", BLUE)

    return 0


def install_test_deps():
    """Install test dependencies."""
    log("Installing test dependencies...", BLUE)
    subprocess.run(
        [
            sys.executable,
            "-m",
            "pip",
            "install",
            "pytest>=7.0.0",
            "pytest-cov>=4.0.0",
            "pytest-xdist>=3.0.0",
            "requests>=2.28.0",
        ],
        check=True,
    )
    log("Test dependencies installed!", GREEN)
    return 0


def run_tests(category="", verbose=False):
    """Run test suite."""
    test_script = Path("run_tests.py")
    if not test_script.exists():
        log("ERROR: Test runner not found", RED)
        return 1

    cmd = [sys.executable, str(test_script)]

    if category:
        cmd.extend(["--category", category])

    if verbose:
        cmd.append("-v")

    result = subprocess.run(cmd)
    return result.returncode


if __name__ == "__main__":
    if len(sys.argv) > 1:
        if sys.argv[1] == "run":
            sys.exit(run())
        elif sys.argv[1] == "lint":
            sys.exit(lint())
        elif sys.argv[1] == "screenshot":
            output = sys.argv[2] if len(sys.argv) > 2 else ""
            sys.exit(screenshot(output))
        elif sys.argv[1] == "installer":
            sys.exit(build_installer())
        elif sys.argv[1] == "doctor":
            sys.exit(doctor())
        elif sys.argv[1] == "testdeps":
            sys.exit(install_test_deps())
        elif sys.argv[1] == "test":
            category = sys.argv[2] if len(sys.argv) > 2 else ""
            sys.exit(run_tests(category))
        else:
            log(f"Unknown command: {sys.argv[1]}", RED)
            log("Usage:", YELLOW)
            log("  python setup.py           - Build the project", BLUE)
            log("  python setup.py run       - Run the application", BLUE)
            log("  python setup.py lint      - Run lint checks", BLUE)
            log(
                "  python setup.py doctor    - Run preflight checks (deploy/lint/startup smoke test)",
                BLUE,
            )
            log("  python setup.py screenshot [path] - Capture app screenshot", BLUE)
            if IS_WINDOWS:
                log("  python setup.py installer - Build WiX installer (.msi)", BLUE)
            else:
                log("  python setup.py installer - Build AppImage", BLUE)
            log("  python setup.py testdeps  - Install test dependencies", BLUE)
            log("  python setup.py test [category] - Run tests", BLUE)
            log("    Categories: integration, server, inference, tools", BLUE)
            sys.exit(1)
    sys.exit(main())
