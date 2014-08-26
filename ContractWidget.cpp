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
#include <QDebug>

#include "CacheTimerProvider.h"
#include "WarningBarWidget.h"
#include "ButtonWithTimer.h"
#include "ImportSettings.h"

#include "ContractWidget.h"

namespace Evernus
{
    ContractWidget::ContractWidget(const CacheTimerProvider &cacheTimerProvider, bool corp, QWidget *parent)
        : CharacterBoundWidget(std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpContracts) : (TimerType::Contracts)),
                               std::bind(&CacheTimerProvider::getLocalUpdateTimer, &cacheTimerProvider, std::placeholders::_1, (corp) ? (TimerType::CorpContracts) : (TimerType::Contracts)),
                               ImportSettings::maxContractsAgeKey,
                               parent)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        auto &warningBar = getWarningBarWidget();
        mainLayout->addWidget(&warningBar);
    }

    void ContractWidget::updateData()
    {
        refreshImportTimer();
    }

    void ContractWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching contracts to" << id;
    }
}
