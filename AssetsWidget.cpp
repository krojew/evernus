#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QLocale>
#include <QLabel>
#include <QDebug>

#include "ButtonWithTimer.h"
#include "APIManager.h"

#include "AssetsWidget.h"

namespace Evernus
{
    AssetsWidget::AssetsWidget(const AssetListRepository &assetRepository,
                               const EveDataProvider &nameProvider,
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

        toolBarLayout->addStretch();

        auto modelProxy = new QSortFilterProxyModel{this};
        modelProxy->setSourceModel(&mModel);

        mAssetView = new QTreeView{this};
        mainLayout->addWidget(mAssetView);
        mAssetView->setModel(modelProxy);
        mAssetView->setSortingEnabled(true);
        mAssetView->header()->setSectionResizeMode(QHeaderView::Stretch);

        mInfoLabel = new QLabel{this};
        mainLayout->addWidget(mInfoLabel);
    }

    void AssetsWidget::updateData()
    {
        refreshImportTimer();
        mModel.reset();
        mAssetView->expandAll();

        setNewInfo();
    }

    void AssetsWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching assets to" << id;

        mModel.setCharacter(id);
        mAssetView->expandAll();

        setNewInfo();
    }

    void AssetsWidget::setNewInfo()
    {
        QLocale locale;
        mInfoLabel->setText(QString{"Total assets: <strong>%1</strong> Total volume: <strong>%2m³</strong> Total sell price: <strong>%3</strong>"}
            .arg(locale.toString(mModel.getTotalAssets()))
            .arg(locale.toString(mModel.getTotalVolume(), 'f', 2))
            .arg(locale.toCurrencyString(mModel.getTotalSellPrice(), "ISK")));
    }
}
