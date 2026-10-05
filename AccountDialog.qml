import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root

    property var appController: null
    property bool registering: false
    property bool submissionPending: false
    readonly property real dialogUsableHeight: Qt.platform.os === "android"
        && Qt.inputMethod.visible
        && Qt.inputMethod.keyboardRectangle.height > 0
        ? Math.min(parent.height, Qt.inputMethod.keyboardRectangle.y)
        : parent.height

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.max(12, Math.round((dialogUsableHeight - height) / 2))
    modal: true
    width: Math.min(420, parent.width - 32)
    title: registering ? "Register Sync Account" : ""

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
        if (Qt.platform.os === "android") {
            Qt.callLater(function() {
                if (!root.opened) {
                    return
                }
                const field = !root.registering && username.text.length > 0
                    ? password : username
                field.forceActiveFocus()
                Qt.inputMethod.show()
            })
        }
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
            wrapMode: Text.WordWrap
            visible: text.length > 0
        }
    }

    footer: RowLayout {
        Layout.topMargin: -12

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