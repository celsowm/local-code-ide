#pragma once

#include "services/WorkspaceContextService.hpp"

#include <QAbstractListModel>
#include <vector>

namespace ide::ui::models {

class RelevantContextListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        RelativePathRole,
        ReasonRole,
        ScoreRole,
        ExcerptRole
    };

    explicit RelevantContextListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFiles(std::vector<ide::services::RelevantContextFile> files);
    int fileCount() const;

private:
    std::vector<ide::services::RelevantContextFile> m_files;
};

} // namespace ide::ui::models
