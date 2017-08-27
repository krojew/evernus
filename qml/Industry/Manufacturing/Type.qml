import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQuick 2.7

Item {
    readonly property int glowRadius: 10

    property bool selected: false

    width: 300
    height: 50

    RectangularGlow {
        anchors.fill: parent
        glowRadius: 0
        visible: selected
    }

    Rectangle {
        id: header
        color: "lightgray"
        border.width: 1
        border.color: "black"
        x: glowRadius
        y: glowRadius
        width: parent.width - glowRadius * 2
        height: parent.height - glowRadius * 2

        Image {
            id: icon
            anchors.left: parent.left
            anchors.top: parent.top
            height: parent.height
            source: "https://image.eveonline.com/Type/" + typeId + "_64.png"
            fillMode: Image.PreserveAspectFit
        }

        Label {
            font.bold: true
            text: name
            anchors.left: icon.right
            anchors.top: parent.top
            anchors.margins: 5
        }
    }
}
