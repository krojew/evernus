#include "ButtonWithTimer.h"
#include "Repository.h"

#include "CharacterBoundWidget.h"

namespace Evernus
{
    CharacterBoundWidget::CharacterBoundWidget(const Repository<Character> &characterRepository,
                                               const TimeGetter &timeGetter,
                                               QWidget *parent)
        : QWidget{parent}
        , mCharacterRepository{characterRepository}
        , mTimeGetter{timeGetter}
        , mImportBtn{new ButtonWithTimer{tr("API import"), this}}
    {
        connect(mImportBtn, &QPushButton::clicked, this, &CharacterBoundWidget::requestUpdate);
    }

    void CharacterBoundWidget::refreshImportTimer()
    {
        struct CannotSetTimerException { };

        try
        {
            if (mCharacterId == Character::invalidId)
                throw CannotSetTimerException{};

            try
            {
                const auto character = mCharacterRepository.find(mCharacterId);
                const auto key = character.getKeyId();

                if (!key)
                    throw CannotSetTimerException{};

                const auto time = mTimeGetter(*key, mCharacterId);
                mImportBtn->setTimer(time);
            }
            catch (const Repository<Character>::NotFoundException &)
            {
                throw CannotSetTimerException{};
            }
        }
        catch (const CannotSetTimerException &)
        {
        }
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

    const Repository<Character> &CharacterBoundWidget::getCharacterRepository() const noexcept
    {
        return mCharacterRepository;
    }

    Character::IdType CharacterBoundWidget::getCharacterId() const noexcept
    {
        return mCharacterId;
    }
}
