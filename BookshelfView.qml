import QtQuick
import QtQuick.Controls

Pane {
    id: root

    property var collection: null
    property var selection: null

    padding: 0

    Item {
        anchors.fill: parent

        GridView {
            id: bookGrid

            y: 28
            anchors.horizontalCenter: parent.horizontalCenter
            width: count === 0 ? parent.width
                : Math.min(parent.width, occupiedColumnCount * cellWidth)
            height: parent.height - 28
            cellWidth: 136
            cellHeight: 160
            clip: true
            model: root.collection
            flow: height < cellHeight * 2 ? GridView.TopToBottom : GridView.LeftToRight
            readonly property int occupiedColumnCount: {
                if (flow === GridView.LeftToRight) {
                    return Math.min(count, Math.max(1, Math.floor(parent.width / cellWidth)))
                }
                return Math.ceil(count / Math.max(1, Math.floor(height / cellHeight)))
            }

            delegate: Item {
                required property string isbn
                required property string title
                required property url coverSource

                width: bookGrid.cellWidth
                height: bookGrid.cellHeight

                Item {
                    anchors.centerIn: parent
                    width: 110
                    height: 132

                    Rectangle {
                        anchors.fill: parent
                        color: root.palette.alternateBase
                        border.color: root.palette.mid
                        visible: coverImage.status !== Image.Ready
                    }

                    Image {
                        id: coverImage
                        anchors.fill: parent
                        source: coverSource
                        fillMode: Image.PreserveAspectFit
                        visible: status === Image.Ready
                    }

                    Label {
                        anchors.fill: parent
                        anchors.margins: 7
                        text: title
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
