#include "ui/viewmodels/EditorManager.hpp"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <algorithm>

namespace ide::ui::viewmodels {

namespace {
QString loadTextFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QTextStream stream(&file);
    return stream.readAll();
}

bool saveTextFile(const QString& path, const QString& text) {
    const QFileInfo info(path);
    QDir().mkpath(info.dir().absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }
    QTextStream stream(&file);
    stream << text;
    return true;
}

QString inferLanguageId(const QString& path) {
    const QString lower = path.toLower();
    if (lower.endsWith(".rs") || lower.endsWith(".ron")) return "rust";
    if (lower.endsWith(".py")) return "python";
    if (lower.endsWith(".js") || lower.endsWith(".ts") || lower.endsWith(".tsx") || lower.endsWith(".jsx")) return "typescript";
    if (lower.endsWith(".qml")) return "qml";
    if (lower.endsWith(".md") || lower.endsWith(".markdown") || lower.endsWith(".rst")) return "markdown";
    if (lower.endsWith(".json") || lower.endsWith(".jsonc") || lower.endsWith(".json5")) return "json";
    if (lower.endsWith(".yaml") || lower.endsWith(".yml")) return "yaml";
    if (lower.endsWith(".toml")) return "toml";
    if (lower.endsWith(".ini") || lower.endsWith(".conf")) return "ini";
    if (lower.endsWith(".ps1") || lower.endsWith(".psm1") || lower.endsWith(".psd1")) return "powershell";
    return "cpp";
}
}

EditorManager::EditorManager(ide::services::DocumentService* documentService, QObject* parent)
    : QObject(parent)
    , m_documentService(documentService) {
}

QString EditorManager::currentPath() const {
    if (!m_documentService) {
        return {};
    }
    return m_documentService->currentDocument().path();
}

QString EditorManager::editorText() const {
    if (!m_documentService) {
        return {};
    }
    return m_documentService->currentDocument().text();
}

void EditorManager::setEditorText(const QString& text) {
    if (!m_documentService) {
        return;
    }
    if (text == m_documentService->currentDocument().text()) {
        return;
    }
    m_documentService->setText(text);
    emit editorTextChanged();
    syncOpenEditors();
}

QString EditorManager::languageId() const {
    return inferLanguageId(currentPath());
}

QString EditorManager::editorTabTitle() const {
    return displayTitleForPath(currentPath());
}

bool EditorManager::currentDocumentDirty() const {
    return editorText() != m_savedTextSnapshot;
}

QString EditorManager::secondaryEditorPath() const {
    return m_splitState.secondaryPath;
}

QString EditorManager::secondaryEditorText() const {
    return m_splitState.secondaryText;
}

void EditorManager::setSecondaryEditorText(const QString& text) {
    if (text == m_splitState.secondaryText) return;
    m_splitState.secondaryText = text;
    emit splitEditorChanged();
}

QString EditorManager::secondaryLanguageId() const {
    return inferLanguageId(m_splitState.secondaryPath);
}

QString EditorManager::secondaryEditorTitle() const {
    return displayTitleForPath(m_splitState.secondaryPath);
}

bool EditorManager::secondaryEditorDirty() const {
    return !m_splitState.secondaryPath.isEmpty() && m_splitState.secondaryText != m_splitState.secondarySavedSnapshot;
}

QString EditorManager::diffOriginalText() const {
    return m_splitState.diffOriginalText;
}

QString EditorManager::diffModifiedText() const {
    return m_splitState.diffModifiedText;
}

QString EditorManager::diffEditorTitle() const {
    return m_splitState.diffEditorTitle;
}

bool EditorManager::splitEditorVisible() const {
    return m_splitState.visible;
}

bool EditorManager::diffEditorVisible() const {
    return m_splitState.visible && m_splitState.diffMode;
}

QObject* EditorManager::openEditorsModel() {
    return &m_openEditorsModel;
}

int EditorManager::openEditorCount() const {
    return static_cast<int>(m_openEditors.size());
}

void EditorManager::openFile(const QString& path) {
    if (!m_documentService) {
        emit openEditorsChanged();
        return;
    }
    if (!m_documentService->openFile(path)) {
        emit openEditorsChanged();
        return;
    }
    m_savedTextSnapshot = editorText();
    touchOpenEditor(path);
    syncOpenEditors();
    emit currentPathChanged();
    emit editorTextChanged();
}

void EditorManager::openFileInSplit(const QString& path) {
    if (!loadFileIntoSecondaryEditor(path)) {
        return;
    }
    m_splitState.visible = true;
    m_splitState.diffMode = false;
    emit splitEditorChanged();
}

void EditorManager::closeSplitEditor() {
    m_splitState.visible = false;
    m_splitState.diffMode = false;
    m_splitState.secondaryPath.clear();
    m_splitState.secondaryText.clear();
    m_splitState.secondarySavedSnapshot.clear();
    m_splitState.diffOriginalText.clear();
    m_splitState.diffModifiedText.clear();
    m_splitState.diffEditorTitle.clear();
    emit splitEditorChanged();
}

void EditorManager::saveSecondaryEditor() {
    if (m_splitState.secondaryPath.isEmpty() || m_splitState.diffMode) {
        return;
    }
    if (!saveTextFile(m_splitState.secondaryPath, m_splitState.secondaryText)) {
        return;
    }
    m_splitState.secondarySavedSnapshot = m_splitState.secondaryText;
    emit splitEditorChanged();
    emit workspaceChanged();
    emit gitChanged();
}

void EditorManager::switchOpenEditor(const QString& path) {
    if (path.isEmpty() || path == currentPath()) return;
    openFile(path);
}

void EditorManager::closeOpenEditor(const QString& path) {
    if (path.isEmpty()) return;
    const auto originalSize = m_openEditors.size();
    m_openEditors.erase(std::remove_if(m_openEditors.begin(), m_openEditors.end(), [&](const auto& item) {
        return item.path == path;
    }), m_openEditors.end());

    if (m_openEditors.size() == originalSize) return;
    if (path == currentPath() && !m_openEditors.empty()) {
        openFile(m_openEditors.front().path);
        return;
    }
    syncOpenEditors();
}

void EditorManager::saveCurrent() {
    if (!m_documentService) {
        return;
    }
    if (m_documentService->saveFile()) {
        m_savedTextSnapshot = editorText();
        touchOpenEditor(currentPath());
        syncOpenEditors();
        emit currentPathChanged();
        emit workspaceChanged();
        return;
    }
}

void EditorManager::closeAllEditors() {
    m_openEditors.clear();
    closeSplitEditor();
    syncOpenEditors();
}

void EditorManager::setInMemoryDocument(const QString& path, const QString& text) {
    if (!m_documentService) {
        return;
    }
    m_documentService->setPath(path);
    m_documentService->setText(text);
    m_savedTextSnapshot = text;
    touchOpenEditor(path);
    syncOpenEditors();
    emit currentPathChanged();
    emit editorTextChanged();
}

void EditorManager::setupDiffView(const QString& title, const QString& originalText, const QString& modifiedText) {
    m_splitState.visible = true;
    m_splitState.diffMode = true;
    m_splitState.diffOriginalText = originalText;
    m_splitState.diffModifiedText = modifiedText;
    m_splitState.diffEditorTitle = title;
    emit splitEditorChanged();
}

void EditorManager::clearDiffView() {
    m_splitState.diffMode = false;
    m_splitState.diffOriginalText.clear();
    m_splitState.diffModifiedText.clear();
    m_splitState.diffEditorTitle.clear();
    emit splitEditorChanged();
}

void EditorManager::setCurrentDocumentDirty(bool dirty) {
    Q_UNUSED(dirty);
    emit currentDocumentDirtyChanged();
}

void EditorManager::refreshWorkspace() {
    emit workspaceChanged();
}

void EditorManager::refreshGitState() {
    emit gitChanged();
}

QString EditorManager::inferLanguageId(const QString& path) const {
    return ide::ui::viewmodels::inferLanguageId(path);
}

QString EditorManager::displayTitleForPath(const QString& path) const {
    const QFileInfo info(path);
    return info.fileName().isEmpty() ? QStringLiteral("untitled") : info.fileName();
}

QString EditorManager::loadTextFile(const QString& path) const {
    return ide::ui::viewmodels::loadTextFile(path);
}

bool EditorManager::saveTextFile(const QString& path, const QString& text) const {
    return ide::ui::viewmodels::saveTextFile(path, text);
}

void EditorManager::syncOpenEditors() {
    for (auto& item : m_openEditors) {
        item.active = (item.path == currentPath());
        if (item.path == currentPath()) {
            item.title = displayTitleForPath(item.path);
            item.dirty = currentDocumentDirty();
        }
    }
    m_openEditorsModel.setEditors(m_openEditors);
    emit openEditorsChanged();
}

void EditorManager::touchOpenEditor(const QString& path) {
    if (path.isEmpty()) return;
    auto it = std::find_if(m_openEditors.begin(), m_openEditors.end(), [&](const auto& item) {
        return item.path == path;
    });
    if (it == m_openEditors.end()) {
        m_openEditors.insert(m_openEditors.begin(), {path, displayTitleForPath(path), false, false});
    } else {
        auto item = *it;
        m_openEditors.erase(it);
        m_openEditors.insert(m_openEditors.begin(), item);
    }
}

bool EditorManager::loadFileIntoSecondaryEditor(const QString& path) {
    const QString text = loadTextFile(path);
    if (text.isNull()) {
        return false;
    }
    m_splitState.secondaryPath = path;
    m_splitState.secondaryText = text;
    m_splitState.secondarySavedSnapshot = text;
    return true;
}

void EditorManager::updateCurrentDocumentState() {
    syncOpenEditors();
    emit openEditorsChanged();
}

} // namespace ide::ui::viewmodels
