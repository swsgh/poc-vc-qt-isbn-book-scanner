import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property var appController: null
    property bool registering: false

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    modal: true
    width: 420
    title: registering ? "Register Sync Account" : "Log In to Sync"

    onOpened: {
        errorText.text = ""
        serverUrl.text = appController ? appController.defaultSyncServerUrl() : ""
        username.text = registering || !appController
            ? "" : appController.rememberedSyncUsername()
        password.text = ""
        confirmation.text = ""
        rememberUsername.checked = appController
            ? appController.shouldRememberSyncUsername() : false
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
                if (!root.appController) {
                    errorText.text = "Account actions are unavailable."
                    return
                }
                const error = root.appController.submitSyncCredentials(
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