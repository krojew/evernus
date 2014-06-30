#pragma once

#include "CharacterBoundWidget.h"

namespace Evernus
{
    template<class T>
    class Repository;
    class APIManager;

    class AssetsWidget
        : public CharacterBoundWidget
    {
    public:
        AssetsWidget(const Repository<Character> &characterRepository, const APIManager &apiManager, QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    private:
        const Repository<Character> &mCharacterRepository;

        virtual void handleNewCharacter(Character::IdType id) override;
    };
}
