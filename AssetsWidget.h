#pragma once

#include "CharacterBoundWidget.h"

namespace Evernus
{
    class APIManager;

    class AssetsWidget
        : public CharacterBoundWidget
    {
    public:
        AssetsWidget(const Repository<Character> &characterRepository, const APIManager &apiManager, QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    private:
        virtual void handleNewCharacter(Character::IdType id) override;
    };
}
