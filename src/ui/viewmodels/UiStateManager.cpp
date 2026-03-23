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

void UiStateManager::load() {
    m_state.primaryViewIndex = qBound(0, m_settings.value(QStringLiteral("workbench/primaryViewIndex"), 0).toInt(), 2);
    m_state.secondaryAiVisible = m_settings.value(QStringLiteral("workbench/secondaryAiVisible"), true).toBool();
    m_state.secondaryAiTab = qBound(0, m_settings.value(QStringLiteral("workbench/secondaryAiTab"), 0).toInt(), 1);
    m_state.secondaryAiWidth = qBound(320, m_settings.value(QStringLiteral("workbench/secondaryAiWidth"), 390).toInt(), 720);
    emit primaryViewIndexChanged();
    emit secondaryAiChanged();
}

void UiStateManager::save() {
    m_settings.setValue(QStringLiteral("workbench/primaryViewIndex"), m_state.primaryViewIndex);
    m_settings.setValue(QStringLiteral("workbench/secondaryAiVisible"), m_state.secondaryAiVisible);
    m_settings.setValue(QStringLiteral("workbench/secondaryAiTab"), m_state.secondaryAiTab);
    m_settings.setValue(QStringLiteral("workbench/secondaryAiWidth"), m_state.secondaryAiWidth);
    m_settings.sync();
}

} // namespace ide::ui::viewmodels
