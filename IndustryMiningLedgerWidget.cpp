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
#include <QtDebug>

#include <QVBoxLayout>
#include <QHBoxLayout>

#include "CacheTimerProvider.h"
#include "WarningBarWidget.h"
#include "ButtonWithTimer.h"
#include "ImportSettings.h"
#include "TimerTypes.h"

#include "IndustryMiningLedgerWidget.h"

namespace Evernus
{
    IndustryMiningLedgerWidget::IndustryMiningLedgerWidget(const CacheTimerProvider &cacheTimerProvider,
                                                           QWidget *parent)
        : CharacterBoundWidget{std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, TimerType::MiningLedger),
                               std::bind(&CacheTimerProvider::getLocalUpdateTimer, &cacheTimerProvider, std::placeholders::_1, TimerType::MiningLedger),
                               ImportSettings::maxCharacterAgeKey,parent}
    {
        const auto mainLayout = new QVBoxLayout{this};

        const auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        toolBarLayout->addStretch();

        auto &warningBar = getWarningBarWidget();
        mainLayout->addWidget(&warningBar);
    }

    void IndustryMiningLedgerWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching character to" << id;
    }
}
