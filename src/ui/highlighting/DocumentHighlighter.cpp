#include "ui/highlighting/DocumentHighlighter.hpp"

#include <QQuickTextDocument>
#include <QTextDocument>
#include <QVariantMap>

namespace ide::ui::highlighting {

DocumentHighlighter::DocumentHighlighter(QObject* parent)
    : QObject(parent)
    , m_highlighterFactory(std::make_unique<DefaultSyntaxHighlighterFactory>()) {}

void DocumentHighlighter::setHighlighterFactory(std::unique_ptr<ISyntaxHighlighterFactory> factory) {
    if (!factory) {
        m_highlighterFactory = std::make_unique<DefaultSyntaxHighlighterFactory>();
    } else {
        m_highlighterFactory = std::move(factory);
    }
    rebuildHighlighter();
}

QObject* DocumentHighlighter::textDocument() const {
    return m_textDocument.data();
}

void DocumentHighlighter::setTextDocument(QObject* textDocument) {
    if (textDocument == m_textDocument) {
        return;
    }

    if (m_textDocument) {
        disconnect(m_textDocument, &QObject::destroyed, this, nullptr);
    }
    m_textDocument = textDocument;
    if (m_textDocument) {
        connect(m_textDocument, &QObject::destroyed, this, [this]() {
            m_textDocument = nullptr;
            m_highlighter = nullptr;
        });
    }
    rebuildHighlighter();
    emit textDocumentChanged();
}

QString DocumentHighlighter::language() const {
    return m_language;
}

void DocumentHighlighter::setLanguage(const QString& language) {
    if (language == m_language) {
        return;
    }

    m_language = language;
    rebuildHighlighter();
    emit languageChanged();
}

QVariantList DocumentHighlighter::diagnostics() const {
    return m_diagnostics;
}

void DocumentHighlighter::setDiagnostics(const QVariantList& diagnostics) {
    const bool changed = (m_diagnostics != diagnostics);
    if (changed) {
        m_diagnostics = diagnostics;
    }
    if (m_highlighter) {
        std::vector<SyntaxHighlighterBackend::DiagnosticRange> ranges;
        ranges.reserve(static_cast<std::size_t>(m_diagnostics.size()));
        for (const QVariant& item : m_diagnostics) {
            const QVariantMap row = item.toMap();
            SyntaxHighlighterBackend::DiagnosticRange range;
            range.lineStart = row.value("lineStart").toInt();
            range.columnStart = row.value("columnStart").toInt();
            range.lineEnd = row.value("lineEnd").toInt();
            range.columnEnd = row.value("columnEnd").toInt();
            range.severity = row.value("severity").toString();
            ranges.push_back(std::move(range));
        }
        m_highlighter->setDiagnostics(std::move(ranges));
    }
    if (changed) {
        emit diagnosticsChanged();
    }
}

void DocumentHighlighter::rebuildHighlighter() {
    if (m_highlighter) {
        m_highlighter->setDocument(nullptr);
        m_highlighter->deleteLater();
        m_highlighter = nullptr;
    }

    auto* quickDocument = qobject_cast<QQuickTextDocument*>(m_textDocument.data());
    if (!quickDocument) {
        return;
    }

    QTextDocument* document = quickDocument->textDocument();
    if (!document) {
        return;
    }

    const QString language = m_language.toLower();
    if (!m_highlighterFactory) {
        m_highlighterFactory = std::make_unique<DefaultSyntaxHighlighterFactory>();
    }
    auto created = m_highlighterFactory->create(language, document);
    m_highlighter = created.release();
    setDiagnostics(m_diagnostics);
}

} // namespace ide::ui::highlighting
