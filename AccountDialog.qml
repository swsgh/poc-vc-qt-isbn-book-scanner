import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property var appController: null
    property bool registering: false
    property bool submissionPending: false

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    modal: true
    width: 420
    title: registering ? "Register Sync Account" : "Log In to Sync"

    onOpened: {
        errorText.text = ""
        submissionPending = false
        submitButton.enabled = true
        serverUrl.text = appController ? appController.defaultSyncServerUrl() : ""
        username.text = registering || !appController
            ? "" : appController.rememberedSyncUsername()
        password.text = ""
        confirmation.text = ""
        rememberUsername.checked = appController
            ? appController.shouldRememberSyncUsername() : false
    }

    onRejected: {
        if (submissionPending && appController) {
            appController.cancelSyncCredentialsSubmission()
        }
        submissionPending = false
    }

    Connections {
        target: root.appController

        function onSyncCredentialsSubmissionFinished(success, message) {
            if (!root.opened || !root.submissionPending) {
                return
            }
            root.submissionPending = false
            submitButton.enabled = true
            if (success) {
                root.accept()
            } else {
                errorText.text = message
            }
        }
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
            id: submitButton
            text: root.registering ? "Register" : "Log In"
            enabled: !root.submissionPending
            onClicked: {
                if (!root.appController) {
                    errorText.text = "Account actions are unavailable."
                    return
                }
                const result = root.appController.submitSyncCredentials(
                    root.registering, serverUrl.text, username.text, password.text,
                    confirmation.text, rememberUsername.checked)
                if (result.status === "error") {
                    errorText.text = result.message
                } else if (result.status === "pending") {
                    root.submissionPending = true
                    submitButton.enabled = false
                } else if (result.status === "accepted") {
                    root.accept()
                }
            }
        }
    }
}