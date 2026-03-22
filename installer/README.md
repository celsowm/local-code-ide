# WiX Installer for LocalCodeIDE

This directory contains the WiX Toolset configuration for building a Windows MSI installer for LocalCodeIDE.

## Files

```
installer/
├── Product.wxs           # Main WiX package definition
├── Version.wxi           # UpgradeCode and version variables
├── build-installer.ps1   # PowerShell build script
└── README.md             # This file
```

## Prerequisites

### 1. WiX Toolset

```bash
dotnet tool install --global wix
```

### 2. Build the Application

```bash
python setup.py
```

## Building the Installer

### Option 1: Using setup.py (Recommended)

```bash
python setup.py installer
```

### Option 2: Using PowerShell script directly

```bash
powershell -ExecutionPolicy Bypass -File installer/build-installer.ps1 -Version 0.10.1
```

## Output

The MSI installer is created at:

```
build/installer/LocalCodeIDE-<version>.msi
```

## Current Configuration

- **UpgradeCode**: `C957FC5D-DD7C-48C4-AC7C-8900BD9AA945`
- **Repository**: https://github.com/celsowm/local-code-ide
- **Current version**: 0.10.1

## CI/CD

The GitHub Actions workflow automatically builds the MSI installer when a release is published:

1. Create a tag: `python version.py patch`
2. Publish the release on GitHub
3. The workflow will:
   - Build the application
   - Deploy Qt
   - Build the MSI installer
   - Attach the MSI to the release

## Troubleshooting

### "WiX not found"

```bash
dotnet tool install --global wix
```

### "Missing files in staging"

Make sure to run `python setup.py` first:

```bash
python setup.py
```

### "Build failed"

Check the full log:

```bash
powershell -ExecutionPolicy Bypass -File installer/build-installer.ps1 -Version 0.10.1 -Verbose
```

## References

- [WiX Toolset Documentation](https://wixtoolset.org/documentation/)
- [WiX Toolset GitHub](https://github.com/wixtoolset/wix)
