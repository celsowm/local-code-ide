#include "adapters/search/FileSystemSearchProvider.hpp"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

namespace ide::adapters::search {
using SearchResult = ide::services::interfaces::SearchResult;

namespace {
bool shouldSkipPath(const QString& path) {
    return path.contains("/.git/") || path.contains("/build/") || path.contains("/node_modules/");
}

bool isProbablyTextFile(const QString& path) {
    const QString suffix = QFileInfo(path).suffix().toLower();
    static const QSet<QString> allowed = {
        "cpp", "c", "cc", "cxx", "hpp", "h", "hh", "rs", "py", "js", "ts", "tsx", "jsx",
        "json", "toml", "yaml", "yml", "md", "txt", "cmake", "qml", "java", "go"
    };
    return allowed.contains(suffix) || suffix.isEmpty();
}
} // namespace

std::vector<SearchResult> FileSystemSearchProvider::search(const QString& workspaceRoot, const QString& pattern) {
    std::vector<SearchResult> results;
    if (workspaceRoot.isEmpty() || pattern.trimmed().isEmpty()) {
        return results;
    }

    QDirIterator it(workspaceRoot, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        if (shouldSkipPath(path) || !isProbablyTextFile(path)) {
            continue;
        }

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QTextStream stream(&file);
        int lineNumber = 0;
        while (!stream.atEnd()) {
            const QString line = stream.readLine();
            ++lineNumber;
            const int idx = line.indexOf(pattern, 0, Qt::CaseInsensitive);
            if (idx >= 0) {
                SearchResult result;
                result.path = path;
                result.line = lineNumber;
                result.column = idx + 1;
                result.preview = line.trimmed();
                results.push_back(std::move(result));
                if (results.size() >= 500) {
                    return results;
                }
            }
        }
    }

    return results;
}

QString FileSystemSearchProvider::name() const {
    return "Filesystem Search Provider";
}

} // namespace ide::adapters::search
