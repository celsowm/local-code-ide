#pragma once

#include <QString>

namespace ide::services {

struct HardwareProfile {
    double systemRamGiB = -1.0;
    double gpuVramGiB = -1.0;
    QString gpuName;

    bool hasSystemRam() const;
    bool hasGpuVram() const;
    double recommendedBudgetGiB() const;
    QString autoProfile(const QString& queryHint = {}) const;
    QString summary() const;
};

class HardwareProfileService {
public:
    HardwareProfileService(QString ramOverrideGiB = {}, QString vramOverrideGiB = {});

    HardwareProfile detect() const;

private:
    double parseOverrideGiB(const QString& value) const;
    double detectSystemRamGiB() const;
    double detectGpuVramGiB(QString* gpuName) const;

    QString m_ramOverrideGiB;
    QString m_vramOverrideGiB;
};

} // namespace ide::services
