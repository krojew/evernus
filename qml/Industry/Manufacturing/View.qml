import QtQuick 2.8

import "qrc:/qml/Industry/Manufacturing"

Column {
    Repeater {
        model: TypeDelegateModel {
            model: setupModel
        }
    }
}
