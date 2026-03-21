#include "core/Document.hpp"

namespace ide::core {

Document::Document(QString path, QString text)
    : m_path(std::move(path)), m_text(std::move(text)) {}

const QString& Document::path() const {
    return m_path;
}

const QString& Document::text() const {
    return m_text;
}

void Document::setPath(QString path) {
    m_path = std::move(path);
}

void Document::setText(QString text) {
    m_text = std::move(text);
}

} // namespace ide::core
