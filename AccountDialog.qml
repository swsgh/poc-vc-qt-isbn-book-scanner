import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property var mainWindow: null
    property bool registering: false

    modal: true
    width: 420
    title: registering ? "Register Sync Account" : "Log In to Sync"

    onOpened: {
        errorText.text = ""
        serverUrl.text = mainWindow ? mainWindow.defaultSyncServerUrl() : ""
        username.text = registering || !mainWindow
            ? "" : mainWindow.rememberedSyncUsername()
        password.text = ""
        confirmation.text = ""
        rememberUsername.checked = mainWindow
            ? mainWindow.shouldRememberSyncUsername() : false
    }

    contentItem: ColumnLayout {
        spacing: 8

        TextField {
            id: serverUrl
            Layout.fillWidth: true
            placeholderText: "Server URL"
            inputMethodHints: Qt.ImhUrlCharactersOnly
        }

        TextField {
            id: username
            Layout.fillWidth: true
            placeholderText: "Username"
            autocomplete: false
        }

        TextField {
            id: password
            Layout.fillWidth: true
            placeholderText: "Password"
            echoMode: TextInput.Password
        }

        TextField {
            id: confirmation
            Layout.fillWidth: true
            placeholderText: "Confirm password"
            echoMode: TextInput.Password
            visible: root.registering
        }

        CheckBox {
            id: rememberUsername
            text: "Remember username"
            visible: !root.registering
        }

        Label {
            id: errorText
            Layout.fillWidth: true
            color: "#d64f4f"
            wrapMode: Text.WordWrap
            visible: text.length > 0
        }
    }

    footer: RowLayout {
        Button {
            text: "Cancel"
            onClicked: root.reject()
        }

        Item { Layout.fillWidth: true }

        Button {
            text: root.registering ? "Register" : "Log In"
            onClicked: {
                if (!root.mainWindow) {
                    errorText.text = "Account actions are unavailable."
                    return
                }
                const error = root.mainWindow.submitSyncCredentials(
                    root.registering, serverUrl.text, username.text, password.text,
                    confirmation.text, rememberUsername.checked)
                if (error.length > 0) {
                    errorText.text = error
                } else {
                    root.accept()
                }
            }
        }
    }
}