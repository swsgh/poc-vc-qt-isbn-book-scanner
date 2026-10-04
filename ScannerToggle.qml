import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var appController: null

    implicitWidth: 176
    implicitHeight: 36

    Button {
        anchors.fill: parent
        text: root.appController && root.appController.scannerVisible
            ? "Hide Camera Preview" : "Show Camera Preview"
        Accessible.name: "Camera preview toggle"
        onClicked: {
            if (root.appController) {
                root.appController.toggleScannerPanel()
            }
        }
    }
}