#include "adapters/codeintel/LanguageServerCodeIntelProvider.hpp"

#include <QRegularExpression>
#include <QSet>

namespace ide::adapters::codeintel {
using namespace ide::services::interfaces;

LanguageServerCodeIntelProvider::LanguageServerCodeIntelProvider(ide::services::LanguageServerHub* hub)
    : m_hub(hub) {}

std::vector<CompletionItem> LanguageServerCodeIntelProvider::completions(const QString& filePath,
                                                                          const QString& text,
                                                                          const EditorPosition& position) {
    if (m_hub) {
        const auto items = m_hub->completions(filePath, text, position);
        if (!items.empty()) {
            return items;
        }
    }
    return basicCompletions(text);
}

HoverInfo LanguageServerCodeIntelProvider::hover(const QString& filePath,
                                                 const QString& text,
                                                 const EditorPosition& position) {
    if (m_hub) {
        const HoverInfo info = m_hub->hover(filePath, text, position);
        if (!info.contents.trimmed().isEmpty()) {
            return info;
        }

        const auto status = m_hub->runtimeStatus(filePath);
        HoverInfo fallback;
        fallback.contents = status.statusLine.isEmpty()
            ? QStringLiteral("Basic mode: language server unavailable for this file.")
            : QStringLiteral("Basic mode: %1").arg(status.statusLine);
        return fallback;
    }

    HoverInfo info;
    info.contents = QStringLiteral("Basic mode: language server unavailable.");
    return info;
}

std::optional<DefinitionLocation> LanguageServerCodeIntelProvider::definition(const QString& filePath,
                                                                               const QString& text,
                                                                               const EditorPosition& position) {
    if (!m_hub) {
        return std::nullopt;
    }
    return m_hub->definition(filePath, text, position);
}

void LanguageServerCodeIntelProvider::completionsAsync(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position,
    std::function<void(std::vector<CompletionItem>)> onReady) {
    if (!onReady) {
        return;
    }
    if (!m_hub) {
        onReady(basicCompletions(text));
        return;
    }

    m_hub->completionsAsync(filePath, text, position, [this, text, onReady = std::move(onReady)](std::vector<CompletionItem> items) mutable {
        if (items.empty()) {
            onReady(basicCompletions(text));
            return;
        }
        onReady(std::move(items));
    });
}

void LanguageServerCodeIntelProvider::hoverAsync(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position,
    std::function<void(HoverInfo)> onReady) {
    if (!onReady) {
        return;
    }
    if (!m_hub) {
        HoverInfo info;
        info.contents = QStringLiteral("Basic mode: language server unavailable.");
        onReady(std::move(info));
        return;
    }

    m_hub->hoverAsync(filePath, text, position, [this, filePath, onReady = std::move(onReady)](HoverInfo info) mutable {
        if (!info.contents.trimmed().isEmpty()) {
            onReady(std::move(info));
            return;
        }

        const auto status = m_hub->runtimeStatus(filePath);
        HoverInfo fallback;
        fallback.contents = status.statusLine.isEmpty()
            ? QStringLiteral("Basic mode: language server unavailable for this file.")
            : QStringLiteral("Basic mode: %1").arg(status.statusLine);
        onReady(std::move(fallback));
    });
}

void LanguageServerCodeIntelProvider::definitionAsync(
    const QString& filePath,
    const QString& text,
    const EditorPosition& position,
    std::function<void(std::optional<DefinitionLocation>)> onReady) {
    if (!onReady) {
        return;
    }
    if (!m_hub) {
        onReady(std::nullopt);
        return;
    }
    m_hub->definitionAsync(filePath, text, position, std::move(onReady));
}

QString LanguageServerCodeIntelProvider::name() const {
    return QStringLiteral("Language Server Hub Code Intel");
}

std::vector<CompletionItem> LanguageServerCodeIntelProvider::basicCompletions(const QString& text) const {
    QRegularExpression tokenRegex(QStringLiteral("\\b[A-Za-z_][A-Za-z0-9_]{2,}\\b"));
    auto it = tokenRegex.globalMatch(text);
    QSet<QString> seen;

    std::vector<CompletionItem> items;
    items.reserve(20);
    while (it.hasNext() && items.size() < 20) {
        const auto match = it.next();
        const QString token = match.captured(0);
        if (seen.contains(token)) {
            continue;
        }
        seen.insert(token);
        items.push_back(CompletionItem{
            token,
            QStringLiteral("basic completion"),
            token,
            QStringLiteral("text")
        });
    }

    if (items.empty()) {
        items.push_back(CompletionItem{
            QStringLiteral("TODO"),
            QStringLiteral("basic completion"),
            QStringLiteral("TODO"),
            QStringLiteral("text")
        });
    }

    return items;
}

} // namespace ide::adapters::codeintel
