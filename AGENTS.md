# LocalCodeIDE - Developer Guide

## ⚠️ Important Rules for AI Agents

1. **ALWAYS use English** for all documentation, comments, and user-facing text
2. **README.md** is for end-users only (download links, features, overview)
3. **AGENTS.md** is for developers/contributors (build instructions, technical details)
4. **Never use placeholders** - check the actual codebase using git, gh CLI, or file exploration
5. **Verify before assuming** - check what files exist, what dependencies are installed, etc.

---

## 🛠️ Development Setup

### Prerequisites

| Dependency | How to install |
|------------|----------------|
| Python 3.8+ | [python.org](https://python.org) |
| Visual Studio 2022+ | `winget install Microsoft.VisualStudio.2022.Community` |
| CMake | `winget install Kitware.CMake` |

### Quick Setup

```bash
# Install everything and build
python setup.py

# Lint (QML + Python)
python setup.py lint

# Full preflight (Qt runtime + lint + startup smoke test)
python setup.py doctor

# Run
python setup.py run

# Install test dependencies
python setup.py testdeps

# Run tests
python setup.py test
```

### Qt + CMake Path Rules

- `aqt install-qt` uses `win64_msvc2022_64` as the package selector, but the installed directory is `C:\Qt\<version>\msvc2022_64`
- When configuring CMake manually, prefer setting both `CMAKE_PREFIX_PATH` and `Qt6_DIR`
- Recommended manual configure command on Windows:

```bash
cmake -S . -B build ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.0/msvc2022_64 ^
  -DQt6_DIR=C:/Qt/6.8.0/msvc2022_64/lib/cmake/Qt6
```

- If CMake says it cannot find `Qt6Config.cmake`, check the real Qt install directory first instead of assuming the aqt selector name matches the folder name
- In CI, build before running `python setup.py lint`, because module-mode `qmllint` expects generated QML type metadata

### Manual Setup (vcpkg)

If you prefer vcpkg for C++ dependencies:

```bash
# Install vcpkg
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# Build project
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

---

## 📦 Project Structure

```
local-code-ide/
├── setup.py              # Automated setup (Qt install, build, deploy)
├── version.py            # Semantic versioning helper
├── vcpkg.json            # C++ dependencies manifest
├── CMakeLists.txt        # CMake configuration
├── src/
│   ├── core/             # Core logic
│   ├── services/         # Business logic services
│   ├── adapters/         # External integrations
│   └── ui/               # UI components
├── qml/                  # Qt Quick UI files
└── build/                # Build output (gitignored)
```

---

## 🔧 Build Commands

```bash
# Full build (installs Qt if needed)
python setup.py

# Lint
python setup.py lint

# Preflight diagnostics (recommended before run/release)
python setup.py doctor

# Capture app screenshot (auto-launches if needed)
python setup.py screenshot

# Just run
python setup.py run

# Clean rebuild
rmdir /s /q build && python setup.py

# Quick build (if Qt already installed)
dev.bat

# Quick run
run.bat

# Direct screenshot script (more options)
powershell -ExecutionPolicy Bypass -File scripts/capture-localcodeide-screenshot.ps1 -LaunchIfMissing
```

`run.bat` now runs `python setup.py doctor` before launching the app. If preflight fails, launch is blocked.

---

## 🧪 Testing

### Test Suite

The test suite validates local model functionality:

```bash
# Install test dependencies (one-time)
python setup.py testdeps

# Run all tests
python setup.py test

# Run specific category
python setup.py test integration   # Model detection
python setup.py test server        # Server lifecycle
python setup.py test inference     # Chat completion
python setup.py test tools         # Tool calling

# Or use pytest directly
pytest tests/ -v

# Run with coverage
pytest tests/ --cov=src --cov-report=html
```

### Test Categories

| Category | Description | Speed |
|----------|-------------|-------|
| `integration` | Model detection, cache scanning | Fast |
| `server` | Server startup, health, shutdown | Medium |
| `inference` | Chat completion, sampling | Slow |
| `tools` | Tool calling, function execution | Slow |

### Prerequisites

- **Model in cache**: Tests use `unsloth/Qwen3.5-0.8B-GGUF` by default
- **Built application**: Run `python setup.py` first
- **Test dependencies**: Run `python setup.py testdeps`

See [tests/README.md](tests/README.md) for full documentation.

---

## 📝 Versioning

Like npm:

```bash
python version.py patch    # 0.10.1 -> 0.10.2
python version.py minor    # 0.10.1 -> 0.11.0
python version.py major    # 0.10.1 -> 1.0.0
```

This creates a git tag and pushes it, triggering CI/CD.

---

## 🔄 CI/CD

GitHub Actions workflow (`.github/workflows/ci-cd.yml`):

**On push to `main`:**
- Build on Windows Server
- Install Qt 6.8 via aqt
- Compile with CMake
- Deploy Qt DLLs
- Upload artifact (30 days)

**On release tag (`v*`):**
- All of the above +
- Build WiX installer (MSI)
- Create GitHub Release
- Attach ZIP and MSI to release

### Create a release:
```bash
python version.py patch
```

---

## 📦 WiX Installer (Windows)

The project uses WiX Toolset v5 to create Windows MSI installers.

### Files

```
installer/
├── Product.wxs           # Main WiX package definition
├── Version.wxi           # UpgradeCode and version variables
├── build-installer.ps1   # PowerShell build script
└── README.md             # Installer documentation
```

### Build the Installer

```bash
# 1. Install WiX Toolset (one-time)
dotnet tool install --global wix

# 2. Build the application first
python setup.py

# 3. Create the MSI installer
python setup.py installer
```

Output: `build/installer/LocalCodeIDE-<version>.msi`

### Manual Build

```bash
powershell -ExecutionPolicy Bypass -File installer/build-installer.ps1 -Version 0.10.1
```

### Configuration

- **UpgradeCode**: `C957FC5D-DD7C-48C4-AC7C-8900BD9AA945`
- **Architecture**: x64
- **Install location**: `Program Files\LocalCodeIDE`

### CI/CD Integration

The GitHub Actions workflow automatically builds the MSI installer when a release is published. The installer is uploaded as a release asset alongside the portable ZIP.

### Troubleshooting

**"WiX not found"**
```bash
dotnet tool install --global wix
```

**"Missing files in staging"**
Run `python setup.py` first to build and deploy Qt.

**"Duplicate GUID" errors**
Each component must have a unique GUID. Use `Guid="*"` for auto-generation or create explicit GUIDs.

---

## 🔥 Troubleshooting

### "Qt not found"
```bash
python -m aqt install-qt windows desktop 6.8.0 win64_msvc2022_64 --outputdir C:\Qt
```

Then configure CMake with:

```bash
cmake -S . -B build ^
  -DCMAKE_PREFIX_PATH=C:/Qt/6.8.0/msvc2022_64 ^
  -DQt6_DIR=C:/Qt/6.8.0/msvc2022_64/lib/cmake/Qt6
```

### "DLL not found"
```bash
C:\Qt\6.8.0\msvc2022_64\bin\windeployqt.exe build\Release\LocalCodeIDE.exe
```

### Dirty build
```bash
rmdir /s /q build
python setup.py
```

### CMake errors
```bash
rmdir /s /q build
cmake --fresh -S . -B build
```

### "App closes silently on startup"
```bash
python setup.py doctor
```

If the doctor check fails, inspect `build/doctor-startup.log` for the latest startup error output.

---

## 🧪 Testing

```bash
# Run lint (required before commit/PR)
python setup.py lint

# Run tests (if available)
ctest --test-dir build -C Release
```

---

## 📚 Technologies

- **Qt 6.8** - UI framework (QML)
- **C++20** - Language
- **CMake** - Build system
- **vcpkg** - Package manager (optional)
- **aqt** - Automated Qt installer

---

## 🤝 Contributing

1. Fork the repo
2. Create a feature branch: `git checkout -b feature/xyz`
3. Commit: `git commit -m 'Add xyz'`
4. Push: `git push origin feature/xyz`
5. Open a Pull Request

---

## 📄 License

MIT
