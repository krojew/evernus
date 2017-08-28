import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQuick 2.7

Item {
    readonly property int targetGlowRadius: 10

    id: root
    width: 400
    height: 70

    signal selected(int typeId)

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

    Connections {
        target: SetupController
        onTypeSelected: {
            if (id == typeId)
                root.state = "selected";
            else
                root.state = "";
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.selected(typeId)
        }
    }

    Component.onCompleted: {
        root.selected.connect(SetupController.typeSelected);
    }
}
