import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    width: 1600
    height: 1000
    minimumWidth: 1024
    minimumHeight: 640
    visible: true
    title: qsTr("xpscenery — skeleton")

    // Dark theme palette (placeholder — will be replaced by design tokens)
    palette {
        window:     "#0E1116"
        windowText: "#E6E6E6"
        base:       "#14181F"
        alternateBase: "#1A1F27"
        text:       "#E6E6E6"
        button:     "#1A1F27"
        buttonText: "#E6E6E6"
        highlight:  "#3B82F6"
        highlightedText: "#FFFFFF"
    }

    menuBar: MenuBar {
        Menu {
            title: qsTr("&File")
            Action { text: qsTr("&New project…");  shortcut: StandardKey.New }
            Action { text: qsTr("&Open project…"); shortcut: StandardKey.Open }
            MenuSeparator {}
            Action { text: qsTr("E&xit"); onTriggered: Qt.quit() }
        }
        Menu {
            title: qsTr("&Edit")
            Action { text: qsTr("Undo"); shortcut: StandardKey.Undo }
            Action { text: qsTr("Redo"); shortcut: StandardKey.Redo }
        }
        Menu {
            title: qsTr("&View")
            Action { text: qsTr("Command palette…"); shortcut: "Ctrl+Shift+P" }
        }
        Menu {
            title: qsTr("&Help")
            Action { text: qsTr("About xpscenery"); }
        }
    }

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 8

            Label {
                text: "xpscenery"
                font.bold: true
                font.pixelSize: 14
            }
            Label {
                text: "0.1.0-alpha.0"
                color: "#808080"
                font.pixelSize: 12
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("Qt " + qtVersion())
                color: "#606060"
                font.pixelSize: 11
            }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // -------- Left panel: layers + project tree --------
        Rectangle {
            SplitView.preferredWidth: 280
            SplitView.minimumWidth:   180
            color: palette.alternateBase

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Label {
                    text: qsTr("Layers")
                    font.bold: true
                    font.pixelSize: 13
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: palette.base
                    border.color: "#2A2F37"
                    border.width: 1
                    radius: 4
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("(layer list — empty)")
                        color: "#606060"
                    }
                }
            }
        }

        // -------- Center: map placeholder --------
        MapPlaceholder {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 300
        }

        // -------- Right panel: properties --------
        Rectangle {
            SplitView.preferredWidth: 320
            SplitView.minimumWidth:   200
            color: palette.alternateBase

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                Label {
                    text: qsTr("Properties")
                    font.bold: true
                    font.pixelSize: 13
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: palette.base
                    border.color: "#2A2F37"
                    border.width: 1
                    radius: 4
                    Label {
                        anchors.centerIn: parent
                        text: qsTr("(nothing selected)")
                        color: "#606060"
                    }
                }
            }
        }
    }

    footer: StatusBar {}

    function qtVersion() { return Qt.application.version; }
}
