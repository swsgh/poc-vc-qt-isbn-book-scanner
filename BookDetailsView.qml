import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Pane {
    id: root

    property var selection: null

    padding: 14

    ScrollView {
        id: detailsScrollView
        anchors.fill: parent
        anchors.bottomMargin: removeButton.height + 8
        clip: true
        contentWidth: availableWidth
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        Column {
            width: detailsScrollView.availableWidth
            spacing: 12

        Row {
            width: parent.width

            Label {
                width: parent.width - closeButton.width
                text: "Book details"
                font.pixelSize: 16
                font.bold: true
                verticalAlignment: Text.AlignVCenter
            }

            ToolButton {
                id: closeButton
                text: "×"
                Accessible.name: "Close book details"
                onClicked: if (root.selection) root.selection.clearSelectedBook()
            }
        }

        Item {
            width: parent.width
            height: 285

            Item {
                anchors.centerIn: parent
                width: 200
                height: 275

                Image {
                    id: coverImage
                    anchors.fill: parent
                    source: root.selection ? root.selection.selectedBookCoverSource : ""
                    fillMode: Image.PreserveAspectFit
                    visible: status === Image.Ready
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: coverImage.status === Image.Ready
                    cursorShape: Qt.PointingHandCursor
                    onClicked: coverZoom.open()
                }

                Label {
                    anchors.fill: parent
                    anchors.margins: 10
                    text: root.selection ? root.selection.selectedBookTitle : ""
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    visible: coverImage.status !== Image.Ready
                }
            }
        }

        Label {
            width: parent.width
            text: root.selection ? root.selection.selectedBookTitle : ""
            font.pixelSize: 15
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Label {
            width: parent.width
            text: root.selection ? (root.selection.selectedBookAuthors || "Unknown") : "Unknown"
            font.italic: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Label {
            width: parent.width
            text: {
                if (!root.selection) return ""
                const lines = []
                if (root.selection.selectedBookPublicationDate.length > 0)
                    lines.push("First published: " + root.selection.selectedBookPublicationDate)
                if (root.selection.selectedBookPublisher.length > 0)
                    lines.push("Publisher: " + root.selection.selectedBookPublisher)
                if (root.selection.selectedBookPageCount > 0)
                    lines.push("Pages: " + root.selection.selectedBookPageCount)
                return lines.join("\n")
            }
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: text.length > 0
        }

        Label {
            width: parent.width
            text: "ISBN: " + (root.selection ? root.selection.selectedBookIsbn : "")
            font.family: "monospace"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAnywhere
        }

        }
    }

    Popup {
        id: coverZoom
        parent: Overlay.overlay
        x: 0
        y: 0
        width: parent ? parent.width : 0
        height: parent ? parent.height : 0
        modal: true
        dim: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: "#e0101010"
        }

        contentItem: Item {
            Image {
                anchors.fill: parent
                anchors.margins: 32
                source: root.selection ? root.selection.selectedBookCoverSource : ""
                fillMode: Image.PreserveAspectFit
            }

            ToolButton {
                anchors.top: parent.top
                anchors.right: parent.right
                text: "×"
                Accessible.name: "Close enlarged cover"
                onClicked: coverZoom.close()
            }
        }
    }

    Button {
        id: removeButton
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: "Remove book"
        onClicked: removeConfirmation.open()
    }

    Dialog {
        id: removeConfirmation

        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(380, parent.width - 32)
        modal: true
        title: "Remove book"

        contentItem: Label {
            text: "Remove \"" + (root.selection ? root.selection.selectedBookTitle : "")
                + "\" from your collection?"
            wrapMode: Text.WordWrap
        }

        footer: RowLayout {
            Button {
                text: "Cancel"
                onClicked: removeConfirmation.reject()
            }

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: "Remove"
                onClicked: {
                    removeConfirmation.accept()
                    if (root.selection) {
                        root.selection.removeSelectedBook()
                    }
                }
            }
        }
    }
}