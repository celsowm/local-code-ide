#pragma once

#include "core/Document.hpp"

namespace ide::services {

class DocumentService {
public:
    DocumentService();

    const ide::core::Document& currentDocument() const;

    void createUntitled();
    bool openFile(const QString& path);
    bool saveFile(const QString& path = QString());
    void setText(const QString& text);
    void setPath(const QString& path);

private:
    ide::core::Document m_document;
};

} // namespace ide::services
