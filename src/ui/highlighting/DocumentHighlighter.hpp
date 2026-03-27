#pragma once

#include "ui/highlighting/SyntaxHighlighterBackend.hpp"
#include "ui/highlighting/SyntaxHighlighterFactory.hpp"

#include <QObject>
#include <QPointer>
#include <QVariantList>
#include <memory>

class QQuickTextDocument;

namespace ide::ui::highlighting {

class DocumentHighlighter : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QVariantList diagnostics READ diagnostics WRITE setDiagnostics NOTIFY diagnosticsChanged)
public:
    explicit DocumentHighlighter(QObject* parent = nullptr);
    void setHighlighterFactory(std::unique_ptr<ISyntaxHighlighterFactory> factory);

    QObject* textDocument() const;
    void setTextDocument(QObject* textDocument);

    QString language() const;
    void setLanguage(const QString& language);

    QVariantList diagnostics() const;
    void setDiagnostics(const QVariantList& diagnostics);

signals:
    void textDocumentChanged();
    void languageChanged();
    void diagnosticsChanged();

private:
    void rebuildHighlighter();

    QPointer<QObject> m_textDocument;
    QString m_language = "cpp";
    QVariantList m_diagnostics;
    QPointer<SyntaxHighlighterBackend> m_highlighter;
    std::unique_ptr<ISyntaxHighlighterFactory> m_highlighterFactory;
};

} // namespace ide::ui::highlighting
