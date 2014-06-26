#pragma once

#include <QWidget>

namespace Evernus
{
    class CharacterWidget
        : public QWidget
    {
    public:
        explicit CharacterWidget(QWidget *parent = nullptr);
        virtual ~CharacterWidget() = default;
    };
}
