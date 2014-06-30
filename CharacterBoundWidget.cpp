#include "ButtonWithTimer.h"

#include "CharacterBoundWidget.h"

namespace Evernus
{
    CharacterBoundWidget::CharacterBoundWidget(const TimeGetter &timeGetter,
                                               QWidget *parent)
        : QWidget{parent}
        , mTimeGetter{timeGetter}
        , mImportBtn{new ButtonWithTimer{tr("API import"), this}}
    {
        connect(mImportBtn, &QPushButton::clicked, this, &CharacterBoundWidget::requestUpdate);
    }

    void CharacterBoundWidget::refreshImportTimer()
    {
        if (mCharacterId != Character::invalidId)
            mImportBtn->setTimer(mTimeGetter(mCharacterId));
    }

    void CharacterBoundWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
        if (mCharacterId == Character::invalidId)
        {
            mImportBtn->setDisabled(true);
            mImportBtn->stopTimer();
        }
        else
        {
            mImportBtn->setEnabled(true);
            refreshImportTimer();
        }

        handleNewCharacter(mCharacterId);
    }

    void CharacterBoundWidget::requestUpdate()
    {
        Q_ASSERT(mCharacterId != Character::invalidId);
        emit importFromAPI(mCharacterId);
    }

    ButtonWithTimer &CharacterBoundWidget::getAPIImportButton() const noexcept
    {
        return *mImportBtn;
    }

    Character::IdType CharacterBoundWidget::getCharacterId() const noexcept
    {
        return mCharacterId;
    }
}
