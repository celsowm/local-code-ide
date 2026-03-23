#include "ui/viewmodels/GitViewModel.hpp"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>

namespace ide::ui::viewmodels {

namespace {
struct GitRefreshResult {
    bool hasService = false;
    QString summary;
    QString branchLabel;
    std::vector<ide::services::interfaces::GitChange> changes;
    std::vector<ide::services::interfaces::GitCommitSummary> commits;
};

QString loadTextFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    QTextStream stream(&file);
    return stream.readAll();
}
}

GitViewModel::GitViewModel(ide::services::GitService* gitService, QObject* parent)
    : QObject(parent)
    , m_gitService(gitService) {
}

QString GitViewModel::gitSummary() const {
    return m_gitSummary;
}

QString GitViewModel::gitBranchLabel() const {
    return m_gitBranchLabel;
}

QString GitViewModel::scmCommitMessage() const {
    return m_scmCommitMessage;
}

void GitViewModel::setScmCommitMessage(const QString& value) {
    if (value == m_scmCommitMessage) return;
    m_scmCommitMessage = value;
    emit scmCommitMessageChanged();
}

QObject* GitViewModel::gitChangesModel() {
    return &m_gitChangesModel;
}

QObject* GitViewModel::scmSectionsModel() {
    return &m_scmSectionsModel;
}

QObject* GitViewModel::gitRecentCommitsModel() {
    return &m_gitRecentCommitsModel;
}

int GitViewModel::gitChangeCount() const {
    return m_gitChangesModel.changeCount();
}

int GitViewModel::gitStagedCount() const {
    return m_scmSectionsModel.stagedCount();
}

int GitViewModel::gitUnstagedCount() const {
    return m_scmSectionsModel.unstagedCount();
}

int GitViewModel::gitUntrackedCount() const {
    return m_scmSectionsModel.untrackedCount();
}

int GitViewModel::gitRecentCommitCount() const {
    return m_gitRecentCommitsModel.commitCount();
}

bool GitViewModel::busy() const {
    return m_pendingOperationCount > 0 || !m_operationQueue.isEmpty();
}

const std::vector<ide::services::interfaces::GitChange>& GitViewModel::currentChanges() const {
    return m_lastChanges;
}

void GitViewModel::refresh(const QString& workspaceRootPath) {
    const int generation = ++m_refreshGeneration;
    const QString workspace = workspaceRootPath;

    auto* watcher = new QFutureWatcher<GitRefreshResult>(this);
    connect(watcher, &QFutureWatcher<GitRefreshResult>::finished, this, [this, watcher, generation]() {
        const GitRefreshResult result = watcher->result();
        watcher->deleteLater();

        if (generation != m_refreshGeneration) {
            return;
        }

        if (!result.hasService) {
            m_gitSummary = QStringLiteral("Git unavailable.");
            m_gitBranchLabel.clear();
            m_lastChanges.clear();
            m_gitChangesModel.setChanges({});
            m_scmSectionsModel.setChanges({});
            m_gitRecentCommitsModel.setCommits({});
            emit gitSummaryChanged();
            emit gitChanged();
            return;
        }

        m_gitSummary = result.summary;
        m_gitBranchLabel = result.branchLabel;
        m_lastChanges = result.changes;
        m_gitChangesModel.setChanges(result.changes);
        m_scmSectionsModel.setChanges(result.changes);
        m_gitRecentCommitsModel.setCommits(result.commits);
        emit gitSummaryChanged();
        emit gitChanged();
    });

    watcher->setFuture(QtConcurrent::run([this, workspace]() {
        GitRefreshResult result;
        if (!m_gitService) {
            return result;
        }

        result.hasService = true;
        result.summary = m_gitService->summary(workspace);
        result.branchLabel = m_gitService->branchLabel(workspace);
        result.changes = m_gitService->listChanges(workspace);
        result.commits = m_gitService->recentCommits(workspace, 8);
        return result;
    }));
}

void GitViewModel::stage(const QString& workspaceRootPath, const QString& path) {
    if (!m_gitService) {
        return;
    }
    enqueueOperation(PendingOperation{OperationType::Stage, workspaceRootPath, path, {}});
}

void GitViewModel::unstage(const QString& workspaceRootPath, const QString& path) {
    if (!m_gitService) {
        return;
    }
    enqueueOperation(PendingOperation{OperationType::Unstage, workspaceRootPath, path, {}});
}

void GitViewModel::discard(const QString& workspaceRootPath, const QString& path) {
    if (!m_gitService) {
        return;
    }
    enqueueOperation(PendingOperation{OperationType::Discard, workspaceRootPath, path, {}});
}

void GitViewModel::openDiff(const QString& workspaceRootPath, const QString& path, const QString& currentEditorPath, const QString& currentEditorText) {
    Q_UNUSED(workspaceRootPath);
    Q_UNUSED(path);
    Q_UNUSED(currentEditorPath);
    Q_UNUSED(currentEditorText);
}

void GitViewModel::commit(const QString& workspaceRootPath) {
    if (!m_gitService) {
        return;
    }
    enqueueOperation(PendingOperation{OperationType::Commit, workspaceRootPath, {}, m_scmCommitMessage});
}

QString GitViewModel::headFileContent(const QString& workspaceRootPath, const QString& path) const {
    if (!m_gitService) {
        return {};
    }
    return m_gitService->headFileContent(workspaceRootPath, path);
}

void GitViewModel::enqueueOperation(PendingOperation operation) {
    m_operationQueue.enqueue(std::move(operation));
    updateBusyState();
    runNextOperation();
}

void GitViewModel::runNextOperation() {
    if (m_pendingOperationCount > 0 || m_operationQueue.isEmpty()) {
        return;
    }
    if (!m_gitService) {
        m_operationQueue.clear();
        updateBusyState();
        return;
    }

    const PendingOperation operation = m_operationQueue.dequeue();
    m_pendingOperationCount = 1;
    updateBusyState();

    auto* watcher = new QFutureWatcher<OperationResult>(this);
    connect(watcher, &QFutureWatcher<OperationResult>::finished, this, [this, watcher]() {
        const OperationResult result = watcher->result();
        watcher->deleteLater();

        m_pendingOperationCount = 0;

        if (result.success && result.clearCommitMessage) {
            setScmCommitMessage(QString());
        }
        emit operationCompleted(
            result.type == OperationType::Stage ? QStringLiteral("stage")
                : result.type == OperationType::Unstage ? QStringLiteral("unstage")
                : result.type == OperationType::Discard ? QStringLiteral("discard")
                : QStringLiteral("commit"),
            result.path,
            result.success,
            result.message
        );
        refresh(result.workspaceRootPath);
        updateBusyState();
        runNextOperation();
    });

    watcher->setFuture(QtConcurrent::run([this, operation]() {
        OperationResult result;
        result.type = operation.type;
        result.workspaceRootPath = operation.workspaceRootPath;
        result.path = operation.path;

        switch (operation.type) {
        case OperationType::Stage:
            result.success = m_gitService->stage(operation.workspaceRootPath, operation.path);
            result.message = result.success
                ? QStringLiteral("Staged %1").arg(operation.path)
                : QStringLiteral("Failed to stage %1").arg(operation.path);
            break;
        case OperationType::Unstage:
            result.success = m_gitService->unstage(operation.workspaceRootPath, operation.path);
            result.message = result.success
                ? QStringLiteral("Unstaged %1").arg(operation.path)
                : QStringLiteral("Failed to unstage %1").arg(operation.path);
            break;
        case OperationType::Discard:
            result.success = m_gitService->discard(operation.workspaceRootPath, operation.path);
            result.message = result.success
                ? QStringLiteral("Discarded changes in %1").arg(operation.path)
                : QStringLiteral("Failed to discard changes in %1").arg(operation.path);
            break;
        case OperationType::Commit:
            result.success = m_gitService->commit(operation.workspaceRootPath, operation.commitMessage);
            result.clearCommitMessage = result.success;
            result.message = result.success
                ? QStringLiteral("Commit created.")
                : QStringLiteral("Failed to create commit.");
            break;
        }
        return result;
    }));
}

void GitViewModel::updateBusyState() {
    emit busyChanged();
}

} // namespace ide::ui::viewmodels
