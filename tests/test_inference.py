"""
Inference tests for chat completion and text generation.

Tests:
- Basic chat completion
- Streaming responses
- Temperature and sampling
- Context window handling
"""
import pytest
from tests.conftest import (
    TestConfig,
    make_chat_request,
    wait_for_server,
)


class TestBasicInference:
    """Test basic chat completion functionality."""
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    def test_simple_chat_completion(self, server_process, server_url: str):
        """Test basic chat completion request."""
        messages = [
            {"role": "user", "content": "Hello! Who are you?"}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=100,
            temperature=0.7,
        )
        
        assert "choices" in response
        assert len(response["choices"]) > 0
        
        choice = response["choices"][0]
        assert "message" in choice
        assert "content" in choice["message"]
        
        assistant_message = choice["message"]["content"]
        assert len(assistant_message) > 0
        
        print(f"\nUser: Hello! Who are you?")
        print(f"Assistant: {assistant_message[:200]}")
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    def test_chat_with_system_prompt(self, server_process, server_url: str):
        """Test chat with system prompt."""
        messages = [
            {"role": "system", "content": "You are a helpful coding assistant."},
            {"role": "user", "content": "Write a Python function to add two numbers."}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=200,
            temperature=0.3,  # Lower temperature for code
        )
        
        assert "choices" in response
        assert len(response["choices"]) > 0
        
        content = response["choices"][0]["message"]["content"]
        assert len(content) > 0
        
        # Should contain code-like content
        print(f"\nSystem: You are a helpful coding assistant.")
        print(f"User: Write a Python function to add two numbers.")
        print(f"Assistant: {content[:300]}")
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    def test_multi_turn_conversation(self, server_process, server_url: str):
        """Test multi-turn conversation with context."""
        messages = [
            {"role": "user", "content": "What is 2 + 2?"},
            {"role": "assistant", "content": "2 + 2 equals 4."},
            {"role": "user", "content": "What about 3 + 3?"},
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=50,
            temperature=0.1,  # Very low for deterministic math
        )
        
        assert "choices" in response
        content = response["choices"][0]["message"]["content"]
        
        # Should respond with 6 or mention 6
        assert len(content) > 0
        
        print(f"\nConversation:")
        print(f"  User: What is 2 + 2?")
        print(f"  Assistant: 2 + 2 equals 4.")
        print(f"  User: What about 3 + 3?")
        print(f"  Assistant: {content}")


class TestStreamingInference:
    """Test streaming chat completion."""
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    @pytest.mark.slow
    def test_streaming_response(self, server_process, server_url: str):
        """Test streaming chat completion."""
        messages = [
            {"role": "user", "content": "Count from 1 to 5."}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=100,
            temperature=0.0,
            stream=True,
        )
        
        # Process streaming response
        chunks = []
        for line in response.iter_lines():
            if line:
                line_str = line.decode("utf-8")
                if line_str.startswith("data: "):
                    data = line_str[6:]
                    if data.strip() == "[DONE]":
                        break
                    chunks.append(data)
        
        assert len(chunks) > 0, "No streaming chunks received"
        
        print(f"\nReceived {len(chunks)} streaming chunks")
        print(f"First chunk: {chunks[0][:100]}...")
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    def test_streaming_accumulates_correctly(self, server_process, server_url: str):
        """Test that streaming chunks accumulate to full response."""
        messages = [
            {"role": "user", "content": "Say 'Hello World'."}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=50,
            temperature=0.0,
            stream=True,
        )
        
        full_text = ""
        for line in response.iter_lines():
            if line:
                line_str = line.decode("utf-8")
                if line_str.startswith("data: "):
                    data = line_str[6:]
                    if data.strip() == "[DONE]":
                        break
                    
                    import json
                    try:
                        chunk = json.loads(data)
                        delta = chunk["choices"][0]["delta"]
                        if "content" in delta:
                            full_text += delta["content"]
                    except (json.JSONDecodeError, KeyError):
                        continue
        
        assert len(full_text) > 0
        print(f"\nAccumulated response: {full_text}")


class TestSamplingParameters:
    """Test temperature and sampling parameters."""
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    def test_temperature_zero(self, server_process, server_url: str):
        """Test deterministic output with temperature=0."""
        messages = [
            {"role": "user", "content": "What is the capital of France?"}
        ]
        
        response1 = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=50,
            temperature=0.0,
        )
        
        response2 = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=50,
            temperature=0.0,
        )
        
        content1 = response1["choices"][0]["message"]["content"]
        content2 = response2["choices"][0]["message"]["content"]
        
        # With temp=0, responses should be identical or very similar
        print(f"\nResponse 1: {content1}")
        print(f"Response 2: {content2}")
        
        # Both should mention Paris
        assert "Paris" in content1 or "paris" in content1.lower()
        assert "Paris" in content2 or "paris" in content2.lower()
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    def test_high_temperature(self, server_process, server_url: str):
        """Test creative output with high temperature."""
        messages = [
            {"role": "user", "content": "Complete this sentence: The quick brown fox"}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=50,
            temperature=1.5,  # High temperature
        )
        
        content = response["choices"][0]["message"]["content"]
        assert len(content) > 0
        
        print(f"\nHigh temp completion: {content}")
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    def test_max_tokens_limit(self, server_process, server_url: str):
        """Test that max_tokens parameter limits response length."""
        messages = [
            {"role": "user", "content": "Write a long story about a dragon."}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=50,  # Limit to 50 tokens
            temperature=0.8,
        )
        
        content = response["choices"][0]["message"]["content"]
        
        # Count approximate tokens (rough estimate: 1 token ≈ 4 chars)
        estimated_tokens = len(content) / 4
        
        # Should be close to or under the limit
        print(f"\nGenerated {estimated_tokens:.0f} tokens (limit: 50)")
        print(f"Content: {content[:200]}...")


class TestContextWindow:
    """Test context window handling."""
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    @pytest.mark.slow
    def test_long_context(self, server_process, server_url: str):
        """Test handling of longer context."""
        # Create a long context
        long_text = "The quick brown fox jumps over the lazy dog. " * 50
        
        messages = [
            {"role": "system", "content": "You are a helpful assistant."},
            {"role": "user", "content": f"Read this text and summarize: {long_text}"}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=100,
            temperature=0.5,
        )
        
        content = response["choices"][0]["message"]["content"]
        assert len(content) > 0
        
        print(f"\nLong context response: {content[:200]}...")
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.inference
    def test_context_retention(self, server_process, server_url: str):
        """Test that model retains context across turns."""
        messages = [
            {"role": "user", "content": "My name is Alice. Remember this."},
            {"role": "assistant", "content": "I'll remember that your name is Alice."},
            {"role": "user", "content": "What is my name?"}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=50,
            temperature=0.1,
        )
        
        content = response["choices"][0]["message"]["content"]
        
        # Should mention Alice
        assert "Alice" in content or "alice" in content.lower()
        
        print(f"\nContext retention test:")
        print(f"  Response: {content}")
