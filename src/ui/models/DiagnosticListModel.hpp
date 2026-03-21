#pragma once

#include "services/interfaces/IDiagnosticProvider.hpp"

#include <QAbstractListModel>
#include <QString>
#include <vector>

namespace ide::ui::models {

class DiagnosticListModel final : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        LineRole = Qt::UserRole + 1,
        ColumnRole,
        MessageRole,
        SeverityRole
    };

    explicit DiagnosticListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setDiagnostics(std::vector<ide::services::interfaces::Diagnostic> diagnostics);
    int diagnosticCount() const;
    QString summaryText(int limit = 8) const;

private:
    std::vector<ide::services::interfaces::Diagnostic> m_diagnostics;
};

} // namespace ide::ui::models
