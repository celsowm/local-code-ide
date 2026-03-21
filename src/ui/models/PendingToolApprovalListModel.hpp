#pragma once

#include "services/ToolCallingService.hpp"

#include <QAbstractListModel>
#include <vector>

namespace ide::ui::models {

class PendingToolApprovalListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        ApprovalIdRole = Qt::UserRole + 1,
        ToolNameRole,
        SummaryRole,
        ArgumentsTextRole,
        PathHintsRole,
        DestructiveRole
    };

    explicit PendingToolApprovalListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setApprovals(std::vector<ide::services::PendingToolApproval> approvals);
    int approvalCount() const;

private:
    std::vector<ide::services::PendingToolApproval> m_approvals;
};

} // namespace ide::ui::models
