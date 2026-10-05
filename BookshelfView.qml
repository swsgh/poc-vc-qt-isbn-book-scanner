import QtQuick
import QtQuick.Controls

Pane {
    id: root

    property var collection: null
    property var selection: null

    padding: 0

    Column {
        anchors.fill: parent
        spacing: 6

        Label {
            text: "Bookshelf"
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
            flow: height < cellHeight * 2 ? GridView.TopToBottom : GridView.LeftToRight

            delegate: Item {
                required property string isbn
                required property url coverSource

                width: bookGrid.cellWidth
                height: bookGrid.cellHeight

                Item {
                    anchors.centerIn: parent
                    width: 110
                    height: 132

                    Image {
                        id: coverImage
                        anchors.fill: parent
                        source: coverSource
                        fillMode: Image.PreserveAspectFit
                        visible: status === Image.Ready
                    }

                    Label {
                        anchors.centerIn: parent
                        text: "No cover"
                        visible: coverImage.status !== Image.Ready
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

            Label {
                anchors.centerIn: parent
                text: "No books found"
                visible: bookGrid.count === 0
            }
        }
    }
}
