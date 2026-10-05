import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ISBNBookScanner

ApplicationWindow {
    id: root

    property var appController: null
    property var bookCollection: null
    property var scannerController: null
    readonly property int pageMargin: 12
    readonly property int workspaceSpacing: 10
    readonly property int cameraMinimumWidth: 320

    visible: true
    width: Qt.platform.os === "android" ? Screen.width : Screen.desktopAvailableWidth * 0.85
    height: Qt.platform.os === "android" ? Screen.height : Screen.desktopAvailableHeight * 0.85
    minimumWidth: Qt.platform.os === "android" ? 0 : cameraMinimumWidth + pageMargin * 2
    title: "ISBN Book Scanner"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.pageMargin
        spacing: root.workspaceSpacing

        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: root.cameraMinimumWidth
            spacing: 8

            ScannerToggle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                appController: root.appController
            }

            SettingsMenu {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                appController: root.appController
            }

            Label {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                text: root.appController && root.appController.syncServerReachable ? "✓" : "×"
                font.pixelSize: 22
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                Accessible.name: root.appController && root.appController.syncServerReachable
                    ? "Sync server is reachable" : "Sync server is unreachable"
                ToolTip.visible: syncIndicatorHover.hovered
                ToolTip.text: Accessible.name

                HoverHandler {
                    id: syncIndicatorHover
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        ScannerControls {
            Layout.fillWidth: true
            Layout.minimumWidth: root.cameraMinimumWidth
            Layout.minimumHeight: root.appController && root.appController.scannerVisible ? 300 : 0
            Layout.preferredHeight: root.appController && root.appController.scannerVisible ? 310 : 0
            visible: root.appController ? root.appController.scannerVisible : false
            scannerController: root.scannerController
            appController: root.appController
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: root.cameraMinimumWidth
            Layout.minimumHeight: 200

            ColumnLayout {
                anchors.fill: parent
                spacing: 8
                visible: !root.appController || !root.appController.selectedBookVisible

                BookshelfView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 180
                    collection: root.bookCollection
                    selection: root.appController
                }

                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Type to filter by title, author, or ISBN..."
                    onTextChanged: {
                        if (root.appController) {
                            root.appController.setBookSearchText(text)
                        }
                    }
                }
            }

            BookDetailsView {
                anchors.fill: parent
                visible: root.appController ? root.appController.selectedBookVisible : false
                selection: root.appController
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? implicitHeight : 0
            text: root.appController ? root.appController.applicationStatusText : ""
            visible: text.length > 0
            elide: Text.ElideRight
        }
    }
}
