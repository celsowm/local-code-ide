#pragma once

#include "services/DocumentService.hpp"
#include "ui/models/OpenEditorListModel.hpp"
#include <QObject>
#include <QString>
#include <vector>

namespace ide::ui::viewmodels {

struct EditorState {
    QString path;
    QString title;
    QString text;
    QString savedSnapshot;
    bool dirty = false;
    bool active = false;
};

struct SplitEditorState {
    bool visible = false;
    bool diffMode = false;
    QString secondaryPath;
    QString secondaryText;
    QString secondarySavedSnapshot;
    QString diffOriginalText;
    QString diffModifiedText;
    QString diffEditorTitle;
};

using OpenEditorItem = ide::ui::models::OpenEditorItem;

class EditorManager final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentPathChanged)
    Q_PROPERTY(QString editorText READ editorText WRITE setEditorText NOTIFY editorTextChanged)
    Q_PROPERTY(QString languageId READ languageId NOTIFY currentPathChanged)
    Q_PROPERTY(QString editorTabTitle READ editorTabTitle NOTIFY currentPathChanged)
    Q_PROPERTY(bool currentDocumentDirty READ currentDocumentDirty NOTIFY openEditorsChanged)

    Q_PROPERTY(QString secondaryEditorPath READ secondaryEditorPath NOTIFY splitEditorChanged)
    Q_PROPERTY(QString secondaryEditorText READ secondaryEditorText WRITE setSecondaryEditorText NOTIFY splitEditorChanged)
    Q_PROPERTY(QString secondaryLanguageId READ secondaryLanguageId NOTIFY splitEditorChanged)
    Q_PROPERTY(QString secondaryEditorTitle READ secondaryEditorTitle NOTIFY splitEditorChanged)
    Q_PROPERTY(bool secondaryEditorDirty READ secondaryEditorDirty NOTIFY splitEditorChanged)

    Q_PROPERTY(QString diffOriginalText READ diffOriginalText NOTIFY splitEditorChanged)
    Q_PROPERTY(QString diffModifiedText READ diffModifiedText NOTIFY splitEditorChanged)
    Q_PROPERTY(QString diffEditorTitle READ diffEditorTitle NOTIFY splitEditorChanged)

    Q_PROPERTY(bool splitEditorVisible READ splitEditorVisible NOTIFY splitEditorChanged)
    Q_PROPERTY(bool diffEditorVisible READ diffEditorVisible NOTIFY splitEditorChanged)

    Q_PROPERTY(QObject* openEditorsModel READ openEditorsModel CONSTANT)
    Q_PROPERTY(int openEditorCount READ openEditorCount NOTIFY openEditorsChanged)

public:
    explicit EditorManager(ide::services::DocumentService* documentService, QObject* parent = nullptr);

    QString currentPath() const;
    QString editorText() const;
    void setEditorText(const QString& text);
    QString languageId() const;
    QString editorTabTitle() const;
    bool currentDocumentDirty() const;

    QString secondaryEditorPath() const;
    QString secondaryEditorText() const;
    void setSecondaryEditorText(const QString& text);
    QString secondaryLanguageId() const;
    QString secondaryEditorTitle() const;
    bool secondaryEditorDirty() const;

    QString diffOriginalText() const;
    QString diffModifiedText() const;
    QString diffEditorTitle() const;

    bool splitEditorVisible() const;
    bool diffEditorVisible() const;

    QObject* openEditorsModel();
    int openEditorCount() const;

    Q_INVOKABLE void openFile(const QString& path);
    Q_INVOKABLE void openFileInSplit(const QString& path);
    Q_INVOKABLE void closeSplitEditor();
    Q_INVOKABLE void saveSecondaryEditor();
    Q_INVOKABLE void switchOpenEditor(const QString& path);
    Q_INVOKABLE void closeOpenEditor(const QString& path);
    Q_INVOKABLE void saveCurrent();

    Q_INVOKABLE void setupDiffView(const QString& title, const QString& originalText, const QString& modifiedText);
    Q_INVOKABLE void clearDiffView();

    void setCurrentDocumentDirty(bool dirty);
    void refreshWorkspace();
    void refreshGitState();

    QString inferLanguageId(const QString& path) const;

signals:
    void editorTextChanged();
    void currentPathChanged();
    void currentDocumentDirtyChanged();
    void splitEditorChanged();
    void openEditorsChanged();
    void workspaceChanged();
    void gitChanged();

private:
    QString displayTitleForPath(const QString& path) const;
    QString loadTextFile(const QString& path) const;
    bool saveTextFile(const QString& path, const QString& text) const;
    void syncOpenEditors();
    void touchOpenEditor(const QString& path);
    bool loadFileIntoSecondaryEditor(const QString& path);
    void updateCurrentDocumentState();

    ide::services::DocumentService* m_documentService = nullptr;
    ide::ui::models::OpenEditorListModel m_openEditorsModel;

    QString m_savedTextSnapshot;
    std::vector<OpenEditorItem> m_openEditors;

    SplitEditorState m_splitState;
};

} // namespace ide::ui::viewmodels
