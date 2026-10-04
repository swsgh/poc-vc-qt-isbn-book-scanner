import QtQuick
import QtQuick.Controls

Rectangle {
    id: root

    property var collection: null
    property var selection: null

    color: systemPalette.base

    SystemPalette {
        id: systemPalette
    }

    Column {
        anchors.fill: parent
        spacing: 6

        Text {
            text: "Bookshelf"
            color: systemPalette.windowText
            font.pixelSize: 14
            font.bold: true
            height: 22
            verticalAlignment: Text.AlignVCenter
        }

        GridView {
            id: bookGrid

            width: parent.width
            height: parent.height - 28
            cellWidth: 136
            cellHeight: 160
            clip: true
            model: root.collection

            delegate: Item {
                required property string isbn
                required property string title
                required property url coverSource

                width: bookGrid.cellWidth
                height: bookGrid.cellHeight

                Column {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 3

                    Rectangle {
                        width: 110
                        height: 132
                        color: systemPalette.alternateBase
                        border.color: systemPalette.mid
                        radius: 2

                        Image {
                            id: coverImage

                            anchors.fill: parent
                            anchors.margins: 2
                            source: coverSource
                            fillMode: Image.PreserveAspectFit
                            visible: status === Image.Ready
                        }

                        Text {
                            anchors.fill: parent
                            anchors.margins: 8
                            text: title
                            color: systemPalette.windowText
                            font.pixelSize: 11
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            maximumLineCount: 8
                            elide: Text.ElideRight
                            visible: coverImage.status !== Image.Ready
                        }
                    }

                    Text {
                        width: 116
                        text: title
                        color: systemPalette.windowText
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.selection) {
                            root.selection.selectBook(isbn)
                        }
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {}

            Text {
                anchors.centerIn: parent
                text: "No books found"
                color: systemPalette.placeholderText
                visible: bookGrid.count === 0
            }
        }
    }
}