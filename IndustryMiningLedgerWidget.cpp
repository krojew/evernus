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
#include <QHeaderView>
#include <QPushButton>
#include <QGroupBox>
#include <QDate>

#include "AdjustableTableView.h"
#include "CacheTimerProvider.h"
#include "LookupActionGroup.h"
#include "WarningBarWidget.h"
#include "DateRangeWidget.h"
#include "ButtonWithTimer.h"
#include "ImportSettings.h"
#include "FlowLayout.h"
#include "TimerTypes.h"

#include "IndustryMiningLedgerWidget.h"

namespace Evernus
{
    IndustryMiningLedgerWidget::IndustryMiningLedgerWidget(const CacheTimerProvider &cacheTimerProvider,
                                                           const EveDataProvider &dataProvider,
                                                           const MiningLedgerRepository &ledgerRepo,
                                                           QWidget *parent)
        : CharacterBoundWidget{std::bind(&CacheTimerProvider::getLocalCacheTimer, &cacheTimerProvider, std::placeholders::_1, TimerType::MiningLedger),
                               std::bind(&CacheTimerProvider::getLocalUpdateTimer, &cacheTimerProvider, std::placeholders::_1, TimerType::MiningLedger),
                               ImportSettings::maxCharacterAgeKey,parent}
        , EveTypeProvider{}
        , mDetailsModel{dataProvider, ledgerRepo}
    {
        const auto mainLayout = new QVBoxLayout{this};

        const auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        auto &importBtn = getAPIImportButton();
        toolBarLayout->addWidget(&importBtn);

        const auto tillDate = QDate::currentDate();
        const auto fromDate = tillDate.addDays(-7);

        mRangeFilter = new DateRangeWidget{this};
        toolBarLayout->addWidget(mRangeFilter);
        mRangeFilter->setRange(fromDate, tillDate);
        connect(mRangeFilter, &DateRangeWidget::rangeChanged, this, &IndustryMiningLedgerWidget::refresh);

        const auto importFromWeb = new QPushButton{QIcon{":/images/world.png"}, tr("Import data"), this};
        toolBarLayout->addWidget(importFromWeb);
        importFromWeb->setFlat(true);
        connect(importFromWeb, &QPushButton::clicked, this, &IndustryMiningLedgerWidget::importData);

        auto &warningBar = getWarningBarWidget();
        mainLayout->addWidget(&warningBar);

        const auto detailsGroup = new QGroupBox{tr("Details"), this};
        mainLayout->addWidget(detailsGroup);

        const auto detailsGroupLayout = new QVBoxLayout{detailsGroup};

        mDetailsProxy.setSourceModel(&mDetailsModel);

        mDetailsView = new AdjustableTableView{QStringLiteral("industryMiningLedgerDetailsView"), this};
        detailsGroupLayout->addWidget(mDetailsView);
        mDetailsView->setModel(&mDetailsProxy);
        mDetailsView->setSortingEnabled(true);
        mDetailsView->setAlternatingRowColors(true);
        mDetailsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        mDetailsView->setContextMenuPolicy(Qt::ActionsContextMenu);
        mDetailsView->restoreHeaderState();

        mLookupGroup = new LookupActionGroup{*this, this};
        mLookupGroup->setEnabled(false);
        mDetailsView->addActions(mLookupGroup->actions());
    }

    EveType::IdType IndustryMiningLedgerWidget::getTypeId() const
    {
        return mDetailsModel.getTypeId(mDetailsProxy.mapToSource(mDetailsView->currentIndex()));
    }

    void IndustryMiningLedgerWidget::refresh()
    {
        Q_ASSERT(mRangeFilter != nullptr);
        mDetailsModel.refresh(getCharacterId(), mRangeFilter->getFrom(), mRangeFilter->getTo());
    }

    void IndustryMiningLedgerWidget::importData()
    {

    }

    void IndustryMiningLedgerWidget::handleNewCharacter(Character::IdType id)
    {
        qDebug() << "Switching character to" << id;
        refresh();
    }
}
