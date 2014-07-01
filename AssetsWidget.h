#pragma once

#include "CharacterBoundWidget.h"

namespace Evernus
{
    class APIManager;

    class AssetsWidget
        : public CharacterBoundWidget
    {
    public:
        explicit AssetsWidget(const APIManager &apiManager, QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    private:
        virtual void handleNewCharacter(Character::IdType id) override;
    };
}
