import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtCore

Item {
    id: root

    property var mainWindow: null

    ToolButton {
        id: settingsButton
        anchors.fill: parent
        text: "⚙"
        Accessible.name: "Settings and bookshelf actions"
        ToolTip.visible: hovered
        ToolTip.text: "Settings and bookshelf actions"
        onClicked: settingsMenu.popup(settingsButton, 0, settingsButton.height)
    }

    Menu {
        id: settingsMenu

        MenuItem {
            text: "Log In to Sync..."
            enabled: root.mainWindow && !root.mainWindow.syncAuthenticated
            onTriggered: {
                loginDialog.registering = false
                loginDialog.open()
            }
        }
        MenuItem {
            text: "Sync Now"
            enabled: root.mainWindow && root.mainWindow.syncAuthenticated
            onTriggered: root.mainWindow.syncNow()
        }
        MenuItem {
            text: "Log Out of Sync"
            enabled: root.mainWindow && root.mainWindow.syncAuthenticated
            onTriggered: root.mainWindow.logoutSync()
        }

        MenuSeparator {}

        MenuItem {
            text: "Register Sync Account..."
            enabled: root.mainWindow && !root.mainWindow.syncAuthenticated
            onTriggered: {
                registerDialog.registering = true
                registerDialog.open()
            }
        }

        MenuSeparator {}

        MenuItem {
            text: "Import CSV..."
            onTriggered: importDialog.open()
        }
        MenuItem {
            text: "Export CSV..."
            onTriggered: exportDialog.open()
        }
    }

    AccountDialog {
        id: loginDialog
        mainWindow: root.mainWindow
        registering: false
    }

    AccountDialog {
        id: registerDialog
        mainWindow: root.mainWindow
        registering: true
    }

    FileDialog {
        id: importDialog
        title: "Import Books from CSV"
        fileMode: FileDialog.OpenFile
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        nameFilters: ["CSV files (*.csv)"]
        onAccepted: showResult(root.mainWindow.importBooksCsv(selectedFile))
    }

    FileDialog {
        id: exportDialog
        title: "Export Books to CSV"
        fileMode: FileDialog.SaveFile
        currentFolder: StandardPaths.writableLocation(StandardPaths.DocumentsLocation)
        nameFilters: ["CSV files (*.csv)"]
        defaultSuffix: "csv"
        onAccepted: showResult(root.mainWindow.exportBooksCsv(selectedFile))
    }

    Dialog {
        id: resultDialog
        modal: true
        title: resultTitle
        standardButtons: Dialog.Ok

        property string resultTitle: ""
        property string resultMessage: ""

        contentItem: Label {
            text: resultDialog.resultMessage
            wrapMode: Text.WordWrap
        }
    }

    function showResult(result) {
        resultDialog.resultTitle = result.title
        resultDialog.resultMessage = result.message
        resultDialog.open()
    }
}