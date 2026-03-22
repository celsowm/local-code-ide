#!/usr/bin/env python3
"""
Helper script for semantic versioning (like npm version)

Usage:
    python version.py major    # v1.0.0
    python version.py minor    # v0.11.0
    python version.py patch    # v0.10.2
    python version.py 0.12.0   # specific version
"""
import subprocess
import sys
import re

def get_current_version():
    """Get latest tag from git"""
    try:
        result = subprocess.run(
            ["git", "describe", "--tags", "--abbrev=0"],
            capture_output=True, text=True, check=True
        )
        tag = result.stdout.strip()
        match = re.match(r'v?(\d+)\.(\d+)\.(\d+)', tag)
        if match:
            return tuple(map(int, match.groups()))
    except subprocess.CalledProcessError:
        pass
    return (0, 10, 0)  # Default if no tags


def get_version():
    """Get current version as string"""
    version = get_current_version()
    return f"{version[0]}.{version[1]}.{version[2]}"

def bump_version(current, level):
    """Bump version based on level"""
    major, minor, patch = current
    
    if level == "major":
        return (major + 1, 0, 0)
    elif level == "minor":
        return (minor, patch + 1, 0) if major == 0 else (major, minor + 1, 0)
    elif level == "patch":
        return (major, minor, patch + 1)
    else:
        # Try to parse as version string
        match = re.match(r'v?(\d+)\.(\d+)\.(\d+)', level)
        if match:
            return tuple(map(int, match.groups()))
    
    raise ValueError(f"Invalid version level: {level}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python version.py <major|minor|patch|X.Y.Z>")
        print("Example: python version.py patch  # bumps 0.10.1 -> 0.10.2")
        sys.exit(1)
    
    level = sys.argv[1].lower()
    current = get_current_version()
    new_version = bump_version(current, level)
    tag = f"v{new_version[0]}.{new_version[1]}.{new_version[2]}"
    
    print(f"Current version: v{current[0]}.{current[1]}.{current[2]}")
    print(f"New version: {tag}")
    print()
    
    # Create tag
    subprocess.run(["git", "tag", tag], check=True)
    print(f"✓ Tag '{tag}' created")
    
    # Push tag
    result = subprocess.run(["git", "push", "origin", tag], capture_output=True, text=True)
    if result.returncode == 0:
        print(f"✓ Tag pushed to GitHub")
        print()
        print("CI/CD will now build and create a release!")
        print(f"Watch: https://github.com/celsowm/local-code-ide/actions")
    else:
        print(f"✗ Failed to push: {result.stderr}")
        sys.exit(1)

if __name__ == "__main__":
    main()
