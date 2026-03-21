#pragma once

#include "services/interfaces/IModelCatalogProvider.hpp"

#include <QAbstractListModel>
#include <vector>

namespace ide::ui::models {

class ModelFileListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        NameRole,
        SizeBytesRole,
        SizeLabelRole,
        GgufRole,
        SplitRole,
        QuantizationRole,
        RecommendationRole,
        ActiveLocalRole
    };

    explicit ModelFileListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFiles(std::vector<ide::services::interfaces::ModelFileEntry> files,
                  const QString& recommendedPath = {},
                  const QString& activeRemotePath = {});
    int fileCount() const;
    std::vector<ide::services::interfaces::ModelFileEntry> files() const;

private:
    QString formatBytes(qint64 value) const;

    std::vector<ide::services::interfaces::ModelFileEntry> m_files;
    QString m_recommendedPath;
    QString m_activeRemotePath;
};

} // namespace ide::ui::models
