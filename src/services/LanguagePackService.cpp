#include "services/LanguagePackService.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileDevice>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QtGlobal>

namespace ide::services {

namespace {
QString normalizedLanguageForEnvKey(QString languageId) {
    QString key = languageId.trimmed().toUpper();
    key.replace('-', '_');
    key.replace('/', '_');
    key.replace('.', '_');
    return key;
}

QStringList splitArgs(const QString& value) {
    return value.split(';', Qt::SkipEmptyParts);
}
}

LanguagePackService::LanguagePackService(bool allowExternalInstall,
                                         bool allowPathFallback,
                                         QObject* parent)
    : QObject(parent)
    , m_allowExternalInstall(allowExternalInstall)
    , m_allowPathFallback(allowPathFallback) {
    loadManifest();
}

QString LanguagePackService::languageForFilePath(const QString& filePath) const {
    const QString lower = QFileInfo(filePath).fileName().toLower();
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    const QString completeSuffix = QFileInfo(filePath).completeSuffix().toLower();

    if (lower == ".gitignore" || lower == ".gitattributes" || lower == ".gitmodules"
        || lower == ".editorconfig") return "ini";
    if (suffix == "rs" || suffix == "ron" || lower == "cargo.toml") return "rust";
    if (suffix == "c" || suffix == "cpp" || suffix == "cxx" || suffix == "cc"
        || suffix == "h" || suffix == "hpp" || suffix == "hxx" || suffix == "hh") return "cpp";
    if (suffix == "js" || suffix == "mjs" || suffix == "cjs" || suffix == "ts"
        || suffix == "cts" || suffix == "mts" || suffix == "jsx" || suffix == "tsx"
        || completeSuffix == "d.ts") return "typescript";
    if (suffix == "py") return "python";
    if (suffix == "json" || suffix == "jsonc" || suffix == "json5") return "json";
    if (suffix == "yaml" || suffix == "yml") return "yaml";
    if (suffix == "toml") return "toml";
    if (suffix == "ini" || suffix == "conf") return "ini";
    if (suffix == "cmake" || lower == "cmakelists.txt" || lower == "cmakecache.txt") return "cmake";
    if (suffix == "md" || suffix == "markdown" || suffix == "rst"
        || lower.startsWith("readme") || lower.startsWith("license")) return "markdown";
    if (suffix == "ps1" || suffix == "psm1" || suffix == "psd1"
        || suffix == "psc1" || suffix == "ps1xml") return "powershell";
    if (suffix == "qml") return "qml";
    return "cpp";
}

LanguagePackService::ServerSpec LanguagePackService::ensureServerReady(const QString& languageId, bool autoInstall) {
    ServerSpec spec;
    const QString normalized = normalizedLanguageId(languageId);
    const QString overrideCommand = developerOverrideCommand(normalized);
    if (!overrideCommand.isEmpty()) {
        spec.ready = true;
        spec.command = overrideCommand;
        spec.args = developerOverrideArgs(normalized);
        spec.source = QStringLiteral("dev-override");
        spec.statusLine = QStringLiteral("%1: using developer override language server").arg(normalized);
        m_statusByLanguage.insert(normalized, spec.statusLine);
        emit languageStatusChanged(normalized, spec.statusLine);
        return spec;
    }

    const auto pack = findPackForLanguage(normalized);
    if (pack.id.isEmpty()) {
        spec.ready = false;
        spec.statusLine = QStringLiteral("%1: basic mode (no bundled language server)").arg(normalized);
        m_statusByLanguage.insert(normalized, spec.statusLine);
        emit languageStatusChanged(normalized, spec.statusLine);
        return spec;
    }

    const QString command = findCandidateCommand(pack);
    if (!command.isEmpty()) {
        spec.ready = true;
        spec.command = command;
        spec.args = pack.defaultArgs;
        spec.source = QFileInfo(command).fileName();
        spec.statusLine = QStringLiteral("%1: LSP ready (%2)").arg(normalized, pack.id);
        m_statusByLanguage.insert(normalized, spec.statusLine);
        emit languageStatusChanged(normalized, spec.statusLine);
        return spec;
    }

    auto& state = m_installStates[normalized];
    if (state.installing) {
        spec.ready = false;
        spec.statusLine = state.status.isEmpty()
            ? QStringLiteral("%1: installing %2...").arg(normalized, pack.id)
            : state.status;
        m_statusByLanguage.insert(normalized, spec.statusLine);
        emit languageStatusChanged(normalized, spec.statusLine);
        return spec;
    }

    if (state.attempted) {
        spec.ready = false;
        spec.statusLine = state.status.isEmpty()
            ? QStringLiteral("%1: language server unavailable; basic mode active").arg(normalized)
            : state.status;
        m_statusByLanguage.insert(normalized, spec.statusLine);
        emit languageStatusChanged(normalized, spec.statusLine);
        return spec;
    }

    const QString installCommand =
#ifdef Q_OS_WIN
        pack.installCommandWindows;
#else
        pack.installCommandLinux;
#endif

    if (autoInstall && m_allowExternalInstall && !installCommand.trimmed().isEmpty()) {
        startInstall(normalized, pack);
        spec.ready = false;
        spec.statusLine = QStringLiteral("%1: installing %2...").arg(normalized, pack.id);
        m_statusByLanguage.insert(normalized, spec.statusLine);
        emit languageStatusChanged(normalized, spec.statusLine);
        return spec;
    }

    spec.ready = false;
    if (!m_allowExternalInstall && !installCommand.trimmed().isEmpty()) {
        spec.statusLine = QStringLiteral("%1: language server missing (%2), auto-install disabled; basic mode active")
                              .arg(normalized, pack.id);
    } else {
        spec.statusLine = QStringLiteral("%1: bundled language server missing (%2); basic mode active").arg(normalized, pack.id);
    }
    m_statusByLanguage.insert(normalized, spec.statusLine);
    emit languageStatusChanged(normalized, spec.statusLine);
    return spec;
}

QString LanguagePackService::statusLineForLanguage(const QString& languageId) const {
    return m_statusByLanguage.value(normalizedLanguageId(languageId));
}

QString LanguagePackService::activeServerLabel(const QString& languageId) const {
    const auto pack = findPackForLanguage(normalizedLanguageId(languageId));
    return pack.id;
}

bool LanguagePackService::loadManifest() {
    if (m_manifestLoaded) {
        return true;
    }

    QFile file(manifestPath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }
    const QJsonArray packs = doc.object().value("packs").toArray();
    for (const auto& value : packs) {
        const QJsonObject obj = value.toObject();
        PackDefinition pack;
        pack.id = obj.value("id").toString();
        pack.version = obj.value("version").toString();
        for (const auto& lang : obj.value("languages").toArray()) {
            const QString normalized = normalizedLanguageId(lang.toString());
            if (!normalized.isEmpty()) {
                pack.languages.push_back(normalized);
            }
        }
        const QJsonObject bundled = obj.value("bundled").toObject();
        pack.bundledWindowsRelativePath = bundled.value("windows").toString();
        pack.bundledLinuxRelativePath = bundled.value("linux").toString();
        for (const auto& arg : obj.value("args").toArray()) {
            const QString value = arg.toString().trimmed();
            if (!value.isEmpty()) {
                pack.defaultArgs.push_back(value);
            }
        }
        for (const auto& candidate : obj.value("pathCandidates").toArray()) {
            const QString value = candidate.toString().trimmed();
            if (!value.isEmpty()) {
                pack.pathCandidates.push_back(value);
            }
        }
        const QJsonObject install = obj.value("install").toObject();
        pack.installCommandWindows = install.value("windows").toString();
        pack.installCommandLinux = install.value("linux").toString();
        if (!pack.id.isEmpty() && !pack.languages.isEmpty()) {
            m_packs.push_back(pack);
        }
    }

    m_manifestLoaded = true;
    return true;
}

QString LanguagePackService::manifestPath() const {
    const QString appManifest = QDir(QCoreApplication::applicationDirPath())
                                    .filePath(QStringLiteral("language-packs/manifest.json"));
    if (QFileInfo::exists(appManifest)) {
        return appManifest;
    }

    if (QFileInfo::exists(QStringLiteral("resources/language-packs/manifest.json"))) {
        return QStringLiteral("resources/language-packs/manifest.json");
    }

    return QStringLiteral(":/language-packs/manifest.json");
}

QString LanguagePackService::appBundledPathFor(const PackDefinition& pack) const {
    QString relativePath;
#ifdef Q_OS_WIN
    relativePath = pack.bundledWindowsRelativePath;
#else
    relativePath = pack.bundledLinuxRelativePath;
#endif

    if (relativePath.isEmpty()) {
        return {};
    }
    return QDir(QCoreApplication::applicationDirPath()).filePath(relativePath);
}

QString LanguagePackService::developerOverrideCommand(const QString& languageId) const {
    const QString key = QStringLiteral("LOCALCODEIDE_LSP_%1_COMMAND").arg(normalizedLanguageForEnvKey(languageId));
    return qEnvironmentVariable(key.toUtf8().constData());
}

QStringList LanguagePackService::developerOverrideArgs(const QString& languageId) const {
    const QString key = QStringLiteral("LOCALCODEIDE_LSP_%1_ARGS").arg(normalizedLanguageForEnvKey(languageId));
    return splitArgs(qEnvironmentVariable(key.toUtf8().constData()));
}

QString LanguagePackService::findCandidateCommand(const PackDefinition& pack) const {
    const QString bundled = appBundledPathFor(pack);
    if (!bundled.isEmpty() && QFileInfo::exists(bundled)) {
        return QFileInfo(bundled).absoluteFilePath();
    }

    if (!m_allowPathFallback) {
        return {};
    }

    for (const QString& candidate : pack.pathCandidates) {
        const QString found = QStandardPaths::findExecutable(candidate);
        if (!found.isEmpty()) {
            return found;
        }
    }

    // Per-user install location fallback for future async installs.
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty()) {
        return {};
    }
    const QString fallback = QDir(base).filePath(QStringLiteral("language-packs/%1/%2").arg(pack.id, pack.id));
    return QFileInfo::exists(fallback) ? fallback : QString{};
}

LanguagePackService::PackDefinition LanguagePackService::findPackForLanguage(const QString& languageId) const {
    for (const auto& pack : m_packs) {
        if (pack.languages.contains(languageId)) {
            return pack;
        }
    }
    return {};
}

void LanguagePackService::startInstall(const QString& languageId, const PackDefinition& pack) {
    auto& state = m_installStates[languageId];
    if (state.installing) {
        return;
    }
    const QString installCommand =
#ifdef Q_OS_WIN
        pack.installCommandWindows;
#else
        pack.installCommandLinux;
#endif

    if (installCommand.trimmed().isEmpty()) {
        return;
    }

    state.installing = true;
    state.attempted = true;
    state.status = QStringLiteral("%1: installing %2...").arg(languageId, pack.id);
    state.process = new QProcess(this);
    state.process->setWorkingDirectory(QDir::homePath());

#ifdef Q_OS_WIN
    state.process->setProgram(QStringLiteral("cmd"));
    state.process->setArguments({QStringLiteral("/C"), installCommand});
#else
    state.process->setProgram(QStringLiteral("/bin/sh"));
    state.process->setArguments({QStringLiteral("-lc"), installCommand});
#endif

    emit languageToastRequested(QStringLiteral("Installing %1 language pack...").arg(pack.id));
    emit languageStatusChanged(languageId, state.status);

    connect(state.process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
            [this, languageId, pack](int exitCode, QProcess::ExitStatus) {
        auto& localState = m_installStates[languageId];
        localState.installing = false;
        if (localState.process) {
            localState.process->deleteLater();
            localState.process = nullptr;
        }
        const QString resolved = findCandidateCommand(pack);
        if (exitCode == 0 && !resolved.isEmpty()) {
            localState.status = QStringLiteral("%1: LSP ready (%2)").arg(languageId, pack.id);
            emit languageToastRequested(QStringLiteral("%1 language pack is ready.").arg(pack.id));
        } else {
            localState.status = QStringLiteral("%1: install failed for %2; basic mode active.").arg(languageId, pack.id);
            emit languageToastRequested(QStringLiteral("Failed to install %1 pack. Basic mode remains active.").arg(pack.id));
        }
        m_statusByLanguage.insert(languageId, localState.status);
        emit languageStatusChanged(languageId, localState.status);
    });

    connect(state.process, &QProcess::errorOccurred, this, [this, languageId, pack](QProcess::ProcessError) {
        auto& localState = m_installStates[languageId];
        localState.installing = false;
        if (localState.process) {
            localState.process->deleteLater();
            localState.process = nullptr;
        }
        localState.status = QStringLiteral("%1: unable to run installer for %2; basic mode active.").arg(languageId, pack.id);
        m_statusByLanguage.insert(languageId, localState.status);
        emit languageToastRequested(QStringLiteral("Could not start installer for %1.").arg(pack.id));
        emit languageStatusChanged(languageId, localState.status);
    });

    state.process->start();
}

QString LanguagePackService::normalizedLanguageId(const QString& languageId) const {
    return languageId.trimmed().toLower();
}

} // namespace ide::services
