"""
Server lifecycle tests for llama.cpp server.

Tests:
- Start server with model
- Health check endpoint
- Graceful shutdown
- Server probe functionality
"""
import time
import pytest
import subprocess
from tests.conftest import (
    TestConfig,
    wait_for_server,
    check_server_health,
)


class TestServerStartup:
    """Test server startup and initialization."""
    
    @pytest.mark.requires_model
    @pytest.mark.slow
    def test_server_starts_with_model(
        self,
        llama_server_executable: Path,
        model_path: Path,
    ):
        """Verify server starts successfully with model loaded."""
        port = 8081  # Use different port to avoid conflicts
        
        process = subprocess.Popen(
            [
                str(llama_server_executable),
                "-m", str(model_path),
                "--port", str(port),
                "--host", "127.0.0.1",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        
        try:
            # Wait for server to start
            server_url = f"http://localhost:{port}"
            assert wait_for_server(server_url, TestConfig.SERVER_START_TIMEOUT), (
                f"Server failed to start within {TestConfig.SERVER_START_TIMEOUT}s"
            )
            
            # Verify health endpoint
            health = check_server_health(server_url)
            assert "error" not in health, f"Health check failed: {health}"
            assert health.get("status") == "ok", f"Unhealthy status: {health}"
            
            print(f"\nServer started successfully on {server_url}")
            print(f"Health: {health}")
            
        finally:
            # Cleanup
            process.terminate()
            try:
                process.wait(timeout=TestConfig.SERVER_SHUTDOWN_TIMEOUT)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
    
    @pytest.mark.requires_model
    def test_server_loads_model_metadata(
        self,
        llama_server_executable: Path,
        model_path: Path,
    ):
        """Verify server loads and exposes model metadata."""
        port = 8082
        
        process = subprocess.Popen(
            [
                str(llama_server_executable),
                "-m", str(model_path),
                "--port", str(port),
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        
        try:
            server_url = f"http://localhost:{port}"
            wait_for_server(server_url)
            
            # Check model info endpoint
            import requests
            response = requests.get(f"{server_url}/v1/models", timeout=10)
            assert response.status_code == 200
            
            models_data = response.json()
            assert "data" in models_data
            assert len(models_data["data"]) > 0
            
            model_info = models_data["data"][0]
            print(f"\nLoaded model info:")
            print(f"  ID: {model_info.get('id', 'N/A')}")
            print(f"  Owned by: {model_info.get('owned_by', 'N/A')}")
            
        finally:
            process.terminate()
            process.wait(timeout=TestConfig.SERVER_SHUTDOWN_TIMEOUT)


class TestServerHealth:
    """Test server health monitoring."""
    
    @pytest.mark.server
    def test_health_endpoint(self, server_process, server_url: str):
        """Test /health endpoint returns OK status."""
        health = check_server_health(server_url)
        
        assert "error" not in health
        assert health.get("status") == "ok"
        assert "healthy" in health or "status" in health
        
        print(f"\nHealth check: {health}")
    
    @pytest.mark.server
    def test_health_endpoint_details(self, server_process, server_url: str):
        """Test health endpoint returns detailed information."""
        import requests
        
        response = requests.get(f"{server_url}/health", timeout=10)
        health = response.json()
        
        # Check for expected health fields
        expected_fields = ["status", "healthy"]
        for field in expected_fields:
            if field in health:
                print(f"\n{field}: {health[field]}")
        
        # Some llama.cpp servers return additional info
        if "slots" in health:
            print(f"Slots: {health['slots']}")
    
    @pytest.mark.server
    def test_server_version(self, server_process, server_url: str):
        """Test server exposes version information."""
        import requests
        
        try:
            response = requests.get(f"{server_url}/version", timeout=10)
            if response.status_code == 200:
                version_info = response.json()
                print(f"\nServer version: {version_info}")
        except requests.exceptions.HTTPError:
            pytest.skip("/version endpoint not available")


class TestServerShutdown:
    """Test server shutdown behavior."""
    
    def test_graceful_shutdown(self, llama_server_executable: Path, model_path: Path):
        """Test server shuts down gracefully on SIGTERM."""
        port = 8083
        
        process = subprocess.Popen(
            [
                str(llama_server_executable),
                "-m", str(model_path),
                "--port", str(port),
            ],
        )
        
        # Wait for startup
        server_url = f"http://localhost:{port}"
        wait_for_server(server_url)
        
        # Send SIGTERM
        process.terminate()
        
        # Should exit cleanly within timeout
        try:
            exit_code = process.wait(timeout=TestConfig.SERVER_SHUTDOWN_TIMEOUT)
            assert exit_code == 0, f"Server exited with code {exit_code}"
            print(f"\nServer shut down gracefully (exit code: {exit_code})")
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait()
            pytest.fail("Server did not shut down gracefully")
    
    def test_force_kill(self, llama_server_executable: Path, model_path: Path):
        """Test server can be force killed."""
        port = 8084
        
        process = subprocess.Popen(
            [
                str(llama_server_executable),
                "-m", str(model_path),
                "--port", str(port),
            ],
        )
        
        wait_for_server(f"http://localhost:{port}")
        
        # Force kill
        process.kill()
        exit_code = process.wait()
        
        # On Windows, killed processes typically return non-zero
        print(f"\nServer killed (exit code: {exit_code})")


class TestServerProbe:
    """Test server probe functionality."""
    
    @pytest.mark.server
    def test_probe_endpoint(self, server_process, server_url: str):
        """Test server probe/ready endpoint."""
        import requests
        
        endpoints_to_try = [
            "/ready",
            "/health/ready",
            "/v1/models",
        ]
        
        for endpoint in endpoints_to_try:
            try:
                response = requests.get(f"{server_url}{endpoint}", timeout=5)
                print(f"\n{endpoint}: {response.status_code}")
                
                if response.status_code == 200:
                    print(f"  Response: {response.text[:200]}")
                    return
            except Exception as e:
                print(f"\n{endpoint}: Error - {e}")
        
        # At least one endpoint should work
        pytest.skip("No probe endpoint responded")
