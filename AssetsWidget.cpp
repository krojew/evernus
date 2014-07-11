/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QLineEdit>
#include <QLocale>
#include <QLabel>
#include <QDebug>

#include "LeafFilterProxyModel.h"
#include "CacheTimerProvider.h"
#include "ButtonWithTimer.h"
#include "AssetProvider.h"

#include "AssetsWidget.h"

namespace Evernus
{
    AssetsWidget::AssetsWidget(const AssetProvider &assetProvider,
                               const EveDataProvider &nameProvider,
                               const CacheTimerProvider &cacheTimerProvider,
                               QWidget *parent)
        : CharacterBoundWidget{std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, CacheTimerProvider::TimerType::AssetList),
                               parent}
        , mAssetProvider{assetProvider}
        , mModel{mAssetProvider, nameProvider}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        auto importFromWeb = new QPushButton{QIcon{":/images/world.png"}, tr("Import prices from Web"), this};
        toolBarLayout->addWidget(importFromWeb);
        importFromWeb->setFlat(true);
        connect(importFromWeb, &QPushButton::clicked, this, &AssetsWidget::prepareItemImportFromWeb);

        auto importFromFile = new QPushButton{QIcon{":/images/page_refresh.png"}, tr("Import prices from logs"), this};
        toolBarLayout->addWidget(importFromFile);
        importFromFile->setFlat(true);
        connect(importFromFile, &QPushButton::clicked, this, &AssetsWidget::prepareItemImportFromFile);

        mFilterEdit = new QLineEdit{this};
        toolBarLayout->addWidget(mFilterEdit);
        mFilterEdit->setPlaceholderText(tr("type in keywords and press Enter"));
        mFilterEdit->setClearButtonEnabled(true);
        connect(mFilterEdit, &QLineEdit::returnPressed, this, &AssetsWidget::applyKeywords);

        mModelProxy = new LeafFilterProxyModel{this};
        mModelProxy->setSourceModel(&mModel);
        mModelProxy->setSortRole(Qt::UserRole);
        mModelProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

        mAssetView = new QTreeView{this};
        mainLayout->addWidget(mAssetView);
        mAssetView->setModel(mModelProxy);
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

    void AssetsWidget::prepareItemImportFromWeb()
    {
        emit importPricesFromWeb(getImportTarget());
    }

    void AssetsWidget::prepareItemImportFromFile()
    {
        emit importPricesFromFile(getImportTarget());
    }

    void AssetsWidget::applyKeywords()
    {
        mModelProxy->setFilterFixedString(mFilterEdit->text());
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

    ItemPriceImporter::TypeLocationPairs AssetsWidget::getImportTarget() const
    {
        ItemPriceImporter::TypeLocationPairs target;

        const auto &assets = mAssetProvider.fetchForCharacter(getCharacterId());
        for (const auto &item : assets)
        {
            const auto locationId = item->getLocationId();
            if (!locationId)
                continue;

            buildImportTarget(target, *item, *locationId);
        }

        return target;
    }

    void AssetsWidget::buildImportTarget(ItemPriceImporter::TypeLocationPairs &target, const Item &item, quint64 locationId)
    {
        target.emplace(std::make_pair(item.getTypeId(), locationId));
        for (const auto &child : item)
            buildImportTarget(target, *child, locationId);
    }
}
