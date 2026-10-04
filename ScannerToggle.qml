import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var scannerActions: null

    implicitWidth: 176
    implicitHeight: 36

    Button {
        anchors.fill: parent
        text: root.scannerActions && root.scannerActions.scannerVisible
            ? "Hide Camera Preview" : "Show Camera Preview"
        Accessible.name: "Camera preview toggle"
        onClicked: {
            if (root.scannerActions) {
                root.scannerActions.toggleScannerPanel()
            }
        }
    }
}