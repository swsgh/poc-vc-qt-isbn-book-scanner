import QtQuick
import QtQuick.Controls

Item {
    id: root

    property var appController: null

    implicitWidth: 44
    implicitHeight: 44

    ToolButton {
        anchors.fill: parent
        text: "📷"
        Accessible.name: root.appController && root.appController.scannerVisible
            ? "Hide camera preview" : "Show camera preview"
        ToolTip.visible: hovered
        ToolTip.text: root.appController && root.appController.scannerVisible
            ? "Hide camera preview" : "Show camera preview"
        onClicked: {
            if (root.appController) {
                root.appController.toggleScannerPanel()
            }
        }
    }
}