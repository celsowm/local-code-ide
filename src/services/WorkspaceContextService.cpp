#include "services/WorkspaceContextService.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QTextStream>

#include <algorithm>

namespace ide::services {
using WorkspaceFile = ide::services::interfaces::WorkspaceFile;

namespace {
QString normalizeSlash(QString value) {
    return QDir::fromNativeSeparators(std::move(value));
}

QSet<QString> stopWords() {
    return {
        QStringLiteral("the"), QStringLiteral("and"), QStringLiteral("for"), QStringLiteral("with"),
        QStringLiteral("this"), QStringLiteral("that"), QStringLiteral("from"), QStringLiteral("into"),
        QStringLiteral("uma"), QStringLiteral("para"), QStringLiteral("com"), QStringLiteral("dos"),
        QStringLiteral("das"), QStringLiteral("que"), QStringLiteral("por"), QStringLiteral("sem"),
        QStringLiteral("tem"), QStringLiteral("vai"), QStringLiteral("ser"), QStringLiteral("uma"),
        QStringLiteral("como"), QStringLiteral("erro"), QStringLiteral("diff"), QStringLiteral("patch")
    };
}

QString fileNameOnly(const QString& path) {
    return QFileInfo(path).fileName().toLower();
}
} // namespace

QStringList WorkspaceContextService::tokenize(const QString& text) const {
    QString normalized = text.toLower();
    normalized.replace(QRegularExpression(QStringLiteral("[^a-z0-9_./-]+")), QStringLiteral(" "));
    QStringList parts = normalized.split(' ', Qt::SkipEmptyParts);
    const auto stop = stopWords();

    QStringList tokens;
    for (const QString& part : parts) {
        const QString trimmed = part.trimmed();
        if (trimmed.size() < 3 || stop.contains(trimmed)) {
            continue;
        }
        if (!tokens.contains(trimmed)) {
            tokens.push_back(trimmed);
        }
    }
    return tokens;
}

bool WorkspaceContextService::isProbablyTextFile(const QString& path) const {
    const QString suffix = QFileInfo(path).suffix().toLower();
    static const QSet<QString> allowed = {
        QStringLiteral("c"), QStringLiteral("cc"), QStringLiteral("cpp"), QStringLiteral("cxx"),
        QStringLiteral("h"), QStringLiteral("hpp"), QStringLiteral("hh"), QStringLiteral("hxx"),
        QStringLiteral("rs"), QStringLiteral("py"), QStringLiteral("js"), QStringLiteral("ts"),
        QStringLiteral("tsx"), QStringLiteral("jsx"), QStringLiteral("java"), QStringLiteral("kt"),
        QStringLiteral("go"), QStringLiteral("zig"), QStringLiteral("cmake"), QStringLiteral("txt"),
        QStringLiteral("md"), QStringLiteral("json"), QStringLiteral("yaml"), QStringLiteral("yml"),
        QStringLiteral("toml"), QStringLiteral("ini"), QStringLiteral("sh")
    };
    return allowed.contains(suffix);
}

int WorkspaceContextService::scoreFile(const QString& workspaceRoot,
                                       const QString& currentPath,
                                       const QStringList& tokens,
                                       const WorkspaceFile& file,
                                       QString* reason,
                                       QString* excerpt,
                                       int maxExcerptChars) const {
    Q_UNUSED(workspaceRoot)
    const QString normalizedCurrent = normalizeSlash(currentPath);
    const QString normalizedPath = normalizeSlash(file.path);
    if (normalizedPath == normalizedCurrent) {
        return -1;
    }

    QFileInfo info(file.path);
    if (!info.exists() || !info.isFile() || info.size() > 1024 * 1024 || !isProbablyTextFile(file.path)) {
        return -1;
    }

    int score = 0;
    QStringList why;

    const QFileInfo currentInfo(currentPath);
    if (!currentPath.isEmpty() && info.dir().absolutePath() == currentInfo.dir().absolutePath()) {
        score += 30;
        why << QStringLiteral("same directory");
    }
    if (!currentInfo.suffix().isEmpty() && info.suffix().compare(currentInfo.suffix(), Qt::CaseInsensitive) == 0) {
        score += 18;
        why << QStringLiteral("same extension");
    }

    const QString relLower = file.relativePath.toLower();
    const QString nameLower = info.fileName().toLower();
    for (const QString& token : tokens) {
        if (relLower.contains(token)) {
            score += 25;
            why << QStringLiteral("path matches '%1'").arg(token);
        } else if (nameLower.contains(token)) {
            score += 18;
            why << QStringLiteral("name matches '%1'").arg(token);
        }
    }

    const QString bestExcerpt = readBestExcerpt(file.path, tokens, maxExcerptChars);
    const QString excerptLower = bestExcerpt.toLower();
    for (const QString& token : tokens) {
        if (excerptLower.contains(token)) {
            score += 8;
            why << QStringLiteral("content matches '%1'").arg(token);
        }
    }

    if (score <= 0) {
        return -1;
    }

    if (reason) {
        why.removeDuplicates();
        *reason = why.mid(0, 3).join(QStringLiteral(", "));
    }
    if (excerpt) {
        *excerpt = bestExcerpt;
    }
    return score;
}

QString WorkspaceContextService::readBestExcerpt(const QString& path,
                                                 const QStringList& tokens,
                                                 int maxChars) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream stream(&file);
    QString text = stream.read(maxChars * 4);
    if (text.size() <= maxChars || tokens.isEmpty()) {
        return text.left(maxChars).trimmed();
    }

    const QStringList lines = text.split('\n');
    int bestIndex = 0;
    int bestScore = -1;
    for (int i = 0; i < lines.size(); ++i) {
        const QString lower = lines[i].toLower();
        int score = 0;
        for (const QString& token : tokens) {
            if (lower.contains(token)) {
                score += 1;
            }
        }
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }

    const int start = std::max(0, bestIndex - 8);
    const int end = std::min(lines.size(), bestIndex + 12);
    QString excerpt = lines.mid(start, end - start).join('\n').trimmed();
    if (excerpt.size() > maxChars) {
        excerpt = excerpt.left(maxChars);
    }
    return excerpt;
}

std::vector<RelevantContextFile> WorkspaceContextService::collect(const QString& workspaceRoot,
                                                                  const QString& currentPath,
                                                                  const QString& prompt,
                                                                  const std::vector<WorkspaceFile>& workspaceFiles,
                                                                  const Options& options) const {
    QStringList tokens = tokenize(prompt + QStringLiteral(" ") + fileNameOnly(currentPath));
    std::vector<RelevantContextFile> ranked;
    ranked.reserve(workspaceFiles.size());

    for (const auto& file : workspaceFiles) {
        RelevantContextFile item;
        item.path = file.path;
        item.relativePath = file.relativePath;
        item.score = scoreFile(workspaceRoot, currentPath, tokens, file, &item.reason, &item.excerpt, options.maxExcerptCharsPerFile);
        if (item.score > 0) {
            ranked.push_back(std::move(item));
        }
    }

    std::sort(ranked.begin(), ranked.end(), [](const RelevantContextFile& a, const RelevantContextFile& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.relativePath.toLower() < b.relativePath.toLower();
    });

    if (static_cast<int>(ranked.size()) > options.maxFiles) {
        ranked.resize(static_cast<std::size_t>(options.maxFiles));
    }
    return ranked;
}

QString WorkspaceContextService::fenceLanguage(const QString& path) const {
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QStringLiteral("rs") || suffix == QStringLiteral("py") || suffix == QStringLiteral("js") || suffix == QStringLiteral("ts") || suffix == QStringLiteral("json") || suffix == QStringLiteral("md") || suffix == QStringLiteral("toml") || suffix == QStringLiteral("sh")) {
        return suffix;
    }
    if (suffix == QStringLiteral("c") || suffix == QStringLiteral("cc") || suffix == QStringLiteral("cpp") || suffix == QStringLiteral("cxx") || suffix == QStringLiteral("h") || suffix == QStringLiteral("hpp")) {
        return QStringLiteral("cpp");
    }
    return QString();
}

QString WorkspaceContextService::render(const std::vector<RelevantContextFile>& files,
                                        int maxTotalChars) const {
    if (files.empty() || maxTotalChars <= 0) {
        return {};
    }

    QString result;
    int used = 0;
    for (const auto& file : files) {
        QString block = QStringLiteral("File: %1\nReason: %2\n```%3\n%4\n```\n\n")
            .arg(file.relativePath, file.reason, fenceLanguage(file.relativePath), file.excerpt);
        if (used + block.size() > maxTotalChars) {
            const int remaining = maxTotalChars - used;
            if (remaining <= 0) {
                break;
            }
            block = block.left(remaining);
        }
        result += block;
        used += block.size();
        if (used >= maxTotalChars) {
            break;
        }
    }
    return result.trimmed();
}

} // namespace ide::services
