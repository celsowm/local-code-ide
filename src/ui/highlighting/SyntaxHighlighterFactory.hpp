#pragma once

#include "ui/highlighting/SyntaxHighlighterBackend.hpp"

#include <memory>

class QTextDocument;
class QString;

namespace ide::ui::highlighting {

class ISyntaxHighlighterFactory {
public:
    virtual ~ISyntaxHighlighterFactory() = default;
    virtual std::unique_ptr<SyntaxHighlighterBackend> create(const QString& normalizedLanguage,
                                                             QTextDocument* document) const = 0;
};

class DefaultSyntaxHighlighterFactory final : public ISyntaxHighlighterFactory {
public:
    std::unique_ptr<SyntaxHighlighterBackend> create(const QString& normalizedLanguage,
                                                     QTextDocument* document) const override;
};

} // namespace ide::ui::highlighting
