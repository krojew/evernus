#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

#include "ButtonWithTimer.h"
#include "APIManager.h"

#include "AssetsWidget.h"

namespace Evernus
{
    AssetsWidget::AssetsWidget(const APIManager &apiManager, QWidget *parent)
        : CharacterBoundWidget{std::bind(&APIManager::getAssetsLocalCacheTime, &apiManager, std::placeholders::_1),
                               parent}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        toolBarLayout->addStretch();

        mainLayout->addStretch();
    }

    void AssetsWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching assets to" << id;
    }
}
