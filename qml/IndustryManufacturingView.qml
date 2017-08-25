import QtQuick 2.8

import "qrc:/qml"

Column {
    Repeater {
        model: setupModel
        delegate: IndustryType {}
    }
}
