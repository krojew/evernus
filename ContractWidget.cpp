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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QDebug>

#include "CacheTimerProvider.h"
#include "WarningBarWidget.h"
#include "TextFilterWidget.h"
#include "ButtonWithTimer.h"
#include "ImportSettings.h"
#include "ContractView.h"

#include "ContractWidget.h"

namespace Evernus
{
    ContractWidget::ContractWidget(const CacheTimerProvider &cacheTimerProvider,
                                   const EveDataProvider &dataProvider,
                                   const ContractProvider &contractProvider,
                                   const FilterTextRepository &filterRepo,
                                   const CharacterRepository &characterRepo,
                                   bool corp,
                                   QWidget *parent)
        : CharacterBoundWidget(std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpContracts) : (TimerType::Contracts)),
                               std::bind(&CacheTimerProvider::getLocalUpdateTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpContracts) : (TimerType::Contracts)),
                               ImportSettings::maxContractsAgeKey,
                               parent)
        , mIssuedModel(dataProvider, contractProvider, characterRepo, corp)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        auto filterEdit = new TextFilterWidget{filterRepo, this};
        toolBarLayout->addWidget(filterEdit, 1);

        auto &warningBar = getWarningBarWidget();
        mainLayout->addWidget(&warningBar);

        auto tabs = new QTabWidget{this};
        mainLayout->addWidget(tabs, 1);

        auto issuedView = new ContractView{this};
        tabs->addTab(issuedView, tr("Issued"));
        issuedView->setModel(&mIssuedModel);
        connect(filterEdit, &TextFilterWidget::filterEntered, issuedView, &ContractView::setFilterWildcard);
    }

    void ContractWidget::updateData()
    {
        refreshImportTimer();
        mIssuedModel.reset();
    }

    void ContractWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching contracts to" << id;

        mIssuedModel.setCharacter(id);
    }
}
