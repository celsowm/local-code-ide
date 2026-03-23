#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

namespace ide::ui::viewmodels {

struct UiState {
    int primaryViewIndex = 0;
    bool secondaryAiVisible = true;
    int secondaryAiTab = 0;
    int secondaryAiWidth = 390;
};

class UiStateManager final : public QObject {
    Q_OBJECT

    Q_PROPERTY(int primaryViewIndex READ primaryViewIndex WRITE setPrimaryViewIndex NOTIFY primaryViewIndexChanged)
    Q_PROPERTY(bool secondaryAiVisible READ secondaryAiVisible WRITE setSecondaryAiVisible NOTIFY secondaryAiChanged)
    Q_PROPERTY(int secondaryAiTab READ secondaryAiTab WRITE setSecondaryAiTab NOTIFY secondaryAiChanged)
    Q_PROPERTY(int secondaryAiWidth READ secondaryAiWidth WRITE setSecondaryAiWidth NOTIFY secondaryAiChanged)

public:
    explicit UiStateManager(const QString& settingsPath, QObject* parent = nullptr);

    int primaryViewIndex() const;
    void setPrimaryViewIndex(int value);

    bool secondaryAiVisible() const;
    void setSecondaryAiVisible(bool value);

    int secondaryAiTab() const;
    void setSecondaryAiTab(int value);

    int secondaryAiWidth() const;
    void setSecondaryAiWidth(int value);

    Q_INVOKABLE void load();
    Q_INVOKABLE void save();

signals:
    void primaryViewIndexChanged();
    void secondaryAiChanged();

private:
    QSettings m_settings;
    UiState m_state;
};

} // namespace ide::ui::viewmodels
