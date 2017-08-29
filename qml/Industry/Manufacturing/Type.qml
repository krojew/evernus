/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtGraphicalEffects 1.0
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import QtQml.Models 2.2
import QtQuick 2.7

Item {
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
                glowRadius: 10
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

    function selectCurrentType() {
        root.selected(typeId);
    }

    function setSourceTypeCombo() {
        for (var i = 0; i < sourceCombo.model.count; ++i) {
            if (sourceCombo.model.get(i).value === source) {
                sourceCombo.currentIndex = i;
                break;
            }
        }
    }

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

        MouseArea {
            anchors.fill: parent
            onClicked: selectCurrentType()
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

        MouseArea {
            anchors.fill: parent
            onClicked: selectCurrentType()
        }

        RowLayout {
            anchors.left: icon.right
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            ColumnLayout {
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft

                Text {
                    id: quantityText
                    text: qsTr("Quantity produced: %L1").arg(quantityProduced)
                    color: "#cccccc"
                    visible: quantityProduced > 0
                    Layout.fillWidth: true
                }

                Item {
                    visible: !isOutput
                    Layout.fillWidth: true

                    RowLayout {
                        Text {
                            id: sourceText
                            text: qsTr("Source:")
                            color: "#cccccc"
                        }

                        ComboBox {
                            id: sourceCombo
                            model: ListModel {
                                ListElement { text: qsTr("Buy from source"); value: 0; }
                                ListElement { text: qsTr("Acquire for free"); value: 2; }
                                ListElement { text: qsTr("Buy at custom cost"); value: 3; }
                                ListElement { text: qsTr("Take assets then buy from source"); value: 4; }
                                ListElement { text: qsTr("Take assets then buy at custom cost"); value: 6; }
                            }

                            onCurrentIndexChanged: {
                                if (!isOutput)
                                    SetupController.setSource(typeId, model.get(currentIndex).value);
                            }

                            Component.onCompleted: {
                                if (isOutput)
                                    return;

                                if (quantityProduced > 0) {
                                    model.append({ text: qsTr("Manufacture"), value: 1 });
                                    model.append({ text: qsTr("Take assets then manufacture"), value: 5 });
                                }


                                setSourceTypeCombo();
                            }

                            Connections {
                                target: SetupController
                                enabled: !isOutput

                                onSourceChanged: {
                                    if (id == typeId)
                                        setSourceTypeCombo();
                                }
                            }
                        }

                    }
                }
            }

            Text {
                visible: !isOutput
                text: qsTr("%L1").arg(quantityRequired)
                color: "#cccccc"
                font.bold: true
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

    Component.onCompleted: {
        root.selected.connect(SetupController.typeSelected);
    }
}
