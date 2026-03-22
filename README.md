# LocalCodeIDE

Local-first IDE with Qt Quick and local AI

[![CI/CD](https://github.com/celsowm/local-code-ide/actions/workflows/ci-cd.yml/badge.svg)](https://github.com/celsowm/local-code-ide/actions/workflows/ci-cd.yml)
[![Latest Release](https://img.shields.io/github/v/release/celsowm/local-code-ide)](https://github.com/celsowm/local-code-ide/releases)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## 🚀 Quick Start (Windows)

### Option 1: Automated Setup (Recommended)

```bash
# Install everything and build
python setup.py

# Run
python setup.py run
```

### Option 2: vcpkg (Modern, 2024 style)

If you have [vcpkg](https://vcpkg.io):

```bash
# Clone vcpkg if you don't have it
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
cd C:\vcpkg
.\bootstrap-vcpkg.bat

# In the project
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

The `vcpkg.json` defines dependencies (like `package.json` from npm).

### Option 3: Quick Scripts

```bash
dev.bat    # Build
run.bat    # Run
```

---

## 📦 Prerequisites

| Dependency | How to install |
|------------|----------------|
| Python 3.8+ | [python.org](https://python.org) |
| Visual Studio 2022+ | `winget install Microsoft.VisualStudio.2022.Community` |
| CMake | `winget install Kitware.CMake` |
| Qt 6 | **Automatic** (setup.py installs) |
| vcpkg | **Optional** (for C++ dependencies) |

---

## 🔧 What does setup.py do?

Unlike the "90s way" of Qt, the modern setup:

1. ✅ Uses **vcpkg** if available (C++ dependencies)
2. ✅ Uses **aqt** for Qt (pre-built, fast)
3. ✅ Configures CMake automatically
4. ✅ Copies DLLs with `windeployqt`
5. ✅ Copies QML modules
6. ✅ Creates `qt.conf` for plugins

**Zero manual configuration!**

---

## 🛠️ Development

### Project Structure
```
local-code-ide/
├── setup.py          # Automated setup
├── vcpkg.json        # Dependencies (like package.json)
├── CMakeLists.txt    # Build configuration
├── src/              # C++ code
├── qml/              # Qt Quick UI
└── build/            # Ignored by git
```

### Commands
```bash
# Full build
python setup.py

# Just run
python setup.py run

# Clean rebuild
rmdir /s /q build && python setup.py
```

---

## 🎯 Comparison with other ecosystems

| Node.js (npm) | Python (pip) | Rust (cargo) | C++ (this project) |
|---------------|--------------|--------------|-------------------|
| `package.json` | `requirements.txt` | `Cargo.toml` | `vcpkg.json` |
| `npm install` | `pip install` | `cargo build` | `cmake --build` |
| `npm start` | `python app.py` | `cargo run` | `python setup.py run` |

**Now it's almost as easy!**

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

---

## 📚 Technologies

- **Qt 6.8** - UI framework (QML)
- **C++20** - Language
- **CMake** - Build system
- **vcpkg** - Package manager (optional)
- **aqt** - Automated Qt installer

---

## 🤝 Contributing

1. Fork
2. `git checkout -b feature/xyz`
3. `git commit -m 'Add xyz'`
4. `git push`
5. Pull Request

---

## 🔄 CI/CD

This project uses GitHub Actions for automated builds and releases.

### What happens automatically:

**On every push to `main`:**
- ✅ Build on Windows Server
- ✅ Qt 6.8 download and installation
- ✅ CMake configuration and compilation
- ✅ Qt DLLs deployment with windeployqt
- ✅ Artifact upload (available for 30 days)

**On release (when you create a tag):**
- ✅ All of the above +
- ✅ ZIP with executable attached to release

### How to create a release (semantic versioning):

```bash
# Like npm version - use the helper script
python version.py patch    # 0.10.1 -> 0.10.2
python version.py minor    # 0.10.1 -> 0.11.0
python version.py major    # 0.10.1 -> 1.0.0

# Or manually
git tag v0.10.2
git push origin v0.10.2
```

This triggers the CI/CD to:
1. Build the project
2. Create a release with your tag
3. Attach a ZIP with the ready-to-run executable

**Useful links:**
- [Actions](https://github.com/celsowm/local-code-ide/actions) - View builds
- [Releases](https://github.com/celsowm/local-code-ide/releases) - Download versions

---

## 📄 License

MIT

---

## 🔗 Links

- **GitHub**: https://github.com/celsowm/local-code-ide
- **Author**: [@celsowm](https://github.com/celsowm)
