"""
Tool calling and function execution tests.

Tests:
- Basic tool calling
- Multiple tool calls
- Tool execution and response
- Error handling in tool calls
"""
import json
import pytest
from tests.conftest import make_chat_request, TestConfig


# ============================================================================
# Tool Definitions for Testing
# ============================================================================

CALCULATOR_TOOL = {
    "type": "function",
    "function": {
        "name": "calculator",
        "description": "Perform mathematical calculations",
        "parameters": {
            "type": "object",
            "properties": {
                "expression": {
                    "type": "string",
                    "description": "Mathematical expression to evaluate (e.g., '2 + 2')"
                }
            },
            "required": ["expression"]
        }
    }
}

WEATHER_TOOL = {
    "type": "function",
    "function": {
        "name": "get_weather",
        "description": "Get current weather for a location",
        "parameters": {
            "type": "object",
            "properties": {
                "location": {
                    "type": "string",
                    "description": "City name or coordinates"
                },
                "unit": {
                    "type": "string",
                    "enum": ["celsius", "fahrenheit"],
                    "description": "Temperature unit"
                }
            },
            "required": ["location"]
        }
    }
}

SEARCH_TOOL = {
    "type": "function",
    "function": {
        "name": "web_search",
        "description": "Search the web for information",
        "parameters": {
            "type": "object",
            "properties": {
                "query": {
                    "type": "string",
                    "description": "Search query"
                },
                "limit": {
                    "type": "integer",
                    "description": "Maximum number of results",
                    "default": 5
                }
            },
            "required": ["query"]
        }
    }
}


def execute_tool_call(tool_name: str, arguments: dict) -> dict:
    """
    Execute a tool call and return result.
    
    This is a mock implementation for testing.
    In production, this would call actual APIs.
    """
    if tool_name == "calculator":
        try:
            # Safe evaluation of simple math expressions
            expression = arguments.get("expression", "")
            # Only allow safe characters
            if all(c in "0123456789+-*/.() " for c in expression):
                result = eval(expression)
                return {"result": str(result)}
            else:
                return {"error": "Invalid characters in expression"}
        except Exception as e:
            return {"error": str(e)}
    
    elif tool_name == "get_weather":
        # Mock weather response
        location = arguments.get("location", "Unknown")
        return {
            "location": location,
            "temperature": 25,
            "condition": "sunny",
            "humidity": 60
        }
    
    elif tool_name == "web_search":
        # Mock search response
        query = arguments.get("query", "")
        return {
            "query": query,
            "results": [
                {"title": f"Result 1 for {query}", "url": "https://example.com/1"},
                {"title": f"Result 2 for {query}", "url": "https://example.com/2"},
            ]
        }
    
    else:
        return {"error": f"Unknown tool: {tool_name}"}


class TestBasicToolCalling:
    """Test basic tool calling functionality."""
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.tools
    def test_single_tool_call(self, server_process, server_url: str):
        """Test model can call a single tool."""
        messages = [
            {"role": "user", "content": "What is 25 multiplied by 4?"}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=200,
            temperature=0.1,
            tools=[CALCULATOR_TOOL],
        )
        
        # Check for tool calls in response
        choice = response["choices"][0]
        
        # Model might return tool_calls in assistant message
        if "tool_calls" in choice.get("message", {}):
            tool_calls = choice["message"]["tool_calls"]
            assert len(tool_calls) > 0
            
            tool_call = tool_calls[0]
            assert "function" in tool_call
            assert tool_call["function"]["name"] == "calculator"
            
            # Parse arguments
            args = json.loads(tool_call["function"]["arguments"])
            assert "expression" in args
            
            print(f"\nTool called: calculator")
            print(f"Arguments: {args}")
            
            # Execute tool
            result = execute_tool_call("calculator", args)
            print(f"Result: {result}")
        
        else:
            # Model might answer directly without tool call
            content = choice["message"]["content"]
            print(f"\nModel answered directly: {content}")
            assert "100" in content, "Expected answer to contain 100"
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.tools
    def test_tool_with_multiple_parameters(self, server_process, server_url: str):
        """Test tool call with multiple parameters."""
        messages = [
            {"role": "user", "content": "What's the weather in São Paulo in celsius?"}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=200,
            temperature=0.1,
            tools=[WEATHER_TOOL],
        )
        
        choice = response["choices"][0]
        
        if "tool_calls" in choice.get("message", {}):
            tool_calls = choice["message"]["tool_calls"]
            assert len(tool_calls) > 0
            
            tool_call = tool_calls[0]
            args = json.loads(tool_call["function"]["arguments"])
            
            assert args.get("location") == "São Paulo"
            assert args.get("unit") == "celsius"
            
            print(f"\nWeather tool called:")
            print(f"  Location: {args['location']}")
            print(f"  Unit: {args['unit']}")
        else:
            content = choice["message"]["content"]
            print(f"\nModel answered directly: {content}")


class TestMultipleToolCalls:
    """Test multiple tool calls in single response."""
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.tools
    @pytest.mark.slow
    def test_parallel_tool_calls(self, server_process, server_url: str):
        """Test model can make multiple tool calls."""
        messages = [
            {"role": "user", "content": "Calculate 10 + 20 and also 5 * 6"}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=300,
            temperature=0.1,
            tools=[CALCULATOR_TOOL],
        )
        
        choice = response["choices"][0]
        
        if "tool_calls" in choice.get("message", {}):
            tool_calls = choice["message"]["tool_calls"]
            
            # Should have 2 calculator calls
            calculator_calls = [
                tc for tc in tool_calls 
                if tc["function"]["name"] == "calculator"
            ]
            
            print(f"\nMade {len(calculator_calls)} calculator calls")
            
            for i, tc in enumerate(calculator_calls):
                args = json.loads(tc["function"]["arguments"])
                result = execute_tool_call("calculator", args)
                print(f"  Call {i+1}: {args} = {result}")
        else:
            content = choice["message"]["content"]
            print(f"\nModel answered directly: {content}")
            # Should contain both answers
            assert "30" in content
            assert "30" in content  # 5*6=30
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.tools
    def test_different_tools_in_conversation(self, server_process, server_url: str):
        """Test using different tools across conversation turns."""
        tools = [CALCULATOR_TOOL, WEATHER_TOOL]
        
        # First turn: calculator
        messages1 = [
            {"role": "user", "content": "What is 15 + 17?"}
        ]
        
        response1 = make_chat_request(
            url=server_url,
            messages=messages1,
            max_tokens=200,
            temperature=0.1,
            tools=tools,
        )
        
        print("\nTurn 1 (Calculator):")
        if "tool_calls" in response1["choices"][0].get("message", {}):
            print("  Tool was called")
        else:
            content = response1["choices"][0]["message"]["content"]
            print(f"  Answer: {content}")
        
        # Second turn: weather
        messages2 = messages1 + [
            response1["choices"][0]["message"],
            {"role": "user", "content": "What's the weather in Rio de Janeiro?"}
        ]
        
        response2 = make_chat_request(
            url=server_url,
            messages=messages2,
            max_tokens=200,
            temperature=0.1,
            tools=tools,
        )
        
        print("\nTurn 2 (Weather):")
        if "tool_calls" in response2["choices"][0].get("message", {}):
            print("  Tool was called")
        else:
            content = response2["choices"][0]["message"]["content"]
            print(f"  Answer: {content}")


class TestToolExecution:
    """Test tool execution and response handling."""
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.tools
    def test_tool_result_incorporation(self, server_process, server_url: str):
        """Test that tool results are incorporated into conversation."""
        messages = [
            {"role": "user", "content": "Calculate 123 + 456"}
        ]
        
        # First request - get tool call
        response1 = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=200,
            temperature=0.1,
            tools=[CALCULATOR_TOOL],
        )
        
        choice1 = response1["choices"][0]
        
        if "tool_calls" not in choice1.get("message", {}):
            pytest.skip("Model didn't call tool")
        
        tool_call = choice1["message"]["tool_calls"][0]
        args = json.loads(tool_call["function"]["arguments"])
        
        # Execute tool
        tool_result = execute_tool_call("calculator", args)
        
        # Add tool result to conversation
        messages.extend([
            choice1["message"],
            {
                "role": "tool",
                "tool_call_id": tool_call["id"],
                "content": json.dumps(tool_result),
            }
        ])
        
        # Second request - get final answer
        response2 = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=100,
            temperature=0.1,
            tools=[CALCULATOR_TOOL],
        )
        
        final_answer = response2["choices"][0]["message"]["content"]
        
        assert "579" in final_answer, f"Expected 579 in answer, got: {final_answer}"
        
        print(f"\nTool execution flow:")
        print(f"  Called: calculator({args})")
        print(f"  Result: {tool_result}")
        print(f"  Final answer: {final_answer}")


class TestToolErrorHandling:
    """Test error handling in tool calls."""
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.tools
    def test_invalid_tool_arguments(self, server_process, server_url: str):
        """Test handling of invalid tool arguments."""
        messages = [
            {"role": "user", "content": "Calculate abc + xyz"}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=200,
            temperature=0.1,
            tools=[CALCULATOR_TOOL],
        )
        
        choice = response["choices"][0]
        
        if "tool_calls" in choice.get("message", {}):
            tool_call = choice["message"]["tool_calls"][0]
            args = json.loads(tool_call["function"]["arguments"])
            
            # Execute tool - should return error
            result = execute_tool_call("calculator", args)
            
            assert "error" in result, "Expected error for invalid expression"
            print(f"\nError handled correctly: {result}")
        else:
            content = choice["message"]["content"]
            print(f"\nModel handled error in response: {content}")
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.tools
    def test_unknown_tool(self, server_process, server_url: str):
        """Test handling of unknown tool names."""
        messages = [
            {"role": "user", "content": "Search for something"}
        ]
        
        # Provide a tool but model might try to call something else
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=200,
            temperature=0.1,
            tools=[CALCULATOR_TOOL],  # Only calculator available
        )
        
        # Model should either:
        # 1. Call calculator (if it thinks it's appropriate)
        # 2. Answer without tool call
        # 3. Say it can't help
        
        choice = response["choices"][0]
        content = choice["message"]["content"]
        
        print(f"\nResponse to unknown tool request: {content}")


class TestToolCallingFormats:
    """Test different tool calling formats and schemas."""
    
    @pytest.mark.server
    @pytest.mark.requires_model
    @pytest.mark.tools
    def test_nested_tool_parameters(self, server_process, server_url: str):
        """Test tool with nested parameters."""
        complex_tool = {
            "type": "function",
            "function": {
                "name": "create_event",
                "description": "Create a calendar event",
                "parameters": {
                    "type": "object",
                    "properties": {
                        "title": {"type": "string"},
                        "datetime": {
                            "type": "object",
                            "properties": {
                                "start": {"type": "string"},
                                "end": {"type": "string"}
                            },
                            "required": ["start", "end"]
                        },
                        "attendees": {
                            "type": "array",
                            "items": {"type": "string"}
                        }
                    },
                    "required": ["title", "datetime"]
                }
            }
        }
        
        messages = [
            {"role": "user", "content": "Create a meeting tomorrow from 2pm to 3pm"}
        ]
        
        response = make_chat_request(
            url=server_url,
            messages=messages,
            max_tokens=300,
            temperature=0.1,
            tools=[complex_tool],
        )
        
        choice = response["choices"][0]
        
        if "tool_calls" in choice.get("message", {}):
            tool_call = choice["message"]["tool_calls"][0]
            args = json.loads(tool_call["function"]["arguments"])
            
            assert "title" in args
            assert "datetime" in args
            
            print(f"\nComplex tool called:")
            print(f"  Title: {args.get('title')}")
            print(f"  DateTime: {args.get('datetime')}")
        else:
            content = choice["message"]["content"]
            print(f"\nModel response: {content}")
