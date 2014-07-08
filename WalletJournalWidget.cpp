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

#include "WalletEntryFilterWidget.h"
#include "ButtonWithTimer.h"
#include "APIManager.h"

#include "WalletJournalWidget.h"

namespace Evernus
{
    WalletJournalWidget::WalletJournalWidget(const WalletJournalEntryRepository &journalRepo, const APIManager &apiManager, QWidget *parent)
        : CharacterBoundWidget{std::bind(&APIManager::getWalletJournalLocalCacheTime, &apiManager, std::placeholders::_1), parent}
        , mModel{journalRepo}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        toolBarLayout->addStretch();

        auto filter = new WalletEntryFilterWidget{this};
        mainLayout->addWidget(filter);

        auto proxy = new QSortFilterProxyModel{this};
        proxy->setSourceModel(&mModel);

        auto journalView = new QTreeView{this};
        mainLayout->addWidget(journalView, 1);
        journalView->setModel(proxy);
        journalView->setSortingEnabled(true);
        journalView->header()->setSectionResizeMode(QHeaderView::Stretch);
    }

    void WalletJournalWidget::updateData()
    {
        refreshImportTimer();
        mModel.reset();
    }

    void WalletJournalWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching wallet journal to" << id;

        mModel.setCharacter(id);
    }
}
