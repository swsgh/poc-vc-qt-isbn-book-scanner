import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    property var selection: null

    color: systemPalette.base
    border.color: systemPalette.mid
    radius: 6

    SystemPalette {
        id: systemPalette
    }

    Column {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        Row {
            width: parent.width

            Text {
                width: parent.width - closeButton.width
                text: "Book details"
                color: systemPalette.windowText
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
            height: 190

            Rectangle {
                anchors.centerIn: parent
                width: 136
                height: 180
                color: systemPalette.alternateBase
                border.color: systemPalette.mid
                radius: 3

                Image {
                    id: coverImage
                    anchors.fill: parent
                    anchors.margins: 2
                    source: root.selection ? root.selection.selectedBookCoverSource : ""
                    fillMode: Image.PreserveAspectFit
                    visible: status === Image.Ready
                }

                Text {
                    anchors.fill: parent
                    anchors.margins: 12
                    text: root.selection ? root.selection.selectedBookTitle : ""
                    color: systemPalette.windowText
                    font.pixelSize: 13
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap
                    visible: coverImage.status !== Image.Ready
                }
            }
        }

        Text {
            width: parent.width
            text: root.selection ? root.selection.selectedBookTitle : ""
            color: systemPalette.windowText
            font.pixelSize: 15
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Text {
            width: parent.width
            text: root.selection ? (root.selection.selectedBookAuthors || "Unknown") : "Unknown"
            color: systemPalette.windowText
            font.italic: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Text {
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
            color: systemPalette.windowText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: text.length > 0
        }

        Text {
            width: parent.width
            text: "ISBN: " + (root.selection ? root.selection.selectedBookIsbn : "")
            color: systemPalette.windowText
            font.family: "monospace"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAnywhere
        }

        Button {
            width: parent.width
            text: "Remove book"
            onClicked: removeConfirmation.open()
        }
    }

    Dialog {
        id: removeConfirmation

        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: 380
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