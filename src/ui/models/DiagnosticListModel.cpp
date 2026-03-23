#include "ui/models/DiagnosticListModel.hpp"

#include <QStringList>
#include <QVariantMap>

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
        return item.lineStart;
    case ColumnRole:
        return item.columnStart;
    case EndLineRole:
        return item.lineEnd;
    case EndColumnRole:
        return item.columnEnd;
    case FilePathRole:
        return item.filePath;
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
    case SourceRole:
        return item.source;
    case CodeRole:
        return item.code;
    default:
        break;
    }
    return {};
}

QHash<int, QByteArray> DiagnosticListModel::roleNames() const {
    return {
        {LineRole, "line"},
        {ColumnRole, "column"},
        {EndLineRole, "endLine"},
        {EndColumnRole, "endColumn"},
        {FilePathRole, "filePath"},
        {MessageRole, "message"},
        {SeverityRole, "severity"},
        {SourceRole, "source"},
        {CodeRole, "code"}
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
                     .arg(QString::number(item.lineStart), QString::number(item.columnStart), severity, item.message);
        ++count;
    }
    return lines.join('\n');
}

QVariantList DiagnosticListModel::asVariantList() const {
    QVariantList rows;
    rows.reserve(static_cast<int>(m_diagnostics.size()));
    for (const auto& item : m_diagnostics) {
        QVariantMap row;
        row.insert(QStringLiteral("filePath"), item.filePath);
        row.insert(QStringLiteral("lineStart"), item.lineStart);
        row.insert(QStringLiteral("columnStart"), item.columnStart);
        row.insert(QStringLiteral("lineEnd"), item.lineEnd);
        row.insert(QStringLiteral("columnEnd"), item.columnEnd);
        row.insert(QStringLiteral("message"), item.message);
        row.insert(QStringLiteral("source"), item.source);
        row.insert(QStringLiteral("code"), item.code);
        switch (item.severity) {
        case ide::services::interfaces::Diagnostic::Severity::Info:
            row.insert(QStringLiteral("severity"), QStringLiteral("info"));
            break;
        case ide::services::interfaces::Diagnostic::Severity::Warning:
            row.insert(QStringLiteral("severity"), QStringLiteral("warning"));
            break;
        case ide::services::interfaces::Diagnostic::Severity::Error:
            row.insert(QStringLiteral("severity"), QStringLiteral("error"));
            break;
        }
        rows.push_back(row);
    }
    return rows;
}

} // namespace ide::ui::models
