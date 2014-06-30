#pragma once

#include <functional>

#include <QWidget>

#include "Character.h"

namespace Evernus
{
    class ButtonWithTimer;

    class CharacterBoundWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        typedef std::function<QDateTime (Character::IdType)> TimeGetter;

        CharacterBoundWidget(const TimeGetter &timeGetter,
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
        Character::IdType getCharacterId() const noexcept;

    private:
        TimeGetter mTimeGetter;

        ButtonWithTimer *mImportBtn = nullptr;

        Character::IdType mCharacterId = Character::invalidId;

        virtual void handleNewCharacter(Character::IdType id) = 0;
    };
}
