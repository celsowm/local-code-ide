#include "ui/models/ModelFileListModel.hpp"

namespace ide::ui::models {

ModelFileListModel::ModelFileListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int ModelFileListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_files.size());
}

QVariant ModelFileListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_files.size())) {
        return {};
    }

    const auto& file = m_files[static_cast<std::size_t>(index.row())];
    switch (role) {
    case PathRole:
        return file.path;
    case NameRole:
        return file.name;
    case SizeBytesRole:
        return file.sizeBytes;
    case SizeLabelRole:
        return formatBytes(file.sizeBytes);
    case GgufRole:
        return file.isGguf;
    case SplitRole:
        return file.isSplit;
    case QuantizationRole:
        return file.quantization;
    case RecommendationRole:
        return file.path == m_recommendedPath;
    case ActiveLocalRole:
        return file.path == m_activeRemotePath;
    default:
        return {};
    }
}

QHash<int, QByteArray> ModelFileListModel::roleNames() const {
    return {
        {PathRole, "path"},
        {NameRole, "name"},
        {SizeBytesRole, "sizeBytes"},
        {SizeLabelRole, "sizeLabel"},
        {GgufRole, "isGguf"},
        {SplitRole, "isSplit"},
        {QuantizationRole, "quantization"},
        {RecommendationRole, "recommended"},
        {ActiveLocalRole, "activeLocal"}
    };
}

void ModelFileListModel::setFiles(std::vector<ide::services::interfaces::ModelFileEntry> files,
                                  const QString& recommendedPath,
                                  const QString& activeRemotePath) {
    beginResetModel();
    m_files = std::move(files);
    m_recommendedPath = recommendedPath;
    m_activeRemotePath = activeRemotePath;
    endResetModel();
}

int ModelFileListModel::fileCount() const {
    return static_cast<int>(m_files.size());
}

std::vector<ide::services::interfaces::ModelFileEntry> ModelFileListModel::files() const {
    return m_files;
}

QString ModelFileListModel::formatBytes(qint64 value) const {
    if (value <= 0) {
        return QStringLiteral("?");
    }

    const double gib = static_cast<double>(value) / (1024.0 * 1024.0 * 1024.0);
    if (gib >= 1.0) {
        return QString::number(gib, 'f', gib >= 10.0 ? 1 : 2) + QStringLiteral(" GiB");
    }
    const double mib = static_cast<double>(value) / (1024.0 * 1024.0);
    return QString::number(mib, 'f', 1) + QStringLiteral(" MiB");
}

} // namespace ide::ui::models
