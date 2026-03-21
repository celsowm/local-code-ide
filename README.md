# LocalCodeIDE v3.3

Scaffold C++/Qt Quick de uma IDE local-first com workbench inspirado no VS Code.

## Destaques desta versão

- Explorer em árvore com pastas colapsáveis
- Split editor com arquivo secundário editável
- Diff editor lado a lado para patch preview e diffs do Git
- Source Control com botão de diff por arquivo
- Command Palette e Quick Open mantidos
- Assistant local com tool calling e apply patch

## Observações

- Projeto estruturado para continuar evoluindo, mas não foi compilado neste ambiente
- O split editor desta v3.3 usa estado próprio para o editor secundário
- O diff editor foca em side-by-side, não em algoritmo visual fino de hunk/gutter

# LocalCodeIDE v3.2 — Workbench / VS Code parity cut

Esta versão muda o foco de "agentic local assistant" para **workbench**. A ideia foi aproximar a experiência do que faz uma IDE/editor parecer mais com VS Code no uso diário.

## O que entrou

- **Activity Bar** com troca de views:
  - Explorer
  - Search
  - Source Control
  - Assistant
- **Command Palette** (`Ctrl+Shift+P`)
  - catálogo básico de comandos
  - filtro por texto
  - execução de ações do workbench
- **Quick Open** (`Ctrl+P`)
  - abrir arquivo por busca rápida no workspace
- **Open Editors / tabs**
  - lista de arquivos abertos
  - troca rápida entre tabs
  - fechamento de tabs
  - indicação de arquivo com mudança não salva no editor atual
- **Source Control view**
  - leitura de `git status --porcelain`
  - lista de arquivos alterados
  - stage / unstage / discard
  - commit por mensagem
- **Problems panel clicável**
  - clicar no diagnóstico navega para o arquivo/linha atual
- **Status bar** mais parecida com workbench
  - tabs abertas
  - mudanças Git
  - backend AI / provider

## Estrutura nova

```text
src/
  ui/models/
    OpenEditorListModel.*
    CommandPaletteListModel.*
    GitChangeListModel.*
  services/
    GitService.*              # ampliado
  services/interfaces/
    IVersionControlProvider.hpp   # ampliado
  adapters/vcs/
    GitCliProvider.*          # agora lista changes e executa stage/unstage/commit
qml/components/
  ExplorerPanel.qml
  SearchPanel.qml
  SourceControlPanel.qml
  CommandPaletteDialog.qml
  QuickOpenDialog.qml
```

## Atalhos desta versão

- `Ctrl+P` → Quick Open
- `Ctrl+Shift+P` → Command Palette
- `Ctrl+F` → foco em Search

## O que ainda falta para ficar mais VS Code

- árvore real de explorer com pastas colapsáveis
- editor split lado a lado / grid
- debugger / Run view
- extensões / marketplace
- timeline / history por arquivo
- terminal PTY completo com tabs/split
- minimap e diff editor mais fortes

## Build

Requer Qt 6.5+ com `Core`, `Quick`, `Qml` e `Network`.

```bash
cmake -S . -B build
cmake --build build
./build/LocalCodeIDE
```

## Observações

- o ambiente daqui não tem Qt instalado, então eu **não compilei** o scaffold
- esta versão foi pensada para **workbench parity**, não para sandbox/permissões
- o `Source Control` depende da instalação local do `git`
- o editor continua sendo um `TextArea` + highlighter, então ainda não é um engine de editor do nível Monaco/VS Code
