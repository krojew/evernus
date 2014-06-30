#pragma once

#include <QWidget>

#include "Character.h"

namespace Evernus
{
    template<class T>
    class Repository;
    class ButtonWithTimer;
    class APIManager;

    class AssetsWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        AssetsWidget(const Repository<Character> &characterRepository, const APIManager &apiManager, QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    signals:
        void importAssets(Character::IdType id);

    public slots:
        void setCharacter(Character::IdType id);

        void refreshImportTimer();

    private slots:
        void requestUpdate();

    private:
        const Repository<Character> &mCharacterRepository;
        const APIManager &mAPIManager;

        ButtonWithTimer *mImportBtn = nullptr;

        Character::IdType mCharacterId = Character::invalidId;
    };
}
