import QtQuick.Layouts 1.3
import QtQml.Models 2.2
import QtQuick 2.7

import "qrc:/qml/Industry/Manufacturing"

DelegateModel {
    id: mainModel

    delegate: RowLayout {
        ColumnLayout {
            Repeater {
                id: materials

                onItemAdded: {
                    var connection = Qt.createQmlObject("
import com.evernus.qmlcomponents 2.6

BezierCurve {
    anchors.fill: parent
}", connections);

                    function setCurveSourcePoints() {
                        connection.p1 = Qt.point(0, (item.y + item.height / 2) / connections.height);
                        connection.p2 = Qt.point(0.5, (item.y + item.height / 2) / connections.height);
                    }

                    connection.p3 = Qt.point(0.5, 0.5);
                    connection.p4 = Qt.point(1, 0.5);

                    setCurveSourcePoints();
                    connections.heightChanged.connect(setCurveSourcePoints);
                }
            }
        }

        Item {
            id: connections
            width: 50
            anchors.top: parent.top
            anchors.bottom: parent.bottom
        }

        Type {
            id: type
        }

        Component.onCompleted: {
            if (model && model.hasModelChildren) {
                var component = Qt.createComponent("TypeDelegateModel.qml");

                function finishCreation() {
                    function finishObject(object) {
                        materials.model = object;
                    }

                    if (component.status === Component.Ready) {
                        var incubator = component.incubateObject(materials, {
                            "model": mainModel.model,
                            "rootIndex": mainModel.modelIndex(index)
                        }, Qt.Asynchronous);

                        if (incubator.status !== Component.Ready) {
                            incubator.onStatusChanged = function(status) {
                                if (status === Component.Ready)
                                    finishObject(incubator.object);
                                else
                                    console.error("Error creating source object:", status);
                            };
                        } else {
                            finishObject(incubator.object);
                        }
                    } else if (component.status === Component.Error) {
                        console.error("Error loading component:", component.errorString());
                    }
                }

                if (component.status === Component.Ready)
                    finishCreation();
                else if (component.status === Component.Error)
                    console.error("Error loading component:", component.errorString());
                else
                    component.statusChanged.connect(finishCreation);
            }
        }
    }
}
