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
#include <QWidgetAction>
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
#include <QMenu>

#include "ExternalOrderRepository.h"
#include "MarketOrderRepository.h"
#include "DeviationSourceWidget.h"
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
                                             const ItemCostProvider &costProvider,
                                             QWidget *parent)
        : QWidget(parent)
        , mExternalOrderRepo(externalOrderRepo)
        , mOrderRepo(orderRepo)
        , mCorpOrderRepo(corpOrderRepo)
        , mDataProvider(dataProvider)
        , mNameModel(mDataProvider)
        , mOrderNameModel(mDataProvider)
        , mExternalOrderSellModel(mDataProvider, mExternalOrderRepo, characterRepo, orderProvider, corpOrderProvider, costProvider)
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

        auto deviationWidget = new DeviationSourceWidget{this};
        mExternalOrderSellModel.changeDeviationSource(deviationWidget->getCurrentType(), deviationWidget->getCurrentValue());
        connect(deviationWidget, &DeviationSourceWidget::sourceChanged, this, &MarketBrowserWidget::changeDeviationSource);

        auto deviationAction = new QWidgetAction{this};
        deviationAction->setDefaultWidget(deviationWidget);

        auto deviationMenu = new QMenu{this};
        deviationMenu->addAction(deviationAction);

        mDeviationBtn = new QPushButton{QIcon{":/images/tag.png"}, getDeviationButtonText(deviationWidget->getCurrentType()), this};
        toolbarLayout->addWidget(mDeviationBtn);
        mDeviationBtn->setFlat(true);
        mDeviationBtn->setMenu(deviationMenu);

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
        connect(mRegionList, &QListWidget::currentItemChanged, this, &MarketBrowserWidget::selectRegion);

        filterLabel = new QLabel{tr("Solar systems [<a href='#'>all</a>]"), this};
        navigatorGroupLayout->addWidget(filterLabel);
        connect(filterLabel, &QLabel::linkActivated, this, [this] {
            mSolarSystemList->setCurrentRow(0);
        });

        mSolarSystemList = new QListWidget{this};
        navigatorGroupLayout->addWidget(mSolarSystemList);
        connect(mSolarSystemList, &QListWidget::currentItemChanged, this, &MarketBrowserWidget::selectSolarSystem);

        filterLabel = new QLabel{tr("Stations [<a href='#'>all</a>]"), this};
        navigatorGroupLayout->addWidget(filterLabel);
        connect(filterLabel, &QLabel::linkActivated, this, [this] {
            mStationList->setCurrentRow(0);
        });

        mStationList = new QListWidget{this};
        navigatorGroupLayout->addWidget(mStationList);
        connect(mStationList, &QListWidget::currentItemChanged, this, &MarketBrowserWidget::selectStation);

        auto orderLayout = new QVBoxLayout{};
        mainViewLayout->addLayout(orderLayout, 1);

        auto sellGroup = new QGroupBox{tr("Sell orders"), this};
        orderLayout->addWidget(sellGroup);

        auto sellLayout = new QVBoxLayout{};
        sellGroup->setLayout(sellLayout);

        mSellView = new ExternalOrderView{this};
        sellLayout->addWidget(mSellView);
        mSellView->setModel(&mExternalOrderSellModel);

        fillRegions();
    }

    void MarketBrowserWidget::setCharacter(Character::IdType id)
    {
        mExternalOrderSellModel.setCharacter(id);
    }

    void MarketBrowserWidget::updateData()
    {
        fillRegions();
        fillKnownItemNames();
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
        mSolarSystemList->blockSignals(true);
        mSolarSystemList->clear();
        mSolarSystemList->blockSignals(false);

        if (item != nullptr)
        {
            const auto solarSystems = mExternalOrderRepo.fetchUniqueSolarSystems(item->data(Qt::UserRole).toUInt());
            for (const auto &solarSystem : solarSystems)
            {
                auto item = new QListWidgetItem{mDataProvider.getSolarSystemName(solarSystem), mSolarSystemList};
                item->setData(Qt::UserRole, solarSystem);
            }
        }

        mSolarSystemList->sortItems();

        auto allItem = new QListWidgetItem{tr("(all)")};
        mSolarSystemList->insertItem(0, allItem);
        mSolarSystemList->setCurrentItem(allItem);
    }

    void MarketBrowserWidget::selectSolarSystem(QListWidgetItem *item)
    {
        mStationList->blockSignals(true);
        mStationList->clear();
        mStationList->blockSignals(false);

        if (item != nullptr)
        {
            std::vector<uint> stations;

            if (item->data(Qt::UserRole).toUInt() == 0)
            {
                const auto regionItem = mRegionList->currentItem();
                if (regionItem == nullptr || regionItem->data(Qt::UserRole).toUInt() == 0)
                    stations = mExternalOrderRepo.fetchUniqueStations();
                else
                    stations = mExternalOrderRepo.fetchUniqueStationsByRegion(regionItem->data(Qt::UserRole).toUInt());
            }
            else
            {
                stations = mExternalOrderRepo.fetchUniqueStationsBySolarSystem(item->data(Qt::UserRole).toUInt());
            }

            for (const auto &station : stations)
            {
                auto item = new QListWidgetItem{mDataProvider.getLocationName(station), mStationList};
                item->setData(Qt::UserRole, station);
            }
        }

        mStationList->sortItems();

        auto allItem = new QListWidgetItem{tr("(all)")};
        mStationList->insertItem(0, allItem);
        mStationList->setCurrentItem(allItem);
    }

    void MarketBrowserWidget::selectStation(QListWidgetItem *item)
    {
        if (item != nullptr)
        {
            mExternalOrderSellModel.setRegionId(mRegionList->currentItem()->data(Qt::UserRole).toUInt());
            mExternalOrderSellModel.setSolarSystemId(mSolarSystemList->currentItem()->data(Qt::UserRole).toUInt());
            mExternalOrderSellModel.setStationId(item->data(Qt::UserRole).toUInt());

            mExternalOrderSellModel.reset();
        }
    }

    void MarketBrowserWidget::changeDeviationSource(ExternalOrderModel::DeviationSourceType type, double value)
    {
        mDeviationBtn->setText(getDeviationButtonText(type));
        mExternalOrderSellModel.changeDeviationSource(type, value);
    }

    ExternalOrderImporter::TypeLocationPairs MarketBrowserWidget::getImportTarget() const
    {
        ExternalOrderImporter::TypeLocationPairs result;

        const auto orders = mExternalOrderRepo.fetchUniqueTypesAndStations();
        for (const auto &order : orders)
            result.emplace(std::make_pair(order.first, order.second));

        return result;
    }

    void MarketBrowserWidget::fillRegions()
    {
        mRegionList->blockSignals(true);
        mRegionList->clear();
        mRegionList->blockSignals(false);

        const auto regions = mExternalOrderRepo.fetchUniqueRegions();
        for (const auto &region : regions)
        {
            auto item = new QListWidgetItem{mDataProvider.getRegionName(region), mRegionList};
            item->setData(Qt::UserRole, region);
        }

        mRegionList->sortItems();

        auto allItem = new QListWidgetItem{tr("(all)")};
        mRegionList->insertItem(0, allItem);
        mRegionList->setCurrentItem(allItem);
    }

    void MarketBrowserWidget::fillKnownItemNames()
    {
        mNameModel.setTypes(mExternalOrderRepo.fetchUniqueTypes());
    }

    void MarketBrowserWidget::showOrdersForType(EveType::IdType typeId)
    {
        mExternalOrderSellModel.setType(typeId);
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
        connect(view->selectionModel(), &QItemSelectionModel::currentChanged, this, [knownItemsProxy, &model, this](const auto &index) {
            if (index.isValid())
                showOrdersForType(model.data(knownItemsProxy->mapToSource(index), Qt::UserRole).template value<Evernus::EveType::IdType>());
        });

        return knownItemsTab;
    }

    QString MarketBrowserWidget::getDeviationButtonText(ExternalOrderModel::DeviationSourceType type) const
    {
        switch (type) {
        case ExternalOrderModel::DeviationSourceType::Median:
            return tr("Deviation [median]");
        case ExternalOrderModel::DeviationSourceType::Best:
            return tr("Deviation [best price]");
        case ExternalOrderModel::DeviationSourceType::Cost:
            return tr("Deviation [custom cost]");
        case ExternalOrderModel::DeviationSourceType::Fixed:
            return tr("Deviation [fixed]");
        default:
            return tr("Deviation");
        }
    }
}
