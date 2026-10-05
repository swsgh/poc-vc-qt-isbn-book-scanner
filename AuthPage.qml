import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Page {
    id: root

    property var appController: null
    property bool registering: false
    property bool submissionPending: false
    signal closeRequested()

    padding: 12
    palette.window: "#F1F4F5"
    palette.windowText: "#202A30"
    palette.base: "#FFFFFF"
    palette.alternateBase: "#E8EFF0"
    palette.text: "#202A30"
    palette.placeholderText: "#68767C"
    palette.button: "#E2E9E8"
    palette.buttonText: "#202A30"
    palette.highlight: "#176B5E"
    palette.highlightedText: "#FFFFFF"
    Material.accent: "#176B5E"
    background: Rectangle {
        color: "#F1F4F5"
    }

    Component.onCompleted: {
        serverUrl.text = appController ? appController.defaultSyncServerUrl() : ""
        username.text = registering || !appController
            ? "" : appController.rememberedSyncUsername()
        password.text = ""
        confirmation.text = ""
        rememberUsername.checked = appController
            ? appController.shouldRememberSyncUsername() : false
    }

    function focusInitialField() {
        const field = !root.registering && username.text.length > 0
            ? password : username
        field.forceActiveFocus()
        Qt.inputMethod.show()
    }

    function closePage() {
        Qt.inputMethod.hide()
        root.closeRequested()
    }

    StackView.onStatusChanged: {
        if (StackView.status === StackView.Active) {
        Qt.callLater(function() {
                if (root.StackView.status === StackView.Active) {
                    root.focusInitialField()
                }
        })
        }
    }

    Component.onDestruction: {
        if (submissionPending && appController) {
            appController.cancelSyncCredentialsSubmission()
        }
    }

    Connections {
        target: root.appController

        function onSyncCredentialsSubmissionFinished(success, message) {
            if (!root.submissionPending) {
                return
            }
            root.submissionPending = false
            submitButton.enabled = true
            if (success) {
                root.closePage()
            } else {
                errorText.text = message
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            ToolButton {
                text: "Back"
                onClicked: root.closePage()
            }
        }

        ScrollView {
            id: formScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: formScroll.width
                spacing: 10

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                }

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
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Button {
                text: "Cancel"
                onClicked: root.closePage()
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
                        root.submissionPending = false
                        root.closePage()
                    }
                }
            }
        }
    }
}