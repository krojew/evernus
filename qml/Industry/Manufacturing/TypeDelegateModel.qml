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
            }
        }

        Type {}

        Component.onCompleted: {
            if (model && model.hasModelChildren) {
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

                var component = Qt.createComponent("TypeDelegateModel.qml");
                if (component.status === Component.Ready)
                    finishCreation();
                else
                    component.statusChanged.connect(finishCreation);
            }
        }
    }
}
