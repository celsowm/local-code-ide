#include "services/DocumentService.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace ide::services {

DocumentService::DocumentService() {
    createUntitled();
}

const ide::core::Document& DocumentService::currentDocument() const {
    return m_document;
}

void DocumentService::createUntitled() {
    m_document.setPath("untitled.cpp");
    m_document.setText(
        "#include <iostream>\n"
        "#include <vector>\n\n"
        "int main() {\n"
        "    std::vector<int> xs{1, 2, 3};\n"
        "    std::cout << \"Hello from LocalCodeIDE v2\\n\";\n"
        "    // TODO: connect real editor engine\n"
        "    return 0;\n"
        "}\n"
    );
}

bool DocumentService::openFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream stream(&file);
    m_document.setPath(path);
    m_document.setText(stream.readAll());
    return true;
}

bool DocumentService::saveFile(const QString& path) {
    const QString finalPath = path.isEmpty() ? m_document.path() : path;
    const QFileInfo fileInfo(finalPath);
    if (!fileInfo.dir().exists()) {
        QDir().mkpath(fileInfo.dir().absolutePath());
    }

    QFile file(finalPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream stream(&file);
    stream << m_document.text();
    m_document.setPath(finalPath);
    return true;
}

void DocumentService::setText(const QString& text) {
    m_document.setText(text);
}

void DocumentService::setPath(const QString& path) {
    m_document.setPath(path);
}

} // namespace ide::services
