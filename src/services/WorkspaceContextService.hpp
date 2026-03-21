#pragma once

#include "services/interfaces/IWorkspaceProvider.hpp"

#include <QString>
#include <QStringList>
#include <vector>

namespace ide::services {

struct RelevantContextFile {
    QString path;
    QString relativePath;
    QString reason;
    QString excerpt;
    int score = 0;
};

class WorkspaceContextService {
public:
    struct Options {
        int maxFiles = 4;
        int maxTotalChars = 16000;
        int maxExcerptCharsPerFile = 4000;
    };

    std::vector<RelevantContextFile> collect(const QString& workspaceRoot,
                                             const QString& currentPath,
                                             const QString& prompt,
                                             const std::vector<interfaces::WorkspaceFile>& workspaceFiles,
                                             const Options& options = {}) const;

    QString render(const std::vector<RelevantContextFile>& files,
                   int maxTotalChars) const;

private:
    QStringList tokenize(const QString& text) const;
    bool isProbablyTextFile(const QString& path) const;
    int scoreFile(const QString& workspaceRoot,
                  const QString& currentPath,
                  const QStringList& tokens,
                  const interfaces::WorkspaceFile& file,
                  QString* reason,
                  QString* excerpt,
                  int maxExcerptChars) const;
    QString readBestExcerpt(const QString& path,
                            const QStringList& tokens,
                            int maxChars) const;
    QString fenceLanguage(const QString& path) const;
};

} // namespace ide::services
