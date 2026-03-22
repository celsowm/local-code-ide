#include "services/HardwareProfileService.hpp"

#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QtGlobal>
#include <utility>

#ifdef Q_OS_WIN
#define NOMINMAX
#include <windows.h>
#endif

namespace ide::services {

namespace {
constexpr double kBytesPerGiB = 1024.0 * 1024.0 * 1024.0;

QString runCommand(const QString& program, const QStringList& args, int timeoutMs = 2200) {
    QProcess process;
    process.start(program, args);
    if (!process.waitForStarted(500)) {
        return {};
    }
    process.waitForFinished(timeoutMs);
    return QString::fromUtf8(process.readAllStandardOutput()) + QString::fromUtf8(process.readAllStandardError());
}

double bytesToGiB(qulonglong bytes) {
    return static_cast<double>(bytes) / kBytesPerGiB;
}

double parseFirstDouble(const QString& text) {
    static const QRegularExpression numberPattern(QStringLiteral("([0-9]+(?:[\\.,][0-9]+)?)"));
    const auto match = numberPattern.match(text);
    if (!match.hasMatch()) {
        return -1.0;
    }
    QString value = match.captured(1).trimmed();
    value.replace(',', '.');
    bool ok = false;
    const double parsed = value.toDouble(&ok);
    return ok ? parsed : -1.0;
}

qulonglong parseFirstUnsignedLongLong(const QString& text) {
    static const QRegularExpression integerPattern(QStringLiteral("([0-9]{4,})"));
    const auto match = integerPattern.match(text);
    if (!match.hasMatch()) {
        return 0;
    }
    bool ok = false;
    const qulonglong parsed = match.captured(1).toULongLong(&ok);
    return ok ? parsed : 0;
}

double parseMaxMiBLine(const QString& text) {
    double maxMiB = -1.0;
    const auto lines = text.split('\n', Qt::SkipEmptyParts);
    for (const auto& line : lines) {
        if (line.contains(QStringLiteral("VRAM Total Used"), Qt::CaseInsensitive)) {
            continue;
        }
        const double value = parseFirstDouble(line);
        if (value > maxMiB) {
            maxMiB = value;
        }
    }
    return maxMiB > 0.0 ? (maxMiB / 1024.0) : -1.0;
}

QString trimAndCollapse(const QString& text) {
    QString simplified = text.simplified();
    return simplified;
}
}

bool HardwareProfile::hasSystemRam() const {
    return systemRamGiB > 0.0;
}

bool HardwareProfile::hasGpuVram() const {
    return gpuVramGiB > 0.0;
}

double HardwareProfile::recommendedBudgetGiB() const {
    if (hasGpuVram()) {
        return qMax(2.0, gpuVramGiB * 0.82);
    }
    if (hasSystemRam()) {
        return qMax(2.0, systemRamGiB * 0.45);
    }
    return -1.0;
}

QString HardwareProfile::autoProfile(const QString& queryHint) const {
    const bool codingBias = queryHint.contains(QStringLiteral("code"), Qt::CaseInsensitive)
        || queryHint.contains(QStringLiteral("coder"), Qt::CaseInsensitive)
        || queryHint.contains(QStringLiteral("dev"), Qt::CaseInsensitive)
        || queryHint.contains(QStringLiteral("rust"), Qt::CaseInsensitive)
        || queryHint.contains(QStringLiteral("cpp"), Qt::CaseInsensitive)
        || queryHint.contains(QStringLiteral("c++"), Qt::CaseInsensitive)
        || queryHint.contains(QStringLiteral("python"), Qt::CaseInsensitive);

    QString baseProfile = QStringLiteral("balanced");
    if ((hasGpuVram() && gpuVramGiB >= 20.0) || (hasSystemRam() && systemRamGiB >= 64.0)) {
        baseProfile = QStringLiteral("quality");
    } else if ((hasGpuVram() && gpuVramGiB >= 10.0) || (hasSystemRam() && systemRamGiB >= 24.0)) {
        baseProfile = QStringLiteral("balanced");
    } else {
        baseProfile = QStringLiteral("laptop");
    }

    if (codingBias) {
        return QStringLiteral("coding");
    }
    return baseProfile;
}

QString HardwareProfile::summary() const {
    QStringList parts;
    if (hasSystemRam()) {
        parts.push_back(QStringLiteral("RAM %1 GiB").arg(QString::number(systemRamGiB, 'f', systemRamGiB >= 10.0 ? 0 : 1)));
    } else {
        parts.push_back(QStringLiteral("RAM ?"));
    }

    if (hasGpuVram()) {
        const QString label = gpuName.isEmpty()
            ? QStringLiteral("VRAM %1 GiB").arg(QString::number(gpuVramGiB, 'f', gpuVramGiB >= 10.0 ? 0 : 1))
            : QStringLiteral("%1 · VRAM %2 GiB").arg(trimAndCollapse(gpuName), QString::number(gpuVramGiB, 'f', gpuVramGiB >= 10.0 ? 0 : 1));
        parts.push_back(label);
    } else {
        parts.push_back(QStringLiteral("VRAM não detectada"));
    }

    const double budget = recommendedBudgetGiB();
    if (budget > 0.0) {
        parts.push_back(QStringLiteral("budget útil ~%1 GiB").arg(QString::number(budget, 'f', budget >= 10.0 ? 0 : 1)));
    }

    parts.push_back(QStringLiteral("auto=%1").arg(autoProfile()));
    return parts.join(QStringLiteral(" · "));
}

HardwareProfileService::HardwareProfileService(QString ramOverrideGiB, QString vramOverrideGiB)
    : m_ramOverrideGiB(std::move(ramOverrideGiB))
    , m_vramOverrideGiB(std::move(vramOverrideGiB)) {}

HardwareProfile HardwareProfileService::detect() const {
    HardwareProfile profile;
    profile.systemRamGiB = parseOverrideGiB(m_ramOverrideGiB);
    if (profile.systemRamGiB <= 0.0) {
        profile.systemRamGiB = detectSystemRamGiB();
    }

    QString gpuName;
    profile.gpuVramGiB = parseOverrideGiB(m_vramOverrideGiB);
    if (profile.gpuVramGiB <= 0.0) {
        profile.gpuVramGiB = detectGpuVramGiB(&gpuName);
    }
    profile.gpuName = gpuName;
    return profile;
}

double HardwareProfileService::parseOverrideGiB(const QString& value) const {
    if (value.trimmed().isEmpty()) {
        return -1.0;
    }
    return parseFirstDouble(value);
}

double HardwareProfileService::detectSystemRamGiB() const {
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex)) {
        return bytesToGiB(statex.ullTotalPhys);
    }
    return -1.0;
#elif defined(Q_OS_MACOS)
    const QString output = runCommand(QStringLiteral("sysctl"), {QStringLiteral("-n"), QStringLiteral("hw.memsize")});
    const qulonglong bytes = output.trimmed().toULongLong();
    return bytes > 0 ? bytesToGiB(bytes) : -1.0;
#else
    QFile meminfo(QStringLiteral("/proc/meminfo"));
    if (!meminfo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1.0;
    }
    while (!meminfo.atEnd()) {
        const QByteArray line = meminfo.readLine();
        if (!line.startsWith("MemTotal:")) {
            continue;
        }
        const QString content = QString::fromUtf8(line);
        const qulonglong kib = parseFirstUnsignedLongLong(content);
        return kib > 0 ? (static_cast<double>(kib) / (1024.0 * 1024.0)) : -1.0;
    }
    return -1.0;
#endif
}

double HardwareProfileService::detectGpuVramGiB(QString* gpuName) const {
#ifdef Q_OS_WIN
    const QString nvidia = runCommand(
        QStringLiteral("nvidia-smi"),
        {QStringLiteral("--query-gpu=name,memory.total"), QStringLiteral("--format=csv,noheader,nounits")},
        2000
    );
    double bestGiB = -1.0;
    for (const auto& line : nvidia.split('\n', Qt::SkipEmptyParts)) {
        const auto parts = line.split(',');
        if (parts.size() < 2) {
            continue;
        }
        const QString name = parts.at(0).trimmed();
        const double mib = parseFirstDouble(parts.at(1));
        const double gib = mib > 0.0 ? (mib / 1024.0) : -1.0;
        if (gib > bestGiB) {
            bestGiB = gib;
            if (gpuName) {
                *gpuName = name;
            }
        }
    }
    if (bestGiB > 0.0) {
        return bestGiB;
    }

    const QString output = runCommand(
        QStringLiteral("powershell"),
        {
            QStringLiteral("-NoProfile"),
            QStringLiteral("-Command"),
            QStringLiteral("Get-CimInstance Win32_VideoController | ForEach-Object { \"$($_.Name)|$($_.AdapterRAM)\" }")
        },
        2800
    );
    bestGiB = -1.0;
    for (const auto& line : output.split('\n', Qt::SkipEmptyParts)) {
        const auto parts = line.split('|');
        if (parts.size() < 2) {
            continue;
        }
        const qulonglong bytes = parts.at(1).trimmed().toULongLong();
        const double gib = bytes > 0 ? bytesToGiB(bytes) : -1.0;
        if (gib > bestGiB) {
            bestGiB = gib;
            if (gpuName) {
                *gpuName = parts.at(0).trimmed();
            }
        }
    }
    return bestGiB;
#elif defined(Q_OS_MACOS)
    const QString output = runCommand(QStringLiteral("system_profiler"), {QStringLiteral("SPDisplaysDataType")}, 5000);
    if (gpuName) {
        static const QRegularExpression namePattern(QStringLiteral(R"(Chipset Model:\s*(.+))"));
        const auto nameMatch = namePattern.match(output);
        if (nameMatch.hasMatch()) {
            *gpuName = nameMatch.captured(1).trimmed();
        }
    }
    static const QRegularExpression vramPattern(QStringLiteral(R"(VRAM[^:]*:\s*([0-9]+(?:\.[0-9]+)?)\s*(GB|MB))"), QRegularExpression::CaseInsensitiveOption);
    auto it = vramPattern.globalMatch(output);
    double bestGiB = -1.0;
    while (it.hasNext()) {
        const auto match = it.next();
        double value = match.captured(1).toDouble();
        const QString unit = match.captured(2).toUpper();
        if (unit == QStringLiteral("MB")) {
            value /= 1024.0;
        }
        if (value > bestGiB) {
            bestGiB = value;
        }
    }
    return bestGiB;
#else
    const QString nvidia = runCommand(
        QStringLiteral("nvidia-smi"),
        {QStringLiteral("--query-gpu=name,memory.total"), QStringLiteral("--format=csv,noheader,nounits")},
        2000
    );
    double bestGiB = -1.0;
    for (const auto& line : nvidia.split('\n', Qt::SkipEmptyParts)) {
        const auto parts = line.split(',');
        if (parts.size() < 2) {
            continue;
        }
        const QString name = parts.at(0).trimmed();
        const double mib = parseFirstDouble(parts.at(1));
        const double gib = mib > 0.0 ? (mib / 1024.0) : -1.0;
        if (gib > bestGiB) {
            bestGiB = gib;
            if (gpuName) {
                *gpuName = name;
            }
        }
    }
    if (bestGiB > 0.0) {
        return bestGiB;
    }

    const QString rocm = runCommand(
        QStringLiteral("rocm-smi"),
        {QStringLiteral("--showproductname"), QStringLiteral("--showmeminfo"), QStringLiteral("vram")},
        2600
    );
    if (gpuName && gpuName->isEmpty()) {
        static const QRegularExpression productPattern(QStringLiteral(R"(Card series:\s*(.+))"));
        const auto productMatch = productPattern.match(rocm);
        if (productMatch.hasMatch()) {
            *gpuName = productMatch.captured(1).trimmed();
        }
    }
    return parseMaxMiBLine(rocm);
#endif
}

} // namespace ide::services
