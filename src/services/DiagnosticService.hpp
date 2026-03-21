#pragma once

#include "services/interfaces/IDiagnosticProvider.hpp"

#include <memory>
#include <vector>

namespace ide::services {

class DiagnosticService {
public:
    explicit DiagnosticService(std::unique_ptr<interfaces::IDiagnosticProvider> provider);

    std::vector<interfaces::Diagnostic> refresh(const QString& filePath, const QString& text);
    QString providerName() const;

private:
    std::unique_ptr<interfaces::IDiagnosticProvider> m_provider;
};

} // namespace ide::services
