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
#include <QTreeView>
#include <QDebug>

#include "WalletEntryFilterWidget.h"
#include "CacheTimerProvider.h"
#include "ButtonWithTimer.h"

#include "WalletTransactionsWidget.h"

namespace Evernus
{
    WalletTransactionsWidget::WalletTransactionsWidget(const WalletTransactionRepository &walletRepo,
                                                       const CacheTimerProvider &cacheTimerProvider,
                                                       const EveDataProvider &dataProvider,
                                                       QWidget *parent)
        : CharacterBoundWidget{std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, CacheTimerProvider::TimerType::WalletTransactions), parent}
        , mModel{walletRepo, dataProvider}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        toolBarLayout->addStretch();

        mFilter = new WalletEntryFilterWidget{QStringList{} << tr("all") << tr("buy") << tr("sell"), this};
        mainLayout->addWidget(mFilter);
        connect(mFilter, &WalletEntryFilterWidget::filterChanged, this, &WalletTransactionsWidget::updateFilter);

        mFilterModel = new QSortFilterProxyModel{this};
        mFilterModel->setSortRole(Qt::UserRole);
        mFilterModel->setFilterKeyColumn(-1);
        mFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        mFilterModel->setSourceModel(&mModel);

        auto journalView = new QTreeView{this};
        mainLayout->addWidget(journalView, 1);
        journalView->setModel(mFilterModel);
        journalView->setSortingEnabled(true);
        journalView->sortByColumn(1, Qt::DescendingOrder);
        journalView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    }

    void WalletTransactionsWidget::updateData()
    {
        refreshImportTimer();
        mModel.reset();
    }

    void WalletTransactionsWidget::updateFilter(const QDate &from, const QDate &to, const QString &filter, int type)
    {
        mModel.setFilter(getCharacterId(), from, to, static_cast<EntryType>(type));
        mFilterModel->setFilterFixedString(filter);
    }

    void WalletTransactionsWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching wallet transactions to" << id;

        const auto tillDate = QDate::currentDate();
        const auto fromDate = tillDate.addMonths(-1);

        mFilter->blockSignals(true);
        mFilter->setFilter(fromDate, tillDate, QString{}, static_cast<int>(EntryType::All));
        mFilter->blockSignals(false);

        mModel.setFilter(id, fromDate, tillDate, EntryType::All);
    }
}
