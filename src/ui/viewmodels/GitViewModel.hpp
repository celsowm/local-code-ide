#pragma once

#include "services/GitService.hpp"
#include "ui/models/GitChangeListModel.hpp"
#include "ui/models/ScmSectionListModel.hpp"
#include "ui/models/GitCommitListModel.hpp"
#include <QObject>
#include <QQueue>
#include <QString>
#include <vector>

namespace ide::ui::viewmodels {

class GitViewModel final : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString gitSummary READ gitSummary NOTIFY gitSummaryChanged)
    Q_PROPERTY(QString gitBranchLabel READ gitBranchLabel NOTIFY gitSummaryChanged)
    Q_PROPERTY(QString scmCommitMessage READ scmCommitMessage WRITE setScmCommitMessage NOTIFY scmCommitMessageChanged)

    Q_PROPERTY(QObject* gitChangesModel READ gitChangesModel CONSTANT)
    Q_PROPERTY(QObject* scmSectionsModel READ scmSectionsModel CONSTANT)
    Q_PROPERTY(QObject* gitRecentCommitsModel READ gitRecentCommitsModel CONSTANT)

    Q_PROPERTY(int gitChangeCount READ gitChangeCount NOTIFY gitChanged)
    Q_PROPERTY(int gitStagedCount READ gitStagedCount NOTIFY gitChanged)
    Q_PROPERTY(int gitUnstagedCount READ gitUnstagedCount NOTIFY gitChanged)
    Q_PROPERTY(int gitUntrackedCount READ gitUntrackedCount NOTIFY gitChanged)
    Q_PROPERTY(int gitRecentCommitCount READ gitRecentCommitCount NOTIFY gitChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit GitViewModel(ide::services::GitService* gitService, QObject* parent = nullptr);

    QString gitSummary() const;
    QString gitBranchLabel() const;
    QString scmCommitMessage() const;
    void setScmCommitMessage(const QString& value);

    QObject* gitChangesModel();
    QObject* scmSectionsModel();
    QObject* gitRecentCommitsModel();

    int gitChangeCount() const;
    int gitStagedCount() const;
    int gitUnstagedCount() const;
    int gitUntrackedCount() const;
    int gitRecentCommitCount() const;
    bool busy() const;
    const std::vector<ide::services::interfaces::GitChange>& currentChanges() const;

    Q_INVOKABLE void refresh(const QString& workspaceRootPath);
    Q_INVOKABLE void stage(const QString& workspaceRootPath, const QString& path);
    Q_INVOKABLE void unstage(const QString& workspaceRootPath, const QString& path);
    Q_INVOKABLE void discard(const QString& workspaceRootPath, const QString& path);
    Q_INVOKABLE void openDiff(const QString& workspaceRootPath, const QString& path, const QString& currentEditorPath, const QString& currentEditorText);
    Q_INVOKABLE void commit(const QString& workspaceRootPath);

    QString headFileContent(const QString& workspaceRootPath, const QString& path) const;

signals:
    void gitSummaryChanged();
    void gitChanged();
    void scmCommitMessageChanged();
    void busyChanged();
    void operationCompleted(const QString& operation, const QString& path, bool success, const QString& message);

private:
    enum class OperationType {
        Stage,
        Unstage,
        Discard,
        Commit
    };

    struct PendingOperation {
        OperationType type;
        QString workspaceRootPath;
        QString path;
        QString commitMessage;
    };

    struct OperationResult {
        OperationType type = OperationType::Stage;
        QString workspaceRootPath;
        QString path;
        bool success = false;
        bool clearCommitMessage = false;
        QString message;
    };

    void enqueueOperation(PendingOperation operation);
    void runNextOperation();
    void updateBusyState();

    ide::services::GitService* m_gitService = nullptr;
    ide::ui::models::GitChangeListModel m_gitChangesModel;
    ide::ui::models::ScmSectionListModel m_scmSectionsModel;
    ide::ui::models::GitCommitListModel m_gitRecentCommitsModel;
    std::vector<ide::services::interfaces::GitChange> m_lastChanges;

    QString m_gitSummary;
    QString m_gitBranchLabel;
    QString m_scmCommitMessage;
    int m_refreshGeneration = 0;
    int m_pendingOperationCount = 0;
    QQueue<PendingOperation> m_operationQueue;
};

} // namespace ide::ui::viewmodels
