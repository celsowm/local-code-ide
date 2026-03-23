"""
Integration tests for model detection and cache scanning.

Tests:
- Detect GGUF models in Hugging Face cache
- Validate cache directory structure
- Test candidate path resolution
"""
import pytest
from pathlib import Path
from tests.conftest import TestConfig


class TestModelDetection:
    """Test model detection in local cache."""
    
    def test_hf_cache_directory_exists(self, test_config: TestConfig):
        """Verify Hugging Face cache directory exists."""
        cache_dir = test_config.get_hf_cache_dir()
        assert cache_dir.exists(), f"HF cache directory not found: {cache_dir}"
        assert cache_dir.is_dir(), f"HF cache path is not a directory: {cache_dir}"
    
    def test_hf_cache_has_models(self, test_config: TestConfig):
        """Verify cache contains model directories."""
        cache_dir = test_config.get_hf_cache_dir()
        model_dirs = [d for d in cache_dir.glob("models--*") if d.is_dir()]
        
        assert len(model_dirs) > 0, "No model directories found in HF cache"
        
        print(f"\nFound {len(model_dirs)} model(s) in cache:")
        for model_dir in model_dirs[:5]:  # Show first 5
            print(f"  - {model_dir.name}")
    
    def test_test_model_in_cache(self, test_config: TestConfig):
        """Verify test model (Qwen3.5-0.8B) is available in cache."""
        model_path = test_config.get_test_model_path()
        
        assert model_path is not None, (
            f"Test model {test_config.TEST_MODEL_REPO}/{test_config.TEST_MODEL_FILE} "
            "not found in cache. Download it first using the app or hf CLI."
        )
        assert model_path.exists(), f"Model file does not exist: {model_path}"
        assert model_path.stat().st_size > 0, f"Model file is empty: {model_path}"
        
        size_gb = model_path.stat().st_size / (1024 ** 3)
        print(f"\nTest model found: {model_path}")
        print(f"Size: {size_gb:.2f} GB")
    
    def test_model_file_is_valid_gguf(self, model_path: Path):
        """Verify model file has valid GGUF header."""
        # GGUF files start with "GGUF" magic number (0x46554747)
        with open(model_path, "rb") as f:
            magic = f.read(4)
        
        assert magic == b"GGUF", f"Invalid GGUF header: {magic!r}"
        print(f"\nValid GGUF file: {model_path.name}")
    
    def test_list_all_gguf_models(self, test_config: TestConfig):
        """List all GGUF model files in cache."""
        cache_dir = test_config.get_hf_cache_dir()
        gguf_files = []
        
        for snapshot_dir in cache_dir.rglob("snapshots"):
            for gguf_file in snapshot_dir.rglob("*.gguf"):
                if gguf_file.is_file() and not gguf_file.name.endswith(".part"):
                    gguf_files.append(gguf_file)
        
        assert len(gguf_files) > 0, "No GGUF files found in cache"
        
        print(f"\nFound {len(gguf_files)} GGUF file(s):")
        for gguf_file in gguf_files:
            size_gb = gguf_file.stat().st_size / (1024 ** 3)
            print(f"  - {gguf_file.relative_to(cache_dir)} ({size_gb:.2f} GB)")


class TestCacheStructure:
    """Test Hugging Face cache directory structure."""
    
    def test_cache_has_snapshots_directory(self, test_config: TestConfig):
        """Verify cache uses snapshots directory structure."""
        cache_dir = test_config.get_hf_cache_dir()
        
        # Find a model with snapshots
        for model_dir in cache_dir.glob("models--*"):
            snapshots_dir = model_dir / "snapshots"
            if snapshots_dir.exists():
                assert snapshots_dir.is_dir()
                print(f"\nFound snapshots: {snapshots_dir}")
                
                # List snapshot subdirectories
                snapshots = [s for s in snapshots_dir.iterdir() if s.is_dir()]
                print(f"  {len(snapshots)} snapshot(s)")
                return
        
        pytest.skip("No models with snapshots structure found")
    
    def test_model_has_refs_directory(self, test_config: TestConfig):
        """Verify model has refs directory for branch tracking."""
        cache_dir = test_config.get_hf_cache_dir()
        
        for model_dir in cache_dir.glob("models--*"):
            refs_dir = model_dir / "refs"
            if refs_dir.exists():
                assert refs_dir.is_dir()
                
                # Check for main ref
                main_ref = refs_dir / "main"
                if main_ref.exists():
                    commit_hash = main_ref.read_text().strip()
                    print(f"\nModel {model_dir.name} tracks main: {commit_hash[:7]}")
                return
        
        print("\nNote: Not all models use refs directory")


class TestCandidatePathResolution:
    """Test candidate path resolution logic."""
    
    def test_resolve_path_from_cache(self, test_config: TestConfig, model_path: Path):
        """Test resolving model path from cache structure."""
        # Find the repo directory
        cache_dir = test_config.get_hf_cache_dir()
        repo_name = f"models--{test_config.TEST_MODEL_REPO.replace('/', '--')}"
        repo_dir = cache_dir / repo_name
        
        assert repo_dir.exists(), f"Repo directory not found: {repo_dir}"
        
        # Verify model exists in snapshot
        snapshots_dir = repo_dir / "snapshots"
        assert snapshots_dir.exists()
        
        # Find which snapshot contains the model
        found = False
        for snapshot in snapshots_dir.iterdir():
            if snapshot.is_dir():
                candidate = snapshot / test_config.TEST_MODEL_FILE
                if candidate.exists():
                    found = True
                    print(f"\nFound in snapshot: {snapshot.name}")
                    print(f"  Full path: {candidate}")
                    break
        
        assert found, f"Model not found in any snapshot: {test_config.TEST_MODEL_FILE}"
