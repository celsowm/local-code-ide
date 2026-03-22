# LocalCodeIDE - Developer Guide

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

# Run
python setup.py run
```

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

# Just run
python setup.py run

# Clean rebuild
rmdir /s /q build && python setup.py

# Quick build (if Qt already installed)
dev.bat

# Quick run
run.bat
```

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
- Create GitHub Release
- Attach ZIP with executable

### Create a release:
```bash
python version.py patch
```

---

## 🔥 Troubleshooting

### "Qt not found"
```bash
python -m aqt install-qt windows desktop 6.8.0 win64_msvc2022_64 --outputdir C:\Qt
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

---

## 🧪 Testing

```bash
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
