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
#include <unordered_map>

#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QAction>
#include <QDebug>

#include "WalletEntryFilterWidget.h"
#include "CacheTimerProvider.h"
#include "WarningBarWidget.h"
#include "ItemCostProvider.h"
#include "ButtonWithTimer.h"
#include "StyledTreeView.h"
#include "ImportSettings.h"

#include "WalletTransactionsWidget.h"

namespace Evernus
{
    WalletTransactionsWidget::WalletTransactionsWidget(const WalletTransactionRepository &walletRepo,
                                                       const CharacterRepository &characterRepository,
                                                       const FilterTextRepository &filterRepo,
                                                       const CacheTimerProvider &cacheTimerProvider,
                                                       const EveDataProvider &dataProvider,
                                                       ItemCostProvider &itemCostProvider,
                                                       bool corp,
                                                       QWidget *parent)
        : CharacterBoundWidget(std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpWalletTransactions) : (TimerType::WalletTransactions)),
                               std::bind(&CacheTimerProvider::getLocalUpdateTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpWalletTransactions) : (TimerType::WalletTransactions)),
                               ImportSettings::maxWalletAgeKey,
                               parent)
        , mItemCostProvider(itemCostProvider)
        , mModel(walletRepo, characterRepository, dataProvider, corp)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        mFilter = new WalletEntryFilterWidget{QStringList{} << tr("all") << tr("buy") << tr("sell"), filterRepo, this};
        toolBarLayout->addWidget(mFilter, 1);
        connect(mFilter, &WalletEntryFilterWidget::filterChanged, this, &WalletTransactionsWidget::updateFilter);

        auto &warningBar = getWarningBarWidget();
        mainLayout->addWidget(&warningBar);

        mFilterModel = new QSortFilterProxyModel{this};
        mFilterModel->setSortRole(Qt::UserRole);
        mFilterModel->setFilterKeyColumn(-1);
        mFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        mFilterModel->setSourceModel(&mModel);

        auto addCostAct = new QAction{tr("Add to item costs"), this};
        connect(addCostAct, &QAction::triggered, this, &WalletTransactionsWidget::addItemCost);

        mView = new StyledTreeView{this};
        mainLayout->addWidget(mView, 1);
        mView->setModel(mFilterModel);
        mView->sortByColumn(1, Qt::DescendingOrder);
        mView->addAction(addCostAct);
    }

    void WalletTransactionsWidget::updateData()
    {
        refreshImportTimer();
        mModel.reset();
    }

    void WalletTransactionsWidget::updateFilter(const QDate &from, const QDate &to, const QString &filter, int type)
    {
        mModel.setFilter(getCharacterId(), from, to.addDays(1), static_cast<EntryType>(type));
        mFilterModel->setFilterWildcard(filter);
    }

    void WalletTransactionsWidget::addItemCost()
    {
        const auto selection = mView->selectionModel()->selectedIndexes();

        struct ItemData
        {
            uint mQuantity;
            double mPrice;
        };

        std::unordered_map<EveType::IdType, ItemData> aggrData;
        for (const auto &index : selection)
        {
            if (index.column() != 0)
                continue;

            const auto mappedIndex = mFilterModel->mapToSource(index);
            const auto row = mappedIndex.row();

            auto &data = aggrData[mModel.getTypeId(row)];

            const auto quantity = mModel.getQuantity(row);

            data.mQuantity += quantity;
            data.mPrice += quantity * mModel.getPrice(row);
        }

        for (const auto &data : aggrData)
            mItemCostProvider.setForCharacterAndType(getCharacterId(), data.first, data.second.mPrice / data.second.mQuantity);
    }

    void WalletTransactionsWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching wallet transactions to" << id;

        const auto tillDate = QDate::currentDate();
        const auto fromDate = tillDate.addDays(-7);

        mFilter->blockSignals(true);
        mFilter->setFilter(fromDate, tillDate, QString{}, static_cast<int>(EntryType::All));
        mFilter->blockSignals(false);

        mModel.setFilter(id, fromDate, tillDate, EntryType::All);

        mView->header()->resizeSections(QHeaderView::ResizeToContents);
    }
}
