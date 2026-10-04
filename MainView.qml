import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ISBNBookScanner

Item {
    id: root

    property var mainWindow: null
    property var bookCollection: null
    property var scannerController: null

    SystemPalette {
        id: systemPalette
    }

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
                scannerActions: root.mainWindow
            }

            Item {
                Layout.fillWidth: true
            }

            Rectangle {
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                radius: 6
                color: root.mainWindow && root.mainWindow.syncServerReachable
                    ? "#2f9e62" : "#d64f4f"

                ToolTip.visible: indicatorHover.hovered
                ToolTip.text: root.mainWindow && root.mainWindow.syncServerReachable
                    ? "Sync server is reachable" : "Sync server is unreachable"

                HoverHandler {
                    id: indicatorHover
                }
            }

            SettingsMenu {
                Layout.preferredWidth: 42
                Layout.preferredHeight: 36
                mainWindow: root.mainWindow
            }
        }

        ScannerControls {
            Layout.fillWidth: true
            visible: root.mainWindow ? root.mainWindow.scannerVisible : false
            scannerController: root.scannerController
            scannerActions: root.mainWindow
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
                    selection: root.mainWindow
                }

                TextField {
                    Layout.fillWidth: true
                    placeholderText: "Type to filter by title, author, or ISBN..."
                    onTextChanged: {
                        if (root.mainWindow) {
                            root.mainWindow.setBookSearchText(text)
                        }
                    }
                }
            }

            BookDetailsView {
                Layout.preferredWidth: 290
                Layout.fillHeight: true
                visible: root.mainWindow ? root.mainWindow.selectedBookVisible : false
                selection: root.mainWindow
            }
        }
    }
}