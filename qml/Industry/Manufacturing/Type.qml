import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import QtQml.Models 2.2
import QtQuick 2.7

Item {
    readonly property int targetGlowRadius: 10

    property bool isOutput: false

    id: root
    width: 400
    height: header.height + 64

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
        color: "#ffaa00"
    }

    Rectangle {
        id: header
        border.width: 1
        border.color: "black"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: typeName.height + typeName.anchors.margins * 2
        gradient: Gradient {
            GradientStop { position: 0; color: "#658ead" }
            GradientStop { position: 1; color: "#353535" }
        }

        Text {
            id: typeName
            font.bold: true
            text: name
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.margins: 5
            color: "white"
        }
    }

    Rectangle {
        id: content
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 64
        color: "#171916"

        Image {
            id: icon
            source: "https://image.eveonline.com/Type/" + typeId + "_64.png"
            anchors.top: parent.top
            anchors.left: parent.left
        }

        ColumnLayout {
            anchors.left: icon.right
            anchors.top: parent.top

            Text {
                id: quantityText
                text: qsTr("Quantity produced: %L1").arg(quantityProduced)
                color: "#cccccc"
                visible: quantityProduced > 0
                Layout.fillWidth: true
                Layout.fillHeight: true
            }

            Item {
                visible: !isOutput
                Layout.fillWidth: true

                Text {
                    id: sourceText
                    text: qsTr("Source:")
                    color: "#cccccc"
                    anchors.verticalCenter: sourceCombo.verticalCenter
                    anchors.left: parent.left
                }

                ComboBox {
                    id: sourceCombo
                    anchors.left: sourceText.right
                    model: ListModel {
                        ListElement { text: qsTr("Buy from source") }
                        ListElement { text: qsTr("Take assets then buy from source") }
                    }

                    Component.onCompleted: {
                        if (quantityProduced > 0) {
                            model.append({ text: qsTr("Manufacture") });
                            model.append({ text: qsTr("Take assets then manufacture") });
                        }
                    }
                }
            }
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
        propagateComposedEvents: true
        onClicked: {
            root.selected(typeId);
            mouse.accepted = false;
        }
        onPressed: mouse.accepted = false
        onReleased: mouse.accepted = false
        onDoubleClicked: mouse.accepted = false
        onPositionChanged: mouse.accepted = false
        onPressAndHold: mouse.accepted = false
    }

    Component.onCompleted: {
        root.selected.connect(SetupController.typeSelected);
    }
}
