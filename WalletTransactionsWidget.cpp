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
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        toolBarLayout->addStretch();
    }

    void WalletTransactionsWidget::updateData()
    {
        refreshImportTimer();
    }

    void WalletTransactionsWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching wallet transactions to" << id;
    }
}
