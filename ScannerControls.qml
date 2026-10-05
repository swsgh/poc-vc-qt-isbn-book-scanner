import QtQuick
import QtQuick.Controls
import ISBNBookScanner

Column {
    id: root

    property var scannerController: null
    property var appController: null

    spacing: 6

    ScannerPreview {
        width: Math.min(root.width, 932)
        height: 220
        anchors.horizontalCenter: parent.horizontalCenter
        scannerController: root.scannerController
    }

    Row {
        width: root.width
        spacing: 8

        TextField {
            id: isbnInput

            width: parent.width - lookupButton.width - parent.spacing
            placeholderText: "Type an ISBN code manually (e.g. 9781449392178)..."
            maximumLength: 17
            onAccepted: root.lookupIsbn()
        }

        Button {
            id: lookupButton
            text: "Lookup"
            onClicked: root.lookupIsbn()
        }
    }

    Label {
        width: root.width
        text: root.appController ? root.appController.scannerStatusText : ""
        font.bold: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }

    function lookupIsbn() {
        if (root.appController && root.appController.submitManualIsbn(isbnInput.text)) {
            isbnInput.clear()
        }
    }
}