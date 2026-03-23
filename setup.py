#!/usr/bin/env python3
"""
LocalCodeIDE Automated Setup

Modern 2024 style:
- Uses vcpkg if available (for C++ dependencies)
- Uses aqt for Qt (pre-built, fast)
- Zero manual configuration
"""
import os
import sys
import subprocess
import shutil
import time
from pathlib import Path

# Colors
RED = '\033[91m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
GRAY = '\033[90m'
RESET = '\033[0m'

def log(msg, color=RESET):
    print(f"{color}{msg}{RESET}")

def check_cmd(cmd):
    return shutil.which(cmd) is not None

def find_vs():
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
    if os.environ.get('QT_DIR'):
        return os.environ['QT_DIR']
    qt_base = Path(r"C:\Qt")
    if qt_base.exists():
        for version in sorted(qt_base.iterdir(), reverse=True):
            if version.is_dir():
                for compiler in version.glob("msvc*2022_64"):
                    if compiler.is_dir():
                        return str(compiler)
    return None

def qt6_cmake_dir(qt_path):
    return str(Path(qt_path) / "lib" / "cmake" / "Qt6")

def find_qmllint():
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

def find_vcpkg():
    if os.environ.get('VCPKG_ROOT'):
        return os.environ['VCPKG_ROOT']
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
    log("Installing Qt 6.8.0 (this may take a while)...", BLUE)
    try:
        subprocess.run([
            sys.executable, "-m", "aqt", "install-qt",
            "windows", "desktop", "6.8.0", "win64_msvc2022_64",
            "-m", "qtquick3d", "qtquicktimeline", "qtcharts",
            "--outputdir", r"C:\Qt"
        ], check=True)
        return r"C:\Qt\6.8.0\msvc2022_64"
    except subprocess.CalledProcessError as e:
        log(f"Error: {e}", RED)
        return None

def deploy_qt(exe_path, qt_path):
    """Copy Qt DLLs to executable"""
    log("Deploying Qt dependencies...", BLUE)
    subprocess.run([
        f"{qt_path}\\bin\\windeployqt.exe",
        str(exe_path),
        "--release",
        "--qmldir", "qml",
        "--no-translations",
        "--no-opengl-sw"
    ], check=True)
    
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
    qt_conf.write_text("[Paths]\nPrefix = .\nPlugins = ./plugins\nQml2Imports = ./qml\n")


def required_qt_runtime_paths(exe_path):
    """Return required Qt runtime artifacts expected near the executable."""
    release_dir = exe_path.parent
    return [
        release_dir / "Qt6Core.dll",
        release_dir / "Qt6Gui.dll",
        release_dir / "Qt6Qml.dll",
        release_dir / "Qt6Quick.dll",
        release_dir / "plugins" / "platforms" / "qwindows.dll",
        release_dir / "qml" / "QtQuick" / "Controls" / "qtquickcontrols2plugin.dll",
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

    with log_path.open("w", encoding="utf-8", errors="ignore") as handle:
        proc = subprocess.Popen([str(exe_path)], stdout=handle, stderr=subprocess.STDOUT, env=env)
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
    log("\n" + "=" * 60, GREEN)
    log("LocalCodeIDE - Doctor", GREEN)
    log("=" * 60, GREEN)

    exe = Path("build/Release/LocalCodeIDE.exe")
    if not exe.exists():
        log("ERROR: Missing build/Release/LocalCodeIDE.exe. Run 'python setup.py' first.", RED)
        return 1

    qt_path = find_qt()
    if not qt_path:
        log("ERROR: Qt installation not found (C:\\Qt or QT_DIR).", RED)
        return 1

    os.environ['PATH'] = f"{qt_path}\\bin;{os.environ['PATH']}"

    log("\n[1/3] Qt runtime deploy check...", BLUE)
    if needs_qt_deploy(exe):
        log("Qt runtime incomplete. Running deploy...", YELLOW)
        try:
            deploy_qt(exe, qt_path)
        except subprocess.CalledProcessError as e:
            log(f"ERROR: Qt deploy failed: {e}", RED)
            return 1

    missing = [str(path) for path in required_qt_runtime_paths(exe) if not path.exists()]
    if missing:
        log("ERROR: Missing runtime artifacts after deploy:", RED)
        for item in missing:
            log(f"  - {item}", RED)
        return 1
    log("Qt runtime artifacts: OK", GREEN)

    log("\n[2/3] QML lint check...", BLUE)
    lint_result = lint()
    if lint_result != 0:
        log("ERROR: Lint failed.", RED)
        return lint_result

    log("\n[3/3] Startup smoke test...", BLUE)
    skip_startup_smoke = os.environ.get("LOCALCODEIDE_DOCTOR_SKIP_STARTUP", "").strip() == "1"
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
    log("\n" + "=" * 60, GREEN)
    log("LocalCodeIDE - Modern Setup (2024)", GREEN)
    log("=" * 60, GREEN)
    
    # 1. Python
    log(f"\n[1/7] Python: {sys.version.split()[0]}", BLUE)
    
    # 2. vcpkg (optional)
    log("\n[2/7] Checking vcpkg...", BLUE)
    vcpkg_path = find_vcpkg()
    if vcpkg_path:
        log(f"vcpkg: {vcpkg_path}", GREEN)
        os.environ['CMAKE_TOOLCHAIN_FILE'] = f"{vcpkg_path}/scripts/buildsystems/vcpkg.cmake"
    else:
        log("vcpkg not found (optional)", GRAY)
    
    # 3. aqt
    log("\n[3/7] Checking aqtinstall...", BLUE)
    try:
        subprocess.run([sys.executable, "-m", "aqt", "--version"],
                      capture_output=True, check=True)
    except subprocess.CalledProcessError:
        log("Installing aqtinstall...", YELLOW)
        subprocess.run([sys.executable, "-m", "pip", "install", "aqtinstall"], check=True)
    
    # 4. CMake
    log("\n[4/7] Checking CMake...", BLUE)
    if not check_cmd("cmake"):
        log("ERROR: Install CMake from cmake.org", RED)
        return 1
    result = subprocess.run(["cmake", "--version"], capture_output=True, text=True)
    log(result.stdout.strip().split('\n')[0], GREEN)
    
    # 5. Visual Studio
    log("\n[5/7] Checking Visual Studio...", BLUE)
    vs_path = find_vs()
    if not vs_path:
        log("ERROR: Install VS 2022+ with C++ workload", RED)
        log("winget install Microsoft.VisualStudio.2022.Community", YELLOW)
        return 1
    log(f"VS: {os.path.basename(os.path.dirname(vs_path))}", GREEN)
    
    # 6. Qt
    log("\n[6/7] Checking Qt...", BLUE)
    qt_path = find_qt()
    if not qt_path:
        log("Qt not found. Installing...", YELLOW)
        qt_path = install_qt()
        if not qt_path:
            return 1
    log(f"Qt: {Path(qt_path).parent.parent.name}", GREEN)
    
    # 7. Build
    log("\n[7/7] Building...", BLUE)
    os.environ['CMAKE_PREFIX_PATH'] = qt_path
    os.environ['Qt6_DIR'] = qt6_cmake_dir(qt_path)
    os.environ['PATH'] = f"{qt_path}\\bin;{os.environ['PATH']}"
    
    build_dir = Path("build")
    build_dir.mkdir(exist_ok=True)
    
    # CMake configure
    if not (build_dir / "CMakeCache.txt").exists():
        log("Configuring CMake...", BLUE)
        cmake_cmd = ["cmake", "-S", ".", "-B", "build", f"-DQt6_DIR={os.environ['Qt6_DIR']}"]
        if vcpkg_path:
            cmake_cmd.extend(["-DCMAKE_TOOLCHAIN_FILE=" + os.environ['CMAKE_TOOLCHAIN_FILE']])
        subprocess.run(cmake_cmd, check=True)
    
    # CMake build
    log("Compiling...", BLUE)
    subprocess.run(["cmake", "--build", "build", "--config", "Release"], check=True)
    
    # Deploy Qt
    deploy_qt(Path("build/Release/LocalCodeIDE.exe"), qt_path)
    
    # Success!
    log("\n" + "=" * 60, GREEN)
    log("BUILD COMPLETE!", GREEN)
    log("=" * 60, GREEN)
    log("\nTo run:", YELLOW)
    log("  python setup.py run", BLUE)
    log("\nOr: build\\Release\\LocalCodeIDE.exe", BLUE)
    
    return 0

def run():
    qt_path = find_qt()
    exe = Path("build/Release/LocalCodeIDE.exe")
    if not exe.exists():
        log("ERROR: Run 'python setup.py' first", RED)
        return 1
    
    if qt_path:
        os.environ['PATH'] = f"{qt_path}\\bin;{os.environ['PATH']}"
        if needs_qt_deploy(exe):
            log("Qt runtime not deployed to build/Release. Deploying now...", YELLOW)
            try:
                deploy_qt(exe, qt_path)
            except subprocess.CalledProcessError as e:
                log(f"ERROR: Qt deploy failed: {e}", RED)
                return 1
    else:
        log("WARNING: Qt installation not found in C:\\Qt or QT_DIR.", YELLOW)
        log("The app may fail to start if Qt runtime DLLs are missing.", YELLOW)

    result = subprocess.run([str(exe)])
    return result.returncode

def lint():
    """Run project lint checks (QML + Python)."""
    log("\n" + "=" * 60, GREEN)
    log("LocalCodeIDE - Lint", GREEN)
    log("=" * 60, GREEN)

    # Python lint (ruff)
    log("\n[1/3] Python lint (ruff)...", BLUE)
    try:
        subprocess.run([sys.executable, "-m", "ruff", "--version"], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        log("ruff not found. Installing...", YELLOW)
        subprocess.run([sys.executable, "-m", "pip", "install", "ruff"], check=True)
    subprocess.run([sys.executable, "-m", "ruff", "check", "setup.py", "version.py"], check=True)
    log("ruff: OK", GREEN)

    # QML lint (qmllint module mode)
    log("\n[2/3] QML lint (qmllint)...", BLUE)
    qmllint_path = find_qmllint()
    if not qmllint_path:
        log("ERROR: qmllint not found. Install Qt first.", RED)
        return 1

    qt_path = find_qt()
    if qt_path:
        os.environ['CMAKE_PREFIX_PATH'] = qt_path
        os.environ['Qt6_DIR'] = qt6_cmake_dir(qt_path)
        os.environ['PATH'] = f"{qt_path}\\bin;{os.environ['PATH']}"

    build_dir = Path("build")
    build_dir.mkdir(exist_ok=True)
    if not (build_dir / "CMakeCache.txt").exists():
        log("Configuring CMake (required for module lint)...", BLUE)
        subprocess.run(["cmake", "-S", ".", "-B", "build", f"-DQt6_DIR={os.environ['Qt6_DIR']}"], check=True)

    subprocess.run([
        qmllint_path,
        "-M",
        "-I", "build",
        "LocalCodeIDE",
        "--unqualified", "info",
        "--with", "info",
        "--unused-imports", "warning",
        "--multiline-strings", "warning",
        "--max-warnings", "0",
    ], check=True)
    log("qmllint: OK", GREEN)

    # Incremental strict checks for selected components
    log("\n[3/4] QML strict components...", BLUE)
    strict_qml_files = [
        "qml/components/Sidebar.qml",
        "qml/components/SearchPanel.qml",
    ]
    for qml_file in strict_qml_files:
        subprocess.run([
            qmllint_path,
            "--unqualified", "warning",
            "--max-warnings", "0",
            qml_file,
        ], check=True)
    log("strict components: OK", GREEN)

    log("\n[4/4] Lint summary", BLUE)
    log("All lint checks passed.", GREEN)
    return 0

def screenshot(output_path=""):
    """Capture a screenshot for LocalCodeIDE window (or desktop fallback)."""
    script_path = Path("scripts/capture-localcodeide-screenshot.ps1")
    if not script_path.exists():
        log("ERROR: Screenshot script not found", RED)
        return 1

    command = [
        "powershell",
        "-ExecutionPolicy", "Bypass",
        "-File", str(script_path),
    ]
    if output_path:
        command.extend(["-OutputPath", output_path])
    command.extend(["-LaunchIfMissing", "-FallbackToFullScreen"])

    subprocess.run(command, check=True)
    return 0


def build_installer():
    """Build WiX installer"""
    from version import get_version

    qt_path = find_qt()
    if not qt_path:
        log("ERROR: Qt not found. Run 'python setup.py' first", RED)
        return 1

    os.environ['PATH'] = f"{qt_path}\\bin;{os.environ['PATH']}"

    # Add WiX to PATH
    wix_path = Path.home() / ".dotnet" / "tools"
    os.environ['PATH'] = f"{wix_path};{os.environ['PATH']}"

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

    subprocess.run([
        "powershell", "-ExecutionPolicy", "Bypass",
        "-File", str(installer_script),
        "-Version", version
    ], check=True)

    log("\n" + "=" * 60, GREEN)
    log("INSTALLER BUILT SUCCESSFULLY!", GREEN)
    log("=" * 60, GREEN)
    log(f"\nOutput: build/installer/LocalCodeIDE-{version}.msi", BLUE)

    return 0


def install_test_deps():
    """Install test dependencies."""
    log("Installing test dependencies...", BLUE)
    subprocess.run([
        sys.executable, "-m", "pip", "install",
        "pytest>=7.0.0",
        "pytest-cov>=4.0.0",
        "pytest-xdist>=3.0.0",
        "requests>=2.28.0",
    ], check=True)
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
            log("  python setup.py doctor    - Run preflight checks (deploy/lint/startup smoke test)", BLUE)
            log("  python setup.py screenshot [path] - Capture app screenshot", BLUE)
            log("  python setup.py installer - Build WiX installer (.msi)", BLUE)
            log("  python setup.py testdeps  - Install test dependencies", BLUE)
            log("  python setup.py test [category] - Run tests", BLUE)
            log("    Categories: integration, server, inference, tools", BLUE)
            sys.exit(1)
    sys.exit(main())
