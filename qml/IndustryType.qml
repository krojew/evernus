import QtGraphicalEffects 1.0
import QtQuick 2.8

Item {
    width: 300
    height: 50
    
    RectangularGlow {
        anchors.fill: rect
        glowRadius: 10
        cornerRadius: rect.radius + glowRadius
    }
    
    Rectangle {
        id: rect
        color: "black"
        anchors.centerIn: parent
        width: Math.round(parent.width / 1.5)
        height: Math.round(parent.height / 2)
        radius: 25
    }
    
    Text {
        id: name
        color: "white"
        anchors.left: parent.left
        anchors.top: parent.top
    }
}
