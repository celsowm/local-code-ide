#!/usr/bin/env python3
"""
LocalCodeIDE Test Runner

Run tests with different configurations:

    # Run all tests
    python run_tests.py

    # Run specific category
    python run_tests.py --category integration
    python run_tests.py --category server
    python run_tests.py --category inference
    python run_tests.py --category tools

    # Run with coverage
    python run_tests.py --coverage

    # Run quick smoke test
    python run_tests.py --smoke

    # Run verbose
    python run_tests.py -v
"""
import sys
import subprocess
import argparse
from pathlib import Path


def check_prerequisites():
    """Check if test prerequisites are met."""
    errors = []
    
    # Check Python version
    if sys.version_info < (3, 8):
        errors.append("Python 3.8+ required")
    
    # Check pytest
    try:
        import pytest
    except ImportError:
        errors.append("pytest not installed. Run: pip install pytest requests")
    
    # Check requests library
    try:
        import requests
    except ImportError:
        errors.append("requests not installed. Run: pip install requests")
    
    # Check if build exists
    build_dir = Path(__file__).parent / "build" / "Release"
    if not build_dir.exists():
        print("⚠️  Warning: Build directory not found. Run 'python setup.py' first.")
    
    # Check for models in cache
    hf_cache = Path.home() / ".cache" / "huggingface" / "hub"
    if not hf_cache.exists():
        print("⚠️  Warning: Hugging Face cache not found. Tests may be skipped.")
    else:
        gguf_files = list(hf_cache.rglob("*.gguf"))
        if not gguf_files:
            print("⚠️  Warning: No GGUF models in cache. Download tests will be skipped.")
        else:
            print(f"✓ Found {len(gguf_files)} GGUF file(s) in cache")
    
    if errors:
        print("❌ Prerequisites check failed:")
        for error in errors:
            print(f"  - {error}")
        return False
    
    print("✓ Prerequisites check passed")
    return True


def run_tests(args):
    """Run pytest with specified arguments."""
    cmd = [sys.executable, "-m", "pytest"]
    
    # Add test path
    test_dir = Path(__file__).parent / "tests"
    cmd.append(str(test_dir))
    
    # Add verbosity
    if args.verbose:
        cmd.append("-v")
    else:
        cmd.append("-v")  # Always verbose by default
    
    # Add markers
    if args.smoke:
        cmd.extend(["-m", "not slow"])
    elif args.category:
        cmd.extend(["-m", args.category])
    
    # Add coverage
    if args.coverage:
        cmd.extend([
            "--cov=src",
            "--cov=tests",
            "--cov-report=term-missing",
            "--cov-report=html:build/test-coverage"
        ])
    
    # Add custom options
    if args.capture:
        cmd.append(f"--capture={args.capture}")
    
    if args.keyword:
        cmd.extend(["-k", args.keyword])
    
    # Add parallel execution if pytest-xdist is available
    try:
        import xdist  # noqa: F401
        cmd.extend(["-n", "auto"])
        print("✓ Running tests in parallel")
    except ImportError:
        pass
    
    print(f"\n🧪 Running tests: {' '.join(cmd)}\n")
    
    result = subprocess.run(cmd)
    return result.returncode


def main():
    parser = argparse.ArgumentParser(description="LocalCodeIDE Test Runner")
    
    parser.add_argument(
        "--category", "-c",
        choices=["integration", "server", "inference", "tools", "performance"],
        help="Run specific test category"
    )
    
    parser.add_argument(
        "--smoke",
        action="store_true",
        help="Run quick smoke tests (exclude slow tests)"
    )
    
    parser.add_argument(
        "--coverage",
        action="store_true",
        help="Generate coverage report"
    )
    
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        default=True,
        help="Verbose output"
    )
    
    parser.add_argument(
        "--capture",
        choices=["no", "fd", "sys", "tee-sys"],
        default="no",
        help="Output capture mode"
    )
    
    parser.add_argument(
        "--keyword", "-k",
        help="Run tests matching keyword expression"
    )
    
    parser.add_argument(
        "--check-only",
        action="store_true",
        help="Only check prerequisites, don't run tests"
    )
    
    args = parser.parse_args()
    
    # Check prerequisites
    if not check_prerequisites():
        return 1
    
    if args.check_only:
        return 0
    
    # Run tests
    return run_tests(args)


if __name__ == "__main__":
    sys.exit(main())
