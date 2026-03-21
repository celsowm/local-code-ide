#include "ui/models/DiagnosticListModel.hpp"

#include <QStringList>

namespace ide::ui::models {

DiagnosticListModel::DiagnosticListModel(QObject* parent)
    : QAbstractListModel(parent) {}

int DiagnosticListModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_diagnostics.size());
}

QVariant DiagnosticListModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(m_diagnostics.size())) {
        return {};
    }

    const auto& item = m_diagnostics[static_cast<std::size_t>(index.row())];
    switch (role) {
    case LineRole:
        return item.line;
    case ColumnRole:
        return item.column;
    case MessageRole:
        return item.message;
    case SeverityRole:
        switch (item.severity) {
        case ide::services::interfaces::Diagnostic::Severity::Info:
            return "info";
        case ide::services::interfaces::Diagnostic::Severity::Warning:
            return "warning";
        case ide::services::interfaces::Diagnostic::Severity::Error:
            return "error";
        }
        break;
    default:
        break;
    }
    return {};
}

QHash<int, QByteArray> DiagnosticListModel::roleNames() const {
    return {
        {LineRole, "line"},
        {ColumnRole, "column"},
        {MessageRole, "message"},
        {SeverityRole, "severity"}
    };
}

void DiagnosticListModel::setDiagnostics(std::vector<ide::services::interfaces::Diagnostic> diagnostics) {
    beginResetModel();
    m_diagnostics = std::move(diagnostics);
    endResetModel();
}

int DiagnosticListModel::diagnosticCount() const {
    return static_cast<int>(m_diagnostics.size());
}

QString DiagnosticListModel::summaryText(int limit) const {
    if (m_diagnostics.empty()) {
        return QStringLiteral("No diagnostics.");
    }

    QStringList lines;
    int count = 0;
    for (const auto& item : m_diagnostics) {
        if (count >= limit) {
            lines << QStringLiteral("... %1 more").arg(QString::number(static_cast<int>(m_diagnostics.size()) - limit));
            break;
        }

        QString severity;
        switch (item.severity) {
        case ide::services::interfaces::Diagnostic::Severity::Info:
            severity = QStringLiteral("info");
            break;
        case ide::services::interfaces::Diagnostic::Severity::Warning:
            severity = QStringLiteral("warning");
            break;
        case ide::services::interfaces::Diagnostic::Severity::Error:
            severity = QStringLiteral("error");
            break;
        }
        lines << QStringLiteral("%1:%2 [%3] %4")
                     .arg(QString::number(item.line), QString::number(item.column), severity, item.message);
        ++count;
    }
    return lines.join('
');
}

} // namespace ide::ui::models
