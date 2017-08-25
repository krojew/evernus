import QtQuick 2.8

ListView {
    model: setupModel
    delegate: Text {
        text: name
    }
}
