# 🧪 Plano de Testes - LocalCodeIDE

## Visão Geral

Este documento descreve o plano de testes para validar as funcionalidades do LocalCodeIDE relacionadas a modelos locais GGUF, servidor llama.cpp, e tool calling.

## ✅ Testes Implementados

### 1. Testes de Integração (`test_integration.py`)

**Objetivo**: Validar detecção de modelos na cache do Hugging Face.

| Teste | Descrição | Status |
|-------|-----------|--------|
| `test_hf_cache_directory_exists` | Verifica se diretório cache existe | ✅ |
| `test_hf_cache_has_models` | Lista modelos disponíveis | ✅ |
| `test_test_model_in_cache` | Verifica Qwen3.5-0.8B disponível | ✅ |
| `test_model_file_is_valid_gguf` | Valida header GGUF | ✅ |
| `test_list_all_gguf_models` | Lista todos arquivos GGUF | ✅ |
| `test_cache_has_snapshots_directory` | Verifica estrutura snapshots | ✅ |
| `test_model_has_refs_directory` | Verifica refs para branches | ✅ |
| `test_resolve_path_from_cache` | Testa resolução de caminhos | ✅ |

**Como executar**:
```bash
python setup.py test integration
```

### 2. Testes de Servidor (`test_server.py`)

**Objetivo**: Validar ciclo de vida do servidor llama.cpp.

| Teste | Descrição | Status |
|-------|-----------|--------|
| `test_server_starts_with_model` | Inicializa servidor com modelo | 📋 |
| `test_health_endpoint` | Endpoint /health retorna OK | 📋 |
| `test_server_loads_model_metadata` | Carrega metadados do modelo | 📋 |
| `test_graceful_shutdown` | Desliga gracefulmente | 📋 |
| `test_force_kill` | Pode ser forçado a parar | 📋 |
| `test_server_version` | Expõe versão do servidor | 📋 |
| `test_probe_endpoint` | Endpoint ready/probe | 📋 |

**Como executar**:
```bash
python setup.py test server
```

### 3. Testes de Inferência (`test_inference.py`)

**Objetivo**: Validar chat completion e geração de texto.

| Teste | Descrição | Status |
|-------|-----------|--------|
| `test_simple_chat_completion` | Chat básico | 📋 |
| `test_chat_with_system_prompt` | Com prompt de sistema | 📋 |
| `test_multi_turn_conversation` | Conversa multi-turno | 📋 |
| `test_streaming_response` | Resposta streaming | 📋 |
| `test_streaming_accumulates_correctly` | Acumula chunks corretamente | 📋 |
| `test_temperature_zero` | Saída determinística | 📋 |
| `test_high_temperature` | Saída criativa | 📋 |
| `test_max_tokens_limit` | Limite de tokens | 📋 |
| `test_long_context` | Contexto longo | 📋 |
| `test_context_retention` | Retém contexto | 📋 |

**Como executar**:
```bash
python setup.py test inference
```

### 4. Testes de Tool Calling (`test_tools.py`)

**Objetivo**: Validar chamada e execução de ferramentas.

| Teste | Descrição | Status |
|-------|-----------|--------|
| `test_single_tool_call` | Chamada única de ferramenta | 📋 |
| `test_tool_with_multiple_parameters` | Múltiplos parâmetros | 📋 |
| `test_parallel_tool_calls` | Múltiplas chamadas paralelas | 📋 |
| `test_different_tools_in_conversation` | Tools diferentes por turno | 📋 |
| `test_tool_result_incorporation` | Resultado incorporado | 📋 |
| `test_invalid_tool_arguments` | Argumentos inválidos | 📋 |
| `test_unknown_tool` | Ferramenta desconhecida | 📋 |
| `test_nested_tool_parameters` | Parâmetros aninhados | 📋 |

**Ferramentas de teste**:
- `calculator` - Cálculos matemáticos
- `get_weather` - Previsão do tempo (mock)
- `web_search` - Busca web (mock)

**Como executar**:
```bash
python setup.py test tools
```

## 📊 Resumo de Cobertura

| Categoria | Testes | Implementados | Pendentes |
|-----------|--------|---------------|-----------|
| Integração | 8 | 8 ✅ | 0 |
| Servidor | 7 | 7 📋 | 0 |
| Inferência | 10 | 10 📋 | 0 |
| Tool Calling | 8 | 8 📋 | 0 |
| **Total** | **33** | **33** | **0** |

Legenda:
- ✅ = Testado e passando
- 📋 = Implementado, aguardando execução completa
- ⏸️ = Pendente de implementação

## 🚀 Como Executar Todos os Testes

### Instalação (uma vez)

```bash
# Instalar dependências de teste
python setup.py testdeps

# Ou manualmente:
pip install -r requirements-test.txt
```

### Execução

```bash
# Todos os testes
python setup.py test

# Por categoria
python setup.py test integration
python setup.py test server
python setup.py test inference
python setup.py test tools

# Com coverage
python -m pytest tests/ --cov=src --cov-report=html

# Verbose
python setup.py test -v
```

## 📋 Pré-requisitos para Testes

### Modelos Necessários

O teste usa por padrão:
- **Repositório**: `unsloth/Qwen3.5-0.8B-GGUF`
- **Arquivo**: `Qwen3.5-0.8B-Q5_K_M.gguf`
- **Tamanho**: ~550 MB

**Verificar se existe**:
```bash
dir %USERPROFILE%\.cache\huggingface\hub\**\*.gguf
```

**Baixar se necessário**:
```bash
# Usando hf CLI
hf download unsloth/Qwen3.5-0.8B-GGUF Qwen3.5-0.8B-Q5_K_M.gguf

# Ou usar o próprio LocalCodeIDE para baixar
```

### Executável llama-server

Deve estar em:
```
build/Release/llama-server.exe
```

**Gerar se necessário**:
```bash
python setup.py
```

## 🔧 Configurando Testes

### Variáveis de Ambiente

```bash
# Cache customizado
set HF_HUB_CACHE=C:\path\to\cache

# URL do servidor
set TEST_SERVER_URL=http://localhost:8080

# Timeout customizado
set SERVER_START_TIMEOUT=180
```

### Editar Configuração

Arquivo: `tests/conftest.py`

```python
class TestConfig:
    TEST_MODEL_REPO = "unsloth/Qwen3.5-0.8B-GGUF"
    TEST_MODEL_FILE = "Qwen3.5-0.8B-Q5_K_M.gguf"
    SERVER_START_TIMEOUT = 120  # segundos
    INFERENCETIMEOUT = 60  # segundos
```

## 📈 Próximos Passos (Futuro)

### Fase 5: Performance

- [ ] Teste de throughput de tokens/segundo
- [ ] Teste de estresse de janela de contexto
- [ ] Teste de requisições concorrentes
- [ ] Monitoramento de uso de memória
- [ ] Benchmark de tempo de primeira resposta

### Fase 6: Integração Contínua

- [ ] GitHub Actions workflow para testes
- [ ] Testes automáticos em PRs
- [ ] Relatórios de coverage automáticos
- [ ] Badge de status no README

### Fase 7: Testes de UI (Futuro)

- [ ] Testes de componentes QML
- [ ] Testes de integração UI-backend
- [ ] Testes de fluxo do wizard

## 🐛 Solução de Problemas

### "Model not found"

```bash
# Verificar modelos na cache
python -c "from tests.conftest import TestConfig; print(TestConfig.get_test_model_path())"

# Baixar modelo
hf download unsloth/Qwen3.5-0.8B-GGUF Qwen3.5-0.8B-Q5_K_M.gguf
```

### "llama-server not found"

```bash
# Rebuild
rmdir /s /q build
python setup.py
```

### "Port already in use"

```bash
# Matar processos na porta 8080
netstat -ano | findstr :8080
taskkill /PID <PID> /F
```

### Testes falhando por timeout

Editar `tests/conftest.py`:
```python
SERVER_START_TIMEOUT = 180  # Aumentar de 120
INFERENCETIMEOUT = 120  # Aumentar de 60
```

## 📚 Referências

- [pytest Documentation](https://docs.pytest.org/)
- [llama.cpp Server Docs](https://github.com/ggerganov/llama.cpp/tree/master/examples/server)
- [Hugging Face Hub Cache](https://huggingface.co/docs/huggingface_hub/guides/manage-cache)
- [tests/README.md](tests/README.md) - Documentação completa dos testes

## 📄 Licença

MIT License - Ver arquivo LICENSE no diretório raiz.
