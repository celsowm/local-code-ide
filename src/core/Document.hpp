#pragma once

#include <QString>

namespace ide::core {

class Document {
public:
    Document() = default;
    Document(QString path, QString text);

    const QString& path() const;
    const QString& text() const;

    void setPath(QString path);
    void setText(QString text);

private:
    QString m_path;
    QString m_text;
};

} // namespace ide::core
