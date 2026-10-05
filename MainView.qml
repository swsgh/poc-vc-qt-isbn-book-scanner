import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ISBNBookScanner

ApplicationWindow {
    id: root

    property var appController: null
    property var bookCollection: null
    property var scannerController: null
    property string manualEntryIsbn: ""
    readonly property bool keyboardVisible: Qt.platform.os === "android" && Qt.inputMethod.visible
    readonly property int pageMargin: 12
    readonly property int workspaceSpacing: 10
    readonly property int cameraMinimumWidth: 320

    function updateBookFilter(field) {
        if (appController) {
            const start = field.selectionStart >= 0 ? field.selectionStart : field.cursorPosition
            const end = field.selectionEnd >= 0 ? field.selectionEnd : field.cursorPosition
            const query = field.text.slice(0, start) + field.preeditText + field.text.slice(end)
            appController.setBookSearchText(query)
        }
    }

    Connections {
        target: root.appController

        function onBookLookupNotFound(isbn) {
            root.manualEntryIsbn = isbn
            manualBookDialog.open()
        }
    }

    visible: true
    width: Qt.platform.os === "android" ? Screen.width : 430
    height: Qt.platform.os === "android" ? Screen.height : 900
    minimumWidth: Qt.platform.os === "android" ? 0 : cameraMinimumWidth + pageMargin * 2
    title: "ISBN Book Scanner"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: root.pageMargin
        spacing: root.workspaceSpacing

        RowLayout {

    Dialog {
        id: manualBookDialog
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(420, parent.width - 32)
        modal: true
        title: "Add book details"

        onOpened: {
            manualTitle.clear()
            manualAuthors.clear()
            manualTitle.forceActiveFocus()
        }

        contentItem: ColumnLayout {
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: "No metadata was found for ISBN " + root.manualEntryIsbn
                    + ". Enter a title to add it manually."
                wrapMode: Text.WordWrap
            }

            TextField {
                id: manualTitle
                Layout.fillWidth: true
                placeholderText: "Title (required)"
            }

            TextField {
                id: manualAuthors
                Layout.fillWidth: true
                placeholderText: "Author (optional)"
            }
        }

        footer: DialogButtonBox {
            standardButtons: DialogButtonBox.Cancel

            Button {
                text: "Add to shelf"
                enabled: manualTitle.text.trim().length > 0
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: {
                    if (root.appController.addManualBook(
                            root.manualEntryIsbn, manualTitle.text, manualAuthors.text)) {
                        manualBookDialog.accept()
                    }
                }
            }

            onRejected: manualBookDialog.reject()
        }
    }
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
            id: scannerControls
            Layout.fillWidth: true
            Layout.minimumWidth: root.cameraMinimumWidth
            Layout.minimumHeight: visible ? contentHeight : 0
            Layout.preferredHeight: visible ? contentHeight : 0
            visible: root.appController ? root.appController.scannerVisible : false
            scannerController: root.scannerController
            appController: root.appController
        }

        Label {
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? implicitHeight : 0
            visible: !root.appController || !root.appController.selectedBookVisible
            text: "Bookshelf"
            font.pixelSize: 14
            font.bold: true
            verticalAlignment: Text.AlignVCenter
        }

        TextField {
            id: bookFilterField
            Layout.fillWidth: true
            Layout.minimumHeight: 0
            Layout.preferredHeight: visible ? implicitHeight : 0
            visible: !root.appController || !root.appController.selectedBookVisible
            placeholderText: "Type to filter by title, author, or ISBN..."
            onTextChanged: root.updateBookFilter(bookFilterField)
            onPreeditTextChanged: root.updateBookFilter(bookFilterField)
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: root.cameraMinimumWidth
            Layout.minimumHeight: root.keyboardVisible ? 0 : 200

            ColumnLayout {
                anchors.fill: parent
                spacing: 8
                visible: !root.appController || !root.appController.selectedBookVisible

                BookshelfView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: root.keyboardVisible ? 0 : 180
                    collection: root.bookCollection
                    selection: root.appController
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
