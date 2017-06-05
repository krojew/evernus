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
#include <QHeaderView>
#include <QCheckBox>
#include <QGroupBox>
#include <QSettings>
#include <QDate>

#include "AdjustableTableView.h"
#include "RepositoryProvider.h"
#include "DateRangeWidget.h"
#include "FlowLayout.h"
#include "UISettings.h"

#include "ReportsWidget.h"

namespace Evernus
{
    ReportsWidget::ReportsWidget(const RepositoryProvider &repositoryProvider,
                                 const EveDataProvider &dataProvider,
                                 QWidget *parent)
        : QWidget{parent}
        , mPerformanceModel{repositoryProvider.getWalletTransactionRepository(),
                            repositoryProvider.getCorpWalletTransactionRepository(),
                            repositoryProvider.getCharacterRepository(),
                            dataProvider}
    {
        const auto mainLayout = new QVBoxLayout{this};

        const auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        const auto tillDate = QDate::currentDate();
        const auto fromDate = tillDate.addDays(-7);

        mDateRangeEdit = new DateRangeWidget{this};
        toolBarLayout->addWidget(mDateRangeEdit);
        mDateRangeEdit->setRange(fromDate, tillDate);
        connect(mDateRangeEdit, &DateRangeWidget::rangeChanged, this, &ReportsWidget::recalculateData);

        QSettings settings;

        mCombineBtn = new QCheckBox{tr("Combine for all characters"), this};
        toolBarLayout->addWidget(mCombineBtn);
        mCombineBtn->setChecked(settings.value(UISettings::combineReportsKey, UISettings::combineReportsDefault).toBool());
        connect(mCombineBtn, &QCheckBox::toggled, this, [=](bool checked) {
            QSettings settings;
            settings.setValue(UISettings::combineReportsKey, checked);

            recalculateData();
        });

        mCombineWithCorpBtn = new QCheckBox{tr("Combine with corp. data"), this};
        toolBarLayout->addWidget(mCombineWithCorpBtn);
        mCombineWithCorpBtn->setChecked(settings.value(UISettings::combineReportsWithCorpKey, UISettings::combineReportsWithCorpDefault).toBool());
        connect(mCombineWithCorpBtn, &QCheckBox::toggled, this, [=](bool checked) {
            QSettings settings;
            settings.setValue(UISettings::combineReportsWithCorpKey, checked);

            recalculateData();
        });

        mPerformanceProxy.setSortRole(Qt::UserRole);
        mPerformanceProxy.setSourceModel(&mPerformanceModel);

        const auto bestItemsGroup = new QGroupBox{tr("Best items"), this};
        mainLayout->addWidget(bestItemsGroup);

        const auto bestItemsGroupLayout = new QVBoxLayout{bestItemsGroup};

        mBestItemsView = new AdjustableTableView{QStringLiteral("reportsBestItemsView"), this};
        bestItemsGroupLayout->addWidget(mBestItemsView);
        mBestItemsView->setSortingEnabled(true);
        mBestItemsView->setAlternatingRowColors(true);
        mBestItemsView->setModel(&mPerformanceProxy);
        mBestItemsView->restoreHeaderState();
    }

    void ReportsWidget::setCharacter(Character::IdType id)
    {
        const auto prevCharactedId = mCharacterId;

        mCharacterId = id;

        if (!mCombineBtn->isChecked() || prevCharactedId == Character::invalidId)
            recalculateData();
    }

    void ReportsWidget::recalculateData()
    {
        mPerformanceModel.reset(mDateRangeEdit->getFrom(),
                                mDateRangeEdit->getTo(),
                                mCombineBtn->isChecked(),
                                mCombineWithCorpBtn->isChecked(),
                                mCharacterId);

        mBestItemsView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
        mBestItemsView->sortByColumn(TypePerformanceModel::profitColumn, Qt::DescendingOrder);
    }
}
