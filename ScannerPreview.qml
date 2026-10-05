import QtQuick
import QtQuick.Controls
import ISBNBookScanner

Pane {
    id: root

    property var scannerController: null
    padding: 0

    ScannerFrameItem {
        id: frameView
        anchors.fill: parent
    }

    Connections {
        target: root.scannerController

        function onFrameReady(image) {
            frameView.setFrame(image)
        }
    }

    Item {
        id: scanOverlay
        anchors.fill: parent

        readonly property real targetWidth: width * 0.7
        readonly property real targetHeight: height * 0.25
        readonly property real targetX: (width - targetWidth) / 2
        readonly property real targetY: (height - targetHeight) / 2
        readonly property int dashCount: Math.max(1, Math.floor((targetWidth - 10) / 10))
        readonly property real dashWidth: 5
        readonly property real dashSpacing: 5
        readonly property real dashStartX: targetX
            + (targetWidth - (dashCount * dashWidth + (dashCount - 1) * dashSpacing)) / 2

        Rectangle {
            x: 0
            y: 0
            width: parent.width
            height: scanOverlay.targetY
            color: root.palette.shadow
            opacity: 0.4
        }
        Rectangle {
            x: 0
            y: scanOverlay.targetY + scanOverlay.targetHeight
            width: parent.width
            height: parent.height - y
            color: root.palette.shadow
            opacity: 0.4
        }
        Rectangle {
            x: 0
            y: scanOverlay.targetY
            width: scanOverlay.targetX
            height: scanOverlay.targetHeight
            color: root.palette.shadow
            opacity: 0.4
        }
        Rectangle {
            x: scanOverlay.targetX + scanOverlay.targetWidth
            y: scanOverlay.targetY
            width: parent.width - x
            height: scanOverlay.targetHeight
            color: root.palette.shadow
            opacity: 0.4
        }

        Item {
            id: targetFrame
            x: scanOverlay.targetX
            y: scanOverlay.targetY
            width: scanOverlay.targetWidth
            height: scanOverlay.targetHeight

            readonly property real cornerLength: Math.min(width, height) / 3
            readonly property real strokeWidth: 3

            Rectangle {
                width: targetFrame.cornerLength
                height: targetFrame.strokeWidth
                color: root.palette.highlight
            }
            Rectangle {
                width: targetFrame.strokeWidth
                height: targetFrame.cornerLength
                color: root.palette.highlight
            }
            Rectangle {
                x: targetFrame.width - width
                width: targetFrame.cornerLength
                height: targetFrame.strokeWidth
                color: root.palette.highlight
            }
            Rectangle {
                x: targetFrame.width - width
                width: targetFrame.strokeWidth
                height: targetFrame.cornerLength
                color: root.palette.highlight
            }
            Rectangle {
                y: targetFrame.height - height
                width: targetFrame.cornerLength
                height: targetFrame.strokeWidth
                color: root.palette.highlight
            }
            Rectangle {
                y: targetFrame.height - height
                width: targetFrame.strokeWidth
                height: targetFrame.cornerLength
                color: root.palette.highlight
            }
            Rectangle {
                x: targetFrame.width - width
                y: targetFrame.height - height
                width: targetFrame.cornerLength
                height: targetFrame.strokeWidth
                color: root.palette.highlight
            }
            Rectangle {
                x: targetFrame.width - width
                y: targetFrame.height - height
                width: targetFrame.strokeWidth
                height: targetFrame.cornerLength
                color: root.palette.highlight
            }
        }

        Repeater {
            model: scanOverlay.dashCount

            Rectangle {
                x: scanOverlay.dashStartX
                    + index * (scanOverlay.dashWidth + scanOverlay.dashSpacing)
                y: scanOverlay.height / 2 - height / 2
                width: scanOverlay.dashWidth
                height: 2
                color: root.palette.highlight
            }
        }
    }

    Label {
        anchors.centerIn: parent
        text: "Waiting for camera frame..."
        visible: !frameView.hasFrame
    }
}