import QtQuick 2.8

ListView {
    width: 1000
    height: 500
    model: setupModel
    delegate: Text {
        text: name
    }
}
