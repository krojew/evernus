import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQuick 2.7

Item {
    readonly property int targetGlowRadius: 10

    width: 400
    height: 70

    states: [
        State {
            PropertyChanges {
                target: glow
                glowRadius: 0
            }
        },
        State {
            name: "selected"
            PropertyChanges {
                target: glow
                glowRadius: targetGlowRadius
            }
        }
    ]

    transitions: [
        Transition {
            from: "*"
            to: "*"
            NumberAnimation {
                properties: "glowRadius"
                target: glow
            }
        }

    ]

    RectangularGlow {
        id: glow
        anchors.fill: parent
        glowRadius: 0
    }

    Rectangle {
        id: header
        color: "lightgray"
        border.width: 1
        border.color: "black"
        anchors.fill: parent

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

    MouseArea {
        anchors.fill: parent
        onClicked: parent.state = "selected"
    }
}
