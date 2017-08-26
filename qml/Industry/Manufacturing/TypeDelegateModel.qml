import QtQml.Models 2.2
import QtQuick 2.7

import "qrc:/qml/Industry/Manufacturing"

DelegateModel {
    delegate: Row {
        Column {
            Repeater {
                id: materials
            }
        }
        Type {}

        Component.onCompleted: {
            if (model.hasModelChildren) {
                function finishCreation() {
                    if (component.status === Component.Ready) {
//                        var object = component.createObject(materials, {
//                            "model": model,
//                            "rootIndex": modelIndex(index)
//                        });
//                        if (object === null)
//                            console.error("Error creating source object!");
//                        else
//                            materials.model = object;
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
