#pragma once

#include <functional>

#include <QWidget>

#include "Character.h"
#include "Key.h"

namespace Evernus
{
    template<class T>
    class Repository;
    class ButtonWithTimer;

    class CharacterBoundWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        typedef std::function<QDateTime (Key::IdType, Character::IdType)> TimeGetter;

        CharacterBoundWidget(const Repository<Character> &characterRepository,
                             const TimeGetter &timeGetter,
                             QWidget *parent = nullptr);
        virtual ~CharacterBoundWidget() = default;

    signals:
        void importFromAPI(Character::IdType id);

    public slots:
        void refreshImportTimer();

        void setCharacter(Character::IdType id);

    private slots:
        void requestUpdate();

    protected:
        ButtonWithTimer &getAPIImportButton() const noexcept;
        const Repository<Character> &getCharacterRepository() const noexcept;
        Character::IdType getCharacterId() const noexcept;

    private:
        const Repository<Character> &mCharacterRepository;
        TimeGetter mTimeGetter;

        ButtonWithTimer *mImportBtn = nullptr;

        Character::IdType mCharacterId = Character::invalidId;

        virtual void handleNewCharacter(Character::IdType id) = 0;
    };
}
