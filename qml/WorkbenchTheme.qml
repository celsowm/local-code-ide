pragma Singleton

import QtQuick

QtObject {
    readonly property color windowBackground: "#1e1e1e"
    readonly property color panelBackground: "#181818"
    readonly property color elevatedBackground: "#252526"
    readonly property color borderColor: "#2a2d2e"
    readonly property color subtleBorderColor: "#31353a"

    readonly property color textPrimary: "#cccccc"
    readonly property color textMuted: "#8b949e"
    readonly property color textDim: "#6b7179"
    readonly property color accent: "#007acc"
    readonly property color accentSoft: "#0e639c"
    readonly property color selection: "#264f78"
    readonly property color danger: "#f14c4c"
    readonly property color warning: "#cca700"
    readonly property color info: "#4fc1ff"
    readonly property color success: "#73c991"

    readonly property color activityBarBackground: "#181818"
    readonly property color sideBarBackground: "#1f1f1f"
    readonly property color editorBackground: "#1e1e1e"
    readonly property color editorAltBackground: "#202124"
    readonly property color editorTabInactive: "#2d2d30"
    readonly property color editorTabActive: "#1e1e1e"
    readonly property color editorHeaderBackground: "#252526"
    readonly property color editorGutterBackground: "#1b1b1b"
    readonly property color editorGutterText: "#858585"
    readonly property color panelHeaderBackground: "#252526"
    readonly property color terminalBackground: "#111111"

    readonly property string uiFont: "Segoe UI"
    readonly property string monoFont: "Cascadia Code"
    readonly property int tabHeight: 33
    readonly property int compactHeaderHeight: 28
    readonly property int panelHeaderHeight: 32
    readonly property int editorToolbarHeight: 30
    readonly property int gutterWidth: 52
    readonly property int defaultBottomPanelHeight: 300
    readonly property int minBottomPanelHeight: 190
    readonly property int maxBottomPanelHeight: 560
}
