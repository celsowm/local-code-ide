#include "ui/models/WorkspaceFileListModel.hpp"

#include <QFileInfo>

namespace ide::ui::models {

WorkspaceFileListModel::WorkspaceFileListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int WorkspaceFileListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_files.size());
}

QVariant WorkspaceFileListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_files.size())) {
        return {};
    }

    const auto& file = m_files[static_cast<std::size_t>(index.row())];
    switch (role) {
    case PathRole:
        return file.path;
    case RelativePathRole:
        return file.relativePath;
    case NameRole:
        return QFileInfo(file.relativePath).fileName();
    default:
        return {};
    }
}

QHash<int, QByteArray> WorkspaceFileListModel::roleNames() const {
    return {
        {PathRole, "path"},
        {RelativePathRole, "relativePath"},
        {NameRole, "name"}
    };
}

void WorkspaceFileListModel::setFiles(std::vector<ide::services::interfaces::WorkspaceFile> files) {
    beginResetModel();
    m_files = std::move(files);
    endResetModel();
}

int WorkspaceFileListModel::fileCount() const {
    return static_cast<int>(m_files.size());
}

} // namespace ide::ui::models
