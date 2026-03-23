#include "ui/viewmodels/GitViewModel.hpp"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>

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

void GitViewModel::refresh(const QString& workspaceRootPath) {
    if (!m_gitService) {
        m_gitSummary = QStringLiteral("Git unavailable.");
        m_gitBranchLabel.clear();
        m_gitChangesModel.setChanges({});
        m_scmSectionsModel.setChanges({});
        m_gitRecentCommitsModel.setCommits({});
        emit gitSummaryChanged();
        emit gitChanged();
        return;
    }
    m_gitSummary = m_gitService->summary(workspaceRootPath);
    m_gitBranchLabel = m_gitService->branchLabel(workspaceRootPath);
    const auto changes = m_gitService->listChanges(workspaceRootPath);
    m_gitChangesModel.setChanges(changes);
    m_scmSectionsModel.setChanges(changes);
    m_gitRecentCommitsModel.setCommits(m_gitService->recentCommits(workspaceRootPath, 8));
    emit gitSummaryChanged();
    emit gitChanged();
}

void GitViewModel::stage(const QString& workspaceRootPath, const QString& path) {
    if (!m_gitService) {
        return;
    }
    if (m_gitService->stage(workspaceRootPath, path)) {
        refresh(workspaceRootPath);
    }
}

void GitViewModel::unstage(const QString& workspaceRootPath, const QString& path) {
    if (!m_gitService) {
        return;
    }
    if (m_gitService->unstage(workspaceRootPath, path)) {
        refresh(workspaceRootPath);
    }
}

void GitViewModel::discard(const QString& workspaceRootPath, const QString& path) {
    if (!m_gitService) {
        return;
    }
    if (m_gitService->discard(workspaceRootPath, path)) {
        refresh(workspaceRootPath);
    }
}

void GitViewModel::openDiff(const QString& workspaceRootPath, const QString& path, const QString& currentEditorPath, const QString& currentEditorText) {
    Q_UNUSED(workspaceRootPath);
    Q_UNUSED(path);
    Q_UNUSED(currentEditorPath);
    Q_UNUSED(currentEditorText);
}

bool GitViewModel::commit(const QString& workspaceRootPath) {
    if (!m_gitService) {
        return false;
    }
    if (m_gitService->commit(workspaceRootPath, m_scmCommitMessage)) {
        setScmCommitMessage(QString());
        refresh(workspaceRootPath);
        return true;
    }
    return false;
}

QString GitViewModel::headFileContent(const QString& workspaceRootPath, const QString& path) const {
    if (!m_gitService) {
        return {};
    }
    return m_gitService->headFileContent(workspaceRootPath, path);
}

} // namespace ide::ui::viewmodels
