#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

#include "ButtonWithTimer.h"
#include "Repository.h"
#include "APIManager.h"

#include "AssetsWidget.h"

namespace Evernus
{
    AssetsWidget::AssetsWidget(const Repository<Character> &characterRepository, const APIManager &apiManager, QWidget *parent)
        : QWidget{parent}
        , mCharacterRepository{characterRepository}
        , mAPIManager{apiManager}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        mImportBtn = new ButtonWithTimer{tr("API import"), this};
        toolBarLayout->addWidget(mImportBtn);
        connect(mImportBtn, &QPushButton::clicked, this, &AssetsWidget::requestUpdate);

        toolBarLayout->addStretch();

        mainLayout->addStretch();
    }

    void AssetsWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        qDebug() << "Switching assets to" << mCharacterId;

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
    }

    void AssetsWidget::requestUpdate()
    {
        Q_ASSERT(mCharacterId != Character::invalidId);
        emit importAssets(mCharacterId);
    }

    void AssetsWidget::refreshImportTimer()
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

                const auto time = mAPIManager.getAssetsLocalCacheTime(*key, mCharacterId);
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
}
