#pragma once

#include "ui/highlighting/CppSyntaxHighlighter.hpp"

#include <QObject>
#include <memory>

class QQuickTextDocument;

namespace ide::ui::highlighting {

class DocumentHighlighter : public QObject {
    Q_OBJECT
    Q_PROPERTY(QObject* textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
public:
    explicit DocumentHighlighter(QObject* parent = nullptr);

    QObject* textDocument() const;
    void setTextDocument(QObject* textDocument);

    QString language() const;
    void setLanguage(const QString& language);

signals:
    void textDocumentChanged();
    void languageChanged();

private:
    void rebuildHighlighter();

    QObject* m_textDocument = nullptr;
    QString m_language = "cpp";
    std::unique_ptr<CppSyntaxHighlighter> m_cppHighlighter;
};

} // namespace ide::ui::highlighting
