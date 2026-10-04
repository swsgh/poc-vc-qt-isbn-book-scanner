import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

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
                onClicked: bookSelection.clearSelectedBook()
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
                    source: bookSelection.selectedBookCoverSource
                    fillMode: Image.PreserveAspectFit
                    visible: status === Image.Ready
                }

                Text {
                    anchors.fill: parent
                    anchors.margins: 12
                    text: bookSelection.selectedBookTitle
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
            text: bookSelection.selectedBookTitle
            color: systemPalette.windowText
            font.pixelSize: 15
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Text {
            width: parent.width
            text: bookSelection.selectedBookAuthors || "Unknown"
            color: systemPalette.windowText
            font.italic: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Text {
            width: parent.width
            text: bookSelection.selectedBookMetadata
            color: systemPalette.windowText
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: text.length > 0
        }

        Text {
            width: parent.width
            text: "ISBN: " + bookSelection.selectedBookIsbn
            color: systemPalette.windowText
            font.family: "monospace"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAnywhere
        }

        Button {
            width: parent.width
            text: "Remove book"
            onClicked: bookSelection.removeSelectedBook()
        }
    }
}