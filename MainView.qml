import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ISBNBookScanner

ApplicationWindow {
    id: root

    property var appController: null
    property var bookCollection: null
    property var scannerController: null

    visible: true
    width: 950
    height: 900
    minimumWidth: 720
    minimumHeight: 600
    title: "ISBN Book Scanner"

    SystemPalette {
        id: systemPalette
    }

    color: systemPalette.window

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            ScannerToggle {
                Layout.preferredWidth: 176
                Layout.preferredHeight: 36
                appController: root.appController
            }

            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                radius: 6
                color: root.appController && root.appController.syncServerReachable
                    ? "#2f9e62" : "#d64f4f"

                ToolTip.visible: indicatorHover.hovered
                ToolTip.text: root.appController && root.appController.syncServerReachable
                    ? "Sync server is reachable" : "Sync server is unreachable"

                HoverHandler {
                    id: indicatorHover
                }
            }

            SettingsMenu {
                Layout.preferredWidth: 42
                Layout.preferredHeight: 36
                appController: root.appController
            }
        }

        ScannerControls {
            Layout.fillWidth: true
            visible: root.appController ? root.appController.scannerVisible : false
            scannerController: root.scannerController
            appController: root.appController
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                BookshelfView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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
                Layout.preferredWidth: 290
                Layout.fillHeight: true
                visible: root.appController ? root.appController.selectedBookVisible : false
                selection: root.appController
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 22 : 0
            text: root.appController ? root.appController.applicationStatusText : ""
            color: root.appController && root.appController.applicationStatusIsError
                ? "#ff6b6b" : systemPalette.windowText
            font.pixelSize: 12
            visible: text.length > 0
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }
}