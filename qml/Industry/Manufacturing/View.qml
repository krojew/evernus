import QtQuick.Layouts 1.3
import QtQuick 2.7

import "qrc:/qml/Industry/Manufacturing"

ColumnLayout {
    Repeater {
        model: TypeDelegateModel {
            model: setupModel
        }
    }
}
