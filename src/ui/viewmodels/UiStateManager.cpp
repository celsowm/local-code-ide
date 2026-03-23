#include "ui/viewmodels/UiStateManager.hpp"

#include <QDir>
#include <QStandardPaths>

namespace ide::ui::viewmodels {

namespace {
QString defaultUiStateSettingsPath() {
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (base.isEmpty()) {
        base = QDir::homePath() + QStringLiteral("/.localcodeide");
    }
    QDir().mkpath(base);
    return QDir(base).filePath(QStringLiteral("ui.ini"));
}
}

UiStateManager::UiStateManager(const QString& settingsPath, QObject* parent)
    : QObject(parent)
    , m_settings(settingsPath.isEmpty() ? defaultUiStateSettingsPath() : settingsPath, QSettings::IniFormat) {
}

int UiStateManager::primaryViewIndex() const {
    return m_state.primaryViewIndex;
}

void UiStateManager::setPrimaryViewIndex(int value) {
    const int bounded = qBound(0, value, 2);
    if (bounded == m_state.primaryViewIndex) return;
    m_state.primaryViewIndex = bounded;
    emit primaryViewIndexChanged();
}

bool UiStateManager::secondaryAiVisible() const {
    return m_state.secondaryAiVisible;
}

void UiStateManager::setSecondaryAiVisible(bool value) {
    if (value == m_state.secondaryAiVisible) return;
    m_state.secondaryAiVisible = value;
    emit secondaryAiChanged();
}

int UiStateManager::secondaryAiTab() const {
    return m_state.secondaryAiTab;
}

void UiStateManager::setSecondaryAiTab(int value) {
    const int bounded = qBound(0, value, 1);
    if (bounded == m_state.secondaryAiTab) return;
    m_state.secondaryAiTab = bounded;
    emit secondaryAiChanged();
}

int UiStateManager::secondaryAiWidth() const {
    return m_state.secondaryAiWidth;
}

void UiStateManager::setSecondaryAiWidth(int value) {
    const int bounded = qBound(320, value, 720);
    if (bounded == m_state.secondaryAiWidth) return;
    m_state.secondaryAiWidth = bounded;
    emit secondaryAiChanged();
}

int UiStateManager::bottomPanelTab() const {
    return m_state.bottomPanelTab;
}

void UiStateManager::setBottomPanelTab(int value) {
    const int bounded = qBound(0, value, 1);
    if (bounded == m_state.bottomPanelTab) return;
    m_state.bottomPanelTab = bounded;
    emit bottomPanelChanged();
}

int UiStateManager::bottomPanelHeight() const {
    return m_state.bottomPanelHeight;
}

void UiStateManager::setBottomPanelHeight(int value) {
    const int bounded = qBound(190, value, 560);
    if (bounded == m_state.bottomPanelHeight) return;
    m_state.bottomPanelHeight = bounded;
    emit bottomPanelChanged();
}

QStringList UiStateManager::recentFolders() const {
    return m_state.recentFolders;
}

void UiStateManager::addRecentFolder(const QString& folderPath) {
    const QString normalizedPath = QDir::cleanPath(folderPath.trimmed());
    if (normalizedPath.isEmpty()) {
        return;
    }

    QStringList updated;
    updated << normalizedPath;
    for (const QString& existing : m_state.recentFolders) {
        if (QDir::cleanPath(existing) == normalizedPath) {
            continue;
        }
        if (!existing.trimmed().isEmpty()) {
            updated << QDir::cleanPath(existing);
        }
        if (updated.size() >= 8) {
            break;
        }
    }
    if (updated == m_state.recentFolders) {
        return;
    }
    m_state.recentFolders = updated;
    emit recentFoldersChanged();
}

void UiStateManager::removeRecentFolder(const QString& folderPath) {
    const QString normalizedPath = QDir::cleanPath(folderPath.trimmed());
    if (normalizedPath.isEmpty()) {
        return;
    }

    QStringList updated;
    for (const QString& existing : m_state.recentFolders) {
        if (QDir::cleanPath(existing) == normalizedPath) {
            continue;
        }
        updated << QDir::cleanPath(existing);
    }
    if (updated == m_state.recentFolders) {
        return;
    }
    m_state.recentFolders = updated;
    emit recentFoldersChanged();
}

void UiStateManager::load() {
    m_state.primaryViewIndex = qBound(0, m_settings.value(QStringLiteral("workbench/primaryViewIndex"), 0).toInt(), 2);
    m_state.secondaryAiVisible = m_settings.value(QStringLiteral("workbench/secondaryAiVisible"), true).toBool();
    m_state.secondaryAiTab = qBound(0, m_settings.value(QStringLiteral("workbench/secondaryAiTab"), 0).toInt(), 1);
    m_state.secondaryAiWidth = qBound(320, m_settings.value(QStringLiteral("workbench/secondaryAiWidth"), 390).toInt(), 720);
    m_state.bottomPanelTab = qBound(0, m_settings.value(QStringLiteral("workbench/bottomPanelTab"), 0).toInt(), 1);
    m_state.bottomPanelHeight = qBound(190, m_settings.value(QStringLiteral("workbench/bottomPanelHeight"), 300).toInt(), 560);
    QStringList loadedRecents = m_settings.value(QStringLiteral("workbench/recentFolders")).toStringList();
    QStringList sanitized;
    for (const QString& item : loadedRecents) {
        const QString cleaned = QDir::cleanPath(item.trimmed());
        if (cleaned.isEmpty() || sanitized.contains(cleaned)) {
            continue;
        }
        sanitized << cleaned;
        if (sanitized.size() >= 8) {
            break;
        }
    }
    m_state.recentFolders = sanitized;
    emit primaryViewIndexChanged();
    emit secondaryAiChanged();
    emit bottomPanelChanged();
    emit recentFoldersChanged();
}

void UiStateManager::save() {
    m_settings.setValue(QStringLiteral("workbench/primaryViewIndex"), m_state.primaryViewIndex);
    m_settings.setValue(QStringLiteral("workbench/secondaryAiVisible"), m_state.secondaryAiVisible);
    m_settings.setValue(QStringLiteral("workbench/secondaryAiTab"), m_state.secondaryAiTab);
    m_settings.setValue(QStringLiteral("workbench/secondaryAiWidth"), m_state.secondaryAiWidth);
    m_settings.setValue(QStringLiteral("workbench/bottomPanelTab"), m_state.bottomPanelTab);
    m_settings.setValue(QStringLiteral("workbench/bottomPanelHeight"), m_state.bottomPanelHeight);
    m_settings.setValue(QStringLiteral("workbench/recentFolders"), m_state.recentFolders);
    m_settings.sync();
}

} // namespace ide::ui::viewmodels
