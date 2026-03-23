# LocalCodeIDE Test Suite

## 📋 Overview

This test suite validates the LocalCodeIDE application's local model functionality, including:

- **Model Detection**: Scanning Hugging Face cache for GGUF models
- **Server Management**: Starting, stopping, and health monitoring llama.cpp server
- **Inference**: Chat completion, streaming, and sampling parameters
- **Tool Calling**: Function/tool execution and response handling

## 🚀 Quick Start

### Prerequisites

```bash
# Install test dependencies
pip install pytest requests pytest-cov pytest-xdist

# Build the application
python setup.py

# Ensure you have a model in cache
# The test suite uses Qwen3.5-0.8B-GGUF by default
```

### Run All Tests

```bash
python run_tests.py
```

### Run Specific Categories

```bash
# Integration tests (model detection)
python run_tests.py --category integration

# Server tests (startup, health, shutdown)
python run_tests.py --category server

# Inference tests (chat completion)
python run_tests.py --category inference

# Tool calling tests
python run_tests.py --category tools

# Quick smoke test (excludes slow tests)
python run_tests.py --smoke
```

### Run with Coverage

```bash
python run_tests.py --coverage
# Report: build/test-coverage/index.html
```

## 📁 Test Structure

```
tests/
├── __init__.py           # Package initialization
├── conftest.py           # Pytest fixtures and configuration
├── test_integration.py   # Model detection and cache tests
├── test_server.py        # Server lifecycle tests
├── test_inference.py     # Chat completion tests
└── test_tools.py         # Tool calling tests
```

## 🧪 Test Categories

### Integration Tests (`test_integration.py`)

Tests model detection and cache scanning:

- `test_hf_cache_directory_exists` - Verify HF cache exists
- `test_test_model_in_cache` - Check Qwen3.5-0.8B availability
- `test_model_file_is_valid_gguf` - Validate GGUF header
- `test_list_all_gguf_models` - List all cached models

### Server Tests (`test_server.py`)

Tests llama.cpp server management:

- `test_server_starts_with_model` - Server startup
- `test_health_endpoint` - Health check
- `test_graceful_shutdown` - Clean shutdown
- `test_server_loads_model_metadata` - Model info endpoint

### Inference Tests (`test_inference.py`)

Tests chat completion and generation:

- `test_simple_chat_completion` - Basic inference
- `test_streaming_response` - Streaming output
- `test_temperature_zero` - Deterministic output
- `test_context_retention` - Multi-turn conversation

### Tool Calling Tests (`test_tools.py`)

Tests function/tool execution:

- `test_single_tool_call` - Basic tool invocation
- `test_parallel_tool_calls` - Multiple tools
- `test_tool_result_incorporation` - Tool response handling
- `test_tool_error_handling` - Error scenarios

## 🔧 Configuration

### Test Configuration

Edit `tests/conftest.py` to customize:

```python
class TestConfig:
    TEST_MODEL_REPO = "unsloth/Qwen3.5-0.8B-GGUF"
    TEST_MODEL_FILE = "Qwen3.5-0.8B-Q5_K_M.gguf"
    SERVER_START_TIMEOUT = 120  # seconds
    INFERENCETIMEOUT = 60  # seconds
```

### Environment Variables

```bash
# Custom HF cache location
export HF_HUB_CACHE=/path/to/cache

# Custom server URL
export TEST_SERVER_URL=http://localhost:8080
```

## 📊 Test Markers

Use pytest markers to filter tests:

```bash
# Run only fast tests
pytest -m "not slow"

# Run only model-requiring tests
pytest -m requires_model

# Run integration and server tests
pytest -m "integration or server"
```

Available markers:
- `integration` - Model detection tests
- `server` - Server lifecycle tests
- `inference` - Chat completion tests
- `tools` - Tool calling tests
- `performance` - Performance tests
- `requires_model` - Requires model in cache
- `slow` - Tests taking >10 seconds

## 🐛 Troubleshooting

### "Model not found in cache"

Download the test model:

```bash
# Using hf CLI
hf download unsloth/Qwen3.5-0.8B-GGUF Qwen3.5-0.8B-Q5_K_M.gguf

# Or use the LocalCodeIDE app to download
```

### "llama-server not found"

Build the application first:

```bash
python setup.py
```

### "Server failed to start"

Check if port 8080 is available:

```bash
netstat -ano | findstr :8080
```

Use different port:

```bash
# Edit tests/conftest.py
DEFAULT_SERVER_URL = "http://localhost:8081"
```

### Tests timing out

Increase timeouts in `tests/conftest.py`:

```python
SERVER_START_TIMEOUT = 180  # Increase from 120
INFERENCETIMEOUT = 120  # Increase from 60
```

## 📈 Coverage Report

Generate HTML coverage report:

```bash
python run_tests.py --coverage
start build/test-coverage/index.html  # Windows
open build/test-coverage/index.html   # macOS/Linux
```

## 🎯 Test Plan

### Phase 1: Basic Integration ✅

- [x] Detect models in HF cache
- [x] Validate GGUF files
- [x] Test cache structure

### Phase 2: Server Lifecycle ✅

- [x] Start server with model
- [x] Health check endpoint
- [x] Graceful shutdown

### Phase 3: Inference ✅

- [x] Basic chat completion
- [x] Streaming responses
- [x] Temperature control
- [x] Context retention

### Phase 4: Tool Calling ✅

- [x] Single tool call
- [x] Multiple tool calls
- [x] Tool execution
- [x] Error handling

### Phase 5: Performance (Future)

- [ ] Token throughput
- [ ] Context window stress test
- [ ] Concurrent requests
- [ ] Memory usage

## 📝 Writing New Tests

### Example Test

```python
import pytest
from tests.conftest import make_chat_request

@pytest.mark.server
@pytest.mark.requires_model
def test_my_feature(server_process, server_url: str):
    """Test my new feature."""
    messages = [{"role": "user", "content": "Hello"}]
    
    response = make_chat_request(
        url=server_url,
        messages=messages,
        max_tokens=100,
    )
    
    assert "choices" in response
    assert len(response["choices"]) > 0
```

### Best Practices

1. **Use fixtures**: `server_process`, `model_path`, `server_url`
2. **Add markers**: `@pytest.mark.server`, `@pytest.mark.requires_model`
3. **Print output**: Use `print()` for debugging (visible with `-v`)
4. **Clean up**: Fixtures handle cleanup automatically
5. **Timeout**: Set reasonable timeouts for network requests

## 🔗 Related Documentation

- [AGENTS.md](AGENTS.md) - Developer guide
- [README.md](README.md) - User guide
- [pytest documentation](https://docs.pytest.org/)

## 📄 License

MIT License - See project root for details
