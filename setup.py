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
        "--no-translations",
        "--no-opengl-sw"
    ], check=True)
    
    # Copy QML modules
    qml_src = Path(qt_path) / "qml"
    qml_dst = Path("build/Release/qml")
    if qml_src.exists() and not qml_dst.exists():
        log("Copying QML modules...", BLUE)
        shutil.copytree(qml_src, qml_dst)
    
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
    os.environ['PATH'] = f"{qt_path}\\bin;{os.environ['PATH']}"
    
    build_dir = Path("build")
    build_dir.mkdir(exist_ok=True)
    
    # CMake configure
    if not (build_dir / "CMakeCache.txt").exists():
        log("Configuring CMake...", BLUE)
        cmake_cmd = ["cmake", "-S", ".", "-B", "build"]
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
    if qt_path:
        os.environ['PATH'] = f"{qt_path}\\bin;{os.environ['PATH']}"
    
    exe = Path("build/Release/LocalCodeIDE.exe")
    if not exe.exists():
        log("ERROR: Run 'python setup.py' first", RED)
        return 1
    
    subprocess.run([str(exe)])
    return 0

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "run":
        sys.exit(run())
    sys.exit(main())
