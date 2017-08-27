import QtQuick.Layouts 1.3
import QtQuick 2.7

import "qrc:/qml/Industry/Manufacturing"

Flickable {
    contentWidth: contentItem.childrenRect.width
    contentHeight: contentItem.childrenRect.height

    ColumnLayout {
        Repeater {
            model: TypeDelegateModel {
                model: setupModel
            }
        }
    }
}
