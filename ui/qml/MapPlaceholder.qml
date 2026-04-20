import QtQuick
import QtQuick.Controls

Rectangle {
    color: "#0A0D12"
    border.color: "#2A2F37"
    border.width: 1

    // Diagonal-stripes placeholder — will be replaced by MapLibre / QtLocation
    Canvas {
        anchors.fill: parent
        onPaint: {
            const ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            ctx.strokeStyle = "#1A1F27";
            ctx.lineWidth = 1;
            const step = 32;
            for (let x = -height; x < width + height; x += step) {
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x + height, height);
                ctx.stroke();
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 8

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("MAP VIEW")
            font.pixelSize: 22
            font.bold: true
            color: "#3B82F6"
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("(placeholder — MapLibre / QtLocation integration pending)")
            color: "#606060"
            font.pixelSize: 12
        }
    }
}
