#include "ui/models/RelevantContextListModel.hpp"

namespace ide::ui::models {

RelevantContextListModel::RelevantContextListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int RelevantContextListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_files.size());
}

QVariant RelevantContextListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_files.size())) {
        return {};
    }

    const auto& file = m_files[static_cast<std::size_t>(index.row())];
    switch (role) {
    case PathRole:
        return file.path;
    case RelativePathRole:
        return file.relativePath;
    case ReasonRole:
        return file.reason;
    case ScoreRole:
        return file.score;
    case ExcerptRole:
        return file.excerpt;
    default:
        return {};
    }
}

QHash<int, QByteArray> RelevantContextListModel::roleNames() const {
    return {
        {PathRole, "path"},
        {RelativePathRole, "relativePath"},
        {ReasonRole, "reason"},
        {ScoreRole, "score"},
        {ExcerptRole, "excerpt"}
    };
}

void RelevantContextListModel::setFiles(std::vector<ide::services::RelevantContextFile> files) {
    beginResetModel();
    m_files = std::move(files);
    endResetModel();
}

int RelevantContextListModel::fileCount() const {
    return static_cast<int>(m_files.size());
}

} // namespace ide::ui::models
