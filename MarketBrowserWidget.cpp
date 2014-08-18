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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QTabWidget>
#include <QSqlQuery>
#include <QListView>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>

#include "ExternalOrderRepository.h"
#include "MarketOrderRepository.h"
#include "ExternalOrderView.h"
#include "MarketOrderView.h"
#include "EveDataProvider.h"
#include "DatabaseUtils.h"

#include "MarketBrowserWidget.h"

namespace Evernus
{
    MarketBrowserWidget::MarketBrowserWidget(const ExternalOrderRepository &externalOrderRepo,
                                             const MarketOrderRepository &orderRepo,
                                             const MarketOrderRepository &corpOrderRepo,
                                             const CharacterRepository &characterRepo,
                                             const MarketOrderProvider &orderProvider,
                                             const MarketOrderProvider &corpOrderProvider,
                                             const EveDataProvider &dataProvider,
                                             QWidget *parent)
        : QWidget(parent)
        , mExternalOrderRepo(externalOrderRepo)
        , mOrderRepo(orderRepo)
        , mCorpOrderRepo(corpOrderRepo)
        , mDataProvider(dataProvider)
        , mNameModel(mDataProvider)
        , mOrderNameModel(mDataProvider)
        , mExternalOrderSellModel(mDataProvider, mExternalOrderRepo, characterRepo, orderProvider, corpOrderProvider)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolbarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolbarLayout);

        auto importBtn = new QPushButton{QIcon{":/images/world.png"}, tr("Import prices from Web"), this};
        toolbarLayout->addWidget(importBtn);
        importBtn->setFlat(true);
        connect(importBtn, &QPushButton::clicked, this, &MarketBrowserWidget::prepareItemImportFromWeb);

        importBtn = new QPushButton{QIcon{":/images/page_refresh.png"}, tr("Import prices from logs"), this};
        toolbarLayout->addWidget(importBtn);
        importBtn->setFlat(true);
        connect(importBtn, &QPushButton::clicked, this, &MarketBrowserWidget::prepareItemImportFromFile);

        importBtn = new QPushButton{QIcon{":/images/disk_multiple.png"}, tr("Import prices from cache"), this};
        toolbarLayout->addWidget(importBtn);
        importBtn->setFlat(true);
        connect(importBtn, &QPushButton::clicked, this, &MarketBrowserWidget::prepareItemImportFromCache);

        toolbarLayout->addStretch();

        auto mainViewLayout = new QHBoxLayout{};
        mainLayout->addLayout(mainViewLayout, 1);

        auto navigatorGroup = new QGroupBox{tr("Navigator"), this};
        mainViewLayout->addWidget(navigatorGroup);

        auto navigatorGroupLayout = new QVBoxLayout{};
        navigatorGroup->setLayout(navigatorGroupLayout);

        auto itemTabs = new QTabWidget{};
        navigatorGroupLayout->addWidget(itemTabs);

        itemTabs->addTab(createItemNameListTab(mNameModel, mKnownItemList), tr("Name"));
        fillKnownItemNames();

        itemTabs->addTab(createItemNameListTab(mOrderNameModel, mOrderItemList), tr("My orders"));
        fillOrderItemNames();

        auto filterLabel = new QLabel{tr("Regions [<a href='#'>all</a>]"), this};
        navigatorGroupLayout->addWidget(filterLabel);
        connect(filterLabel, &QLabel::linkActivated, this, [this] {
            mRegionList->setCurrentRow(0);
        });

        mRegionList = new QListWidget{this};
        navigatorGroupLayout->addWidget(mRegionList);
        mRegionList->setCurrentItem(new QListWidgetItem{tr("(all)"), mRegionList});
        connect(mRegionList, &QListWidget::currentItemChanged, this, &MarketBrowserWidget::selectRegion);

        const auto regions = dataProvider.getRegions();
        for (const auto &region : regions)
        {
            auto item = new QListWidgetItem{region.second, mRegionList};
            item->setData(Qt::UserRole, region.first);
        }

        filterLabel = new QLabel{tr("Constellations [<a href='#'>all</a>]"), this};
        navigatorGroupLayout->addWidget(filterLabel);
        connect(filterLabel, &QLabel::linkActivated, this, [this] {
            mConstellationList->setCurrentRow(0);
        });

        mConstellationList = new QListWidget{this};
        navigatorGroupLayout->addWidget(mConstellationList);
        mConstellationList->setCurrentItem(new QListWidgetItem{tr("(all)"), mConstellationList});
        connect(mConstellationList, &QListWidget::currentItemChanged, this, &MarketBrowserWidget::selectConstellation);

        filterLabel = new QLabel{tr("Solar systems [<a href='#'>all</a>]"), this};
        navigatorGroupLayout->addWidget(filterLabel);
        connect(filterLabel, &QLabel::linkActivated, this, [this] {
            mSolarSystemList->setCurrentRow(0);
        });

        mSolarSystemList = new QListWidget{this};
        navigatorGroupLayout->addWidget(mSolarSystemList);
        mSolarSystemList->setCurrentItem(new QListWidgetItem{tr("(all)"), mSolarSystemList});
        connect(mSolarSystemList, &QListWidget::currentItemChanged, this, &MarketBrowserWidget::selectSolarSystem);

        filterLabel = new QLabel{tr("Stations [<a href='#'>all</a>]"), this};
        navigatorGroupLayout->addWidget(filterLabel);
        connect(filterLabel, &QLabel::linkActivated, this, [this] {
            mStationList->setCurrentRow(0);
        });

        mStationList = new QListWidget{this};
        navigatorGroupLayout->addWidget(mStationList);
        mStationList->setCurrentItem(new QListWidgetItem{tr("(all)"), mStationList});

        auto orderLayout = new QVBoxLayout{};
        mainViewLayout->addLayout(orderLayout, 1);

        auto sellGroup = new QGroupBox{tr("Sell orders"), this};
        orderLayout->addWidget(sellGroup);

        auto sellLayout = new QVBoxLayout{};
        sellGroup->setLayout(sellLayout);

        mSellView = new ExternalOrderView{this};
        sellLayout->addWidget(mSellView);
        mSellView->setModel(&mExternalOrderSellModel);
    }

    void MarketBrowserWidget::setCharacter(Character::IdType id)
    {
        mExternalOrderSellModel.setCharacter(id);
    }

    void MarketBrowserWidget::updateData()
    {
        fillKnownItemNames();
        mExternalOrderSellModel.reset();
    }

    void MarketBrowserWidget::fillOrderItemNames()
    {
        QSqlQuery query{QString{R"(SELECT DISTINCT ids.type_id FROM (
            SELECT type_id FROM %1 WHERE state = ?
            UNION
            SELECT type_id FROM %2 WHERE state = ?
        ) ids)"}.arg(mOrderRepo.getTableName()).arg(mCorpOrderRepo.getTableName()), mOrderRepo.getDatabase()};

        query.addBindValue(static_cast<int>(MarketOrder::State::Active));
        query.addBindValue(static_cast<int>(MarketOrder::State::Active));

        DatabaseUtils::execQuery(query);

        ItemNameModel::TypeList types;

        const auto size = query.size();
        if (size > 0)
            types.reserve(size);

        while (query.next())
            types.emplace_back(query.value(0).value<EveType::IdType>());

        mOrderNameModel.setTypes(std::move(types));
    }

    void MarketBrowserWidget::prepareItemImportFromWeb()
    {
        emit importPricesFromWeb(getImportTarget());
    }

    void MarketBrowserWidget::prepareItemImportFromFile()
    {
        emit importPricesFromFile(getImportTarget());
    }

    void MarketBrowserWidget::prepareItemImportFromCache()
    {
        emit importPricesFromCache(getImportTarget());
    }

    void MarketBrowserWidget::selectRegion(QListWidgetItem *item)
    {
        mConstellationList->blockSignals(true);
        mConstellationList->clear();
        mConstellationList->blockSignals(false);
        mConstellationList->setCurrentItem(new QListWidgetItem{tr("(all)"), mConstellationList});

        if (item != nullptr && item->data(Qt::UserRole).toUInt() != 0)
        {
            const auto constellations = mDataProvider.getConstellations(item->data(Qt::UserRole).toUInt());
            for (const auto &constellation : constellations)
            {
                auto item = new QListWidgetItem{constellation.second, mConstellationList};
                item->setData(Qt::UserRole, constellation.first);
            }
        }
    }

    void MarketBrowserWidget::selectConstellation(QListWidgetItem *item)
    {
        mSolarSystemList->blockSignals(true);
        mSolarSystemList->clear();
        mSolarSystemList->blockSignals(false);
        mSolarSystemList->setCurrentItem(new QListWidgetItem{tr("(all)"), mSolarSystemList});

        if (item != nullptr && item->data(Qt::UserRole).toUInt() != 0)
        {
            const auto solarSystems = mDataProvider.getSolarSystems(item->data(Qt::UserRole).toUInt());
            for (const auto &solarSystem : solarSystems)
            {
                auto item = new QListWidgetItem{solarSystem.second, mSolarSystemList};
                item->setData(Qt::UserRole, solarSystem.first);
            }
        }
    }

    void MarketBrowserWidget::selectSolarSystem(QListWidgetItem *item)
    {
        mStationList->blockSignals(true);
        mStationList->clear();
        mStationList->blockSignals(false);
        mStationList->setCurrentItem(new QListWidgetItem{tr("(all)"), mStationList});

        if (item != nullptr && item->data(Qt::UserRole).toUInt() != 0)
        {
            const auto stations = mDataProvider.getStations(item->data(Qt::UserRole).toUInt());
            for (const auto &station : stations)
            {
                auto item = new QListWidgetItem{station.second, mStationList};
                item->setData(Qt::UserRole, station.first);
            }
        }
    }

    ExternalOrderImporter::TypeLocationPairs MarketBrowserWidget::getImportTarget() const
    {
        ExternalOrderImporter::TypeLocationPairs result;

        const auto orders = mExternalOrderRepo.fetchUniqueTypesAndStations();
        for (const auto &order : orders)
            result.emplace(std::make_pair(order.first, order.second));

        return result;
    }

    void MarketBrowserWidget::fillKnownItemNames()
    {
        mNameModel.setTypes(mExternalOrderRepo.fetchUniqueTypes());
    }

    void MarketBrowserWidget::showOrdersForType(EveType::IdType typeId)
    {
        mExternalOrderSellModel.setType(typeId);
        mSellView->resizeSections(QHeaderView::ResizeToContents);
    }

    QWidget *MarketBrowserWidget::createItemNameListTab(ItemNameModel &model, QListView *&view)
    {
        auto knownItemsTab = new QWidget{this};

        auto knownItemsLayout = new QVBoxLayout{};
        knownItemsTab->setLayout(knownItemsLayout);

        auto knownItemsProxy = new QSortFilterProxyModel{this};
        knownItemsProxy->setSourceModel(&model);
        knownItemsProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        knownItemsProxy->sort(0);

        auto knowItemsFilterEdit = new QLineEdit{this};
        knownItemsLayout->addWidget(knowItemsFilterEdit);
        knowItemsFilterEdit->setClearButtonEnabled(true);
        knowItemsFilterEdit->setPlaceholderText(tr("type in wildcard"));
        connect(knowItemsFilterEdit, &QLineEdit::textChanged, knownItemsProxy, &QSortFilterProxyModel::setFilterWildcard);

        view = new QListView{this};
        knownItemsLayout->addWidget(view);
        view->setModel(knownItemsProxy);
        connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, [&model, this](const auto &index) {
            if (index.isValid())
                showOrdersForType(model.data(index, Qt::UserRole).template value<Evernus::EveType::IdType>());
        });

        return knownItemsTab;
    }
}
