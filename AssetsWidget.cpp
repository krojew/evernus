#include <QSortFilterProxyModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QDebug>

#include "ButtonWithTimer.h"
#include "APIManager.h"

#include "AssetsWidget.h"

namespace Evernus
{
    AssetsWidget::AssetsWidget(const AssetListRepository &assetRepository,
                               const NameProvider &nameProvider,
                               const APIManager &apiManager,
                               QWidget *parent)
        : CharacterBoundWidget{std::bind(&APIManager::getAssetsLocalCacheTime, &apiManager, std::placeholders::_1),
                               parent}
        , mModel{assetRepository, nameProvider}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        auto modelProxy = new QSortFilterProxyModel{this};
        modelProxy->setSourceModel(&mModel);

        auto assetView = new QTreeView{this};
        mainLayout->addWidget(assetView);
        assetView->setModel(modelProxy);

        toolBarLayout->addStretch();
    }

    void AssetsWidget::updateData()
    {
        refreshImportTimer();
        mModel.reset();
    }

    void AssetsWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching assets to" << id;

        mModel.setCharacter(id);
    }
}
