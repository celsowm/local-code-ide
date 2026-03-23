"""
Test configuration and fixtures for LocalCodeIDE tests.
"""
import os
import time
import shutil
import subprocess
import tempfile
import pytest
from pathlib import Path
from typing import Optional, Generator


# ============================================================================
# Configuration
# ============================================================================

class TestConfig:
    """Test configuration constants."""
    
    # Model configuration
    TEST_MODEL_REPO = "unsloth/Qwen3.5-0.8B-GGUF"
    TEST_MODEL_FILE = "Qwen3.5-0.8B-Q5_K_M.gguf"
    TEST_MODEL_QUANT = "Q5_K_M"
    
    # Server configuration
    DEFAULT_SERVER_URL = "http://localhost:8080"
    SERVER_START_TIMEOUT = 120  # seconds
    SERVER_HEALTH_TIMEOUT = 30  # seconds
    SERVER_SHUTDOWN_TIMEOUT = 10  # seconds
    
    # Test timeouts
    INFERENCETIMEOUT = 60  # seconds for inference requests
    TOOL_CALL_TIMEOUT = 30  # seconds for tool execution
    
    # Paths
    @staticmethod
    def get_hf_cache_dir() -> Path:
        """Get Hugging Face cache directory."""
        env_cache = os.environ.get("HF_HUB_CACHE")
        if env_cache:
            return Path(env_cache)
        
        env_home = os.environ.get("HF_HOME")
        if env_home:
            return Path(env_home) / "hub"
        
        return Path.home() / ".cache" / "huggingface" / "hub"
    
    @staticmethod
    def get_test_model_path() -> Optional[Path]:
        """Get path to test model if available in cache."""
        hf_cache = TestConfig.get_hf_cache_dir()
        repo_dir = hf_cache / f"models--{TestConfig.TEST_MODEL_REPO.replace('/', '--')}"
        
        if not repo_dir.exists():
            return None
        
        # Search for the model file in snapshots
        snapshots_dir = repo_dir / "snapshots"
        if not snapshots_dir.exists():
            return None
        
        for snapshot in snapshots_dir.iterdir():
            if snapshot.is_dir():
                model_path = snapshot / TestConfig.TEST_MODEL_FILE
                if model_path.exists():
                    return model_path
        
        return None
    
    @staticmethod
    def get_llama_server_executable() -> Optional[Path]:
        """Find llama-server executable."""
        # Check common locations
        candidates = [
            Path(__file__).parent.parent / "build" / "Release" / "llama-server.exe",
            Path(__file__).parent.parent / "build" / "llama-server.exe",
            Path(__file__).parent.parent / "bin" / "llama-server.exe",
            shutil.which("llama-server"),
        ]
        
        for candidate in candidates:
            if candidate and Path(candidate).exists():
                return Path(candidate)
        
        return None


# ============================================================================
# Fixtures
# ============================================================================

@pytest.fixture(scope="session")
def test_config() -> TestConfig:
    """Provide test configuration."""
    return TestConfig()


@pytest.fixture(scope="session")
def model_path(test_config: TestConfig) -> Optional[Path]:
    """
    Get path to test model.
    
    Skips test if model is not available in cache.
    """
    path = test_config.get_test_model_path()
    if path is None:
        pytest.skip(f"Model {test_config.TEST_MODEL_REPO}/{test_config.TEST_MODEL_FILE} not found in cache")
    return path


@pytest.fixture(scope="session")
def llama_server_executable(test_config: TestConfig) -> Path:
    """
    Get llama-server executable path.
    
    Skips test if executable is not found.
    """
    exe = test_config.get_llama_server_executable()
    if exe is None:
        pytest.skip("llama-server executable not found. Build project first.")
    return exe


@pytest.fixture
def server_process(
    llama_server_executable: Path,
    model_path: Path,
    request: pytest.FixtureRequest
) -> Generator[subprocess.Popen, None, None]:
    """
    Start llama-server process for testing.
    
    Automatically stops server after test completes.
    """
    port = getattr(request, "param", 8080)
    
    # Start server
    process = subprocess.Popen(
        [
            str(llama_server_executable),
            "-m", str(model_path),
            "--port", str(port),
            "--host", "0.0.0.0",
            "-c", "2048",  # context size
            "-t", str(os.cpu_count() or 4),  # threads
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    
    # Wait for server to start
    start_time = time.time()
    while time.time() - start_time < TestConfig.SERVER_START_TIMEOUT:
        try:
            import requests
            response = requests.get(f"http://localhost:{port}/health", timeout=2)
            if response.status_code == 200:
                break
        except Exception:
            time.sleep(1)
    else:
        process.terminate()
        raise RuntimeError(f"Server failed to start within {TestConfig.SERVER_START_TIMEOUT}s")
    
    yield process
    
    # Cleanup: stop server
    try:
        process.terminate()
        process.wait(timeout=TestConfig.SERVER_SHUTDOWN_TIMEOUT)
    except subprocess.TimeoutExpired:
        process.kill()
        process.wait()


@pytest.fixture
def server_url(request: pytest.FixtureRequest) -> str:
    """Get server URL with configurable port."""
    port = getattr(request, "param", 8080)
    return f"http://localhost:{port}"


@pytest.fixture
def temp_download_dir() -> Generator[Path, None, None]:
    """Create temporary directory for download tests."""
    temp_dir = tempfile.mkdtemp(prefix="localcodeide-test-")
    yield Path(temp_dir)
    shutil.rmtree(temp_dir, ignore_errors=True)


# ============================================================================
# Helper Functions
# ============================================================================

def wait_for_server(url: str, timeout: float = 30.0) -> bool:
    """Wait for server to become healthy."""
    import requests
    
    start_time = time.time()
    while time.time() - start_time < timeout:
        try:
            response = requests.get(f"{url}/health", timeout=2)
            if response.status_code == 200:
                return True
        except Exception:
            time.sleep(0.5)
    
    return False


def check_server_health(url: str) -> dict:
    """Check server health and return status."""
    import requests
    
    try:
        response = requests.get(f"{url}/health", timeout=5)
        response.raise_for_status()
        return response.json()
    except Exception as e:
        return {"error": str(e)}


def make_chat_request(
    url: str,
    messages: list[dict],
    model: str = "default",
    temperature: float = 0.7,
    max_tokens: int = 256,
    stream: bool = False,
    tools: Optional[list[dict]] = None,
) -> dict:
    """Make chat completion request to server."""
    import requests
    
    payload = {
        "messages": messages,
        "temperature": temperature,
        "max_tokens": max_tokens,
        "stream": stream,
    }
    
    if tools:
        payload["tools"] = tools
    
    response = requests.post(
        f"{url}/v1/chat/completions",
        json=payload,
        timeout=TestConfig.INFERENCETIMEOUT,
    )
    response.raise_for_status()
    
    if stream:
        return response  # Return response object for streaming
    return response.json()
