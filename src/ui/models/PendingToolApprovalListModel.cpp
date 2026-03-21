#include "ui/models/PendingToolApprovalListModel.hpp"

namespace ide::ui::models {

PendingToolApprovalListModel::PendingToolApprovalListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int PendingToolApprovalListModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(m_approvals.size());
}

QVariant PendingToolApprovalListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_approvals.size())) {
        return {};
    }

    const auto& item = m_approvals[static_cast<std::size_t>(index.row())];
    switch (role) {
    case ApprovalIdRole:
        return item.approvalId;
    case ToolNameRole:
        return item.toolName;
    case SummaryRole:
        return item.summary;
    case ArgumentsTextRole:
        return item.argumentsText;
    case PathHintsRole:
        return item.pathHints.join(QStringLiteral(" | "));
    case DestructiveRole:
        return item.destructive;
    default:
        return {};
    }
}

QHash<int, QByteArray> PendingToolApprovalListModel::roleNames() const {
    return {
        {ApprovalIdRole, "approvalId"},
        {ToolNameRole, "toolName"},
        {SummaryRole, "summary"},
        {ArgumentsTextRole, "argumentsText"},
        {PathHintsRole, "pathHints"},
        {DestructiveRole, "destructive"}
    };
}

void PendingToolApprovalListModel::setApprovals(std::vector<ide::services::PendingToolApproval> approvals) {
    beginResetModel();
    m_approvals = std::move(approvals);
    endResetModel();
}

int PendingToolApprovalListModel::approvalCount() const {
    return static_cast<int>(m_approvals.size());
}

} // namespace ide::ui::models
