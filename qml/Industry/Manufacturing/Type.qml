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
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQml.Models 2.2
import QtQuick 2.7
import QtQml 2.2

Item {
    property bool isOutput: false
    property bool isManufactured: isOutput || source === 1 || source === 5

    readonly property int contentPadding: 10

    id: root
    width: 500
    height: childrenRect.height

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

    function formatDuration(duration) {
        var hours   = Math.floor(duration / 3600);
        var minutes = Math.floor((duration / 60) % 60);
        var seconds = duration % 60;

        if (hours < 10)
            hours   = "0" + hours;
        if (minutes < 10)
            minutes = "0" + minutes;
        if (seconds < 10)
            seconds = "0" + seconds;

        return "%1:%2:%3".arg(hours).arg(minutes).arg(seconds);
    }

    function formatCurrency(value) {
        return value.toLocaleCurrencyString(Qt.locale(), (omitCurrencySymbol) ? (" ") : (qsTr("ISK")));
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

        Label {
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
        height: Math.max(icon.height, contentLayout.height) + contentPadding * 2
        color: "#171916"

        Image {
            id: icon
            source: "https://image.eveonline.com/Type/" + typeId + "_64.png"
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
        }

        MouseArea {
            anchors.fill: parent
            onClicked: selectCurrentType()
        }

        RowLayout {
            id: contentLayout
            anchors.topMargin: contentPadding
            anchors.left: icon.right
            anchors.top: parent.top
            anchors.right: parent.right

            GridLayout {
                Layout.alignment: Qt.AlignTop | Qt.AlignLeft
                columns: 2

                Label {
                    visible: !isOutput
                    id: sourceText
                    text: qsTr("Source:")
                    color: "#cccccc"
                }

                ComboBox {
                    visible: !isOutput
                    id: sourceCombo
                    textRole: "text"
                    Layout.fillWidth: true
                    model: ListModel {
                        ListElement { text: qsTr("Buy from source"); value: 0; }
                        ListElement { text: qsTr("Acquire for free"); value: 2; }
                        ListElement { text: qsTr("Buy at custom cost"); value: 3; }
                        ListElement { text: qsTr("Take assets then buy from source"); value: 4; }
                        ListElement { text: qsTr("Take assets then buy at custom cost"); value: 6; }
                    }

                    onCurrentIndexChanged: {
                        if (!isOutput)
                            setupController.setSource(typeId, model.get(currentIndex).value);
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
                        target: setupController
                        enabled: !isOutput

                        onSourceChanged: {
                            if (id == typeId)
                                setSourceTypeCombo();
                        }
                    }
                }

                Label {
                    text: qsTr("Runs:")
                    color: "#cccccc"
                    visible: isOutput
                }

                SpinBox {
                    value: runs
                    to: 1000000
                    editable: true
                    visible: isOutput

                    onValueChanged: setupController.setRuns(typeId, value)
                }

                Label {
                    text: qsTr("ME:")
                    color: "#cccccc"
                }

                RowLayout {
                    SpinBox {
                        id: me
                        value: materialEfficiency
                        to: 10
                        editable: true
                        enabled: isManufactured

                        onValueChanged: setupController.setMaterialEfficiency(typeId, value)

                        Connections {
                            target: setupController

                            onMaterialEfficiencyChanged: {
                                if (id == typeId)
                                    me.value = value;
                            }
                        }
                    }

                    Label {
                        text: qsTr("TE:")
                        color: "#cccccc"
                    }

                    SpinBox {
                        id: te
                        value: timeEfficiency
                        to: 20
                        editable: true
                        enabled: isManufactured

                        onValueChanged: setupController.setTimeEfficiency(typeId, value)

                        Connections {
                            target: setupController

                            onTimeEfficiencyChanged: {
                                if (id == typeId)
                                    te.value = value;
                            }
                        }
                    }
                }

                Label {
                    id: quantityText
                    text: (quantityProduced > 0) ? (qsTr("Quantity produced: %L1 / %2").arg(quantityProduced).arg(formatDuration(time))) : (qsTr("Quantity produced: N/A"))
                    color: "#cccccc"
                    Layout.fillWidth: true
                    Layout.columnSpan: 2
                }

                Label {
                    font.bold: true
                    text: (isManufactured) ? (qsTr("Total production time: %1").arg(formatDuration(totalTime))) : (qsTr("Total production time: N/A"))
                    color: "orange"
                    Layout.columnSpan: 2
                }

                Item {
                    width: childrenRect.width
                    height: childrenRect.height
                    Layout.columnSpan: 2

                    Label {
                        id: totalCosts
                        font.bold: true
                        text: qsTr("Total cost: %1").arg(formatCurrency(cost.totalCost))
                        color: "orange"
                    }

                    Image {
                        anchors.left: totalCosts.right
                        anchors.verticalCenter: totalCosts.verticalCenter
                        visible: isManufactured
                        source: "qrc:///images/information.png"
                        asynchronous: true

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.WhatsThisCursor

                            ToolTip.delay: 500
                            ToolTip.visible: containsMouse
                            ToolTip.text: qsTr("Component cost: %1\nJob fee: %2\nJob tax: %3")
                                              .arg(formatCurrency(cost.children))
                                              .arg(formatCurrency(cost.jobFee))
                                              .arg(formatCurrency(cost.jobTax))
                        }
                    }
                }

                Label {
                    font.bold: true
                    text: qsTr("Profit from selling: %1").arg(formatCurrency(profit - cost.totalCost))
                    color: ((profit - cost.totalCost) <= 0) ? ("red") : ("green")
                    Layout.columnSpan: 2
                }

                Label {
                    font.bold: true
                    text: qsTr("ISK/h: %1").arg((totalTime > 0) ? (formatCurrency((profit - cost.totalCost) * 3600 / totalTime)) : ("N/A"))
                    color: ((profit - cost.totalCost) <= 0) ? ("red") : ("green")
                    Layout.columnSpan: 2
                }
            }

            Label {
                visible: !isOutput
                text: Number(quantityRequired).toLocaleString(Qt.locale(), "f", 0)
                color: "#cccccc"
                font.bold: true
                Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
            }
        }
    }

    Connections {
        target: setupController
        onTypeSelected: {
            if (id == typeId)
                root.state = "selected";
            else
                root.state = "";
        }
    }

    Component.onCompleted: {
        root.selected.connect(setupController.typeSelected);
    }
}
