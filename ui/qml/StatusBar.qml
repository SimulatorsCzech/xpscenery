import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ToolBar {
    height: 26
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 12

        Label { text: qsTr("Ready"); font.pixelSize: 11 }
        Item  { Layout.fillWidth: true }
        Label { text: qsTr("CRS: EPSG:4326");   font.pixelSize: 11; color: "#808080" }
        Label { text: qsTr("X: —  Y: —");       font.pixelSize: 11; color: "#808080" }
        Label { text: qsTr("FPS: —");           font.pixelSize: 11; color: "#808080" }
        Label { text: qsTr("RSS: —");           font.pixelSize: 11; color: "#808080" }
    }
}
