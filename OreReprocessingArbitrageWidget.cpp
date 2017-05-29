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
#include <QStackedWidget>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSettings>
#include <QAction>
#include <QLabel>
#include <QDebug>

#include "MarketAnalysisSettings.h"
#include "CalculatingDataWidget.h"
#include "AdjustableTableView.h"
#include "MarketDataProvider.h"
#include "FlowLayout.h"

#include "OreReprocessingArbitrageWidget.h"

namespace Evernus
{
    OreReprocessingArbitrageWidget::OreReprocessingArbitrageWidget(const EveDataProvider &dataProvider,
                                                                   const MarketDataProvider &marketDataProvider,
                                                                   QWidget *parent)
        : StandardModelProxyWidget(mDataModel, mDataProxy, parent)
        , mMarketDataProvider(marketDataProvider)
        , mDataModel(dataProvider)
    {
        const auto mainLayout = new QVBoxLayout{this};

        auto toolBarLayout = new FlowLayout{};
        mainLayout->addLayout(toolBarLayout);

        QSettings settings;

        toolBarLayout->addWidget(new QLabel{tr("Base yield:"), this});

        mStationEfficiencyEdit = new QDoubleSpinBox{this};
        toolBarLayout->addWidget(mStationEfficiencyEdit);
        mStationEfficiencyEdit->setRange(0., 100.);
        mStationEfficiencyEdit->setValue(
            settings.value(MarketAnalysisSettings::reprocessingStationEfficiencyKey, MarketAnalysisSettings::reprocessingStationEfficiencyDefault).toDouble()
        );
        connect(mStationEfficiencyEdit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](auto value) {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::reprocessingStationEfficiencyKey, value);
        });

        mIncludeStationTaxBtn = new QCheckBox{tr("Include station tax"), this};
        toolBarLayout->addWidget(mIncludeStationTaxBtn);
        mIncludeStationTaxBtn->setChecked(
            settings.value(MarketAnalysisSettings::reprocessingIncludeStationTaxKey, MarketAnalysisSettings::reprocessingIncludeStationTaxDefault).toBool()
        );
        connect(mIncludeStationTaxBtn, &QCheckBox::stateChanged, this, [=](auto state) {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::reprocessingIncludeStationTaxKey, state == Qt::Checked);
        });

        mIgnoreMinVolumeBtn = new QCheckBox{tr("Ignore orders with min. volume > 1"), this};
        toolBarLayout->addWidget(mIgnoreMinVolumeBtn);
        mIgnoreMinVolumeBtn->setChecked(
            settings.value(MarketAnalysisSettings::reprocessingIgnoreMinVolumeKey, MarketAnalysisSettings::reprocessingIgnoreMinVolumeDefault).toBool()
        );
        connect(mIgnoreMinVolumeBtn, &QCheckBox::stateChanged, this, [=](auto state) {
            QSettings settings;
            settings.setValue(MarketAnalysisSettings::reprocessingIgnoreMinVolumeKey, state == Qt::Checked);
        });

        auto filterBtn = new QPushButton{tr("Apply"), this};
        toolBarLayout->addWidget(filterBtn);
        connect(filterBtn, &QPushButton::clicked, this, &OreReprocessingArbitrageWidget::recalculateData);

        toolBarLayout->addWidget(new QLabel{tr("Press \"Apply\" to show results. Additional actions are available via the right-click menu."), this});
        toolBarLayout->addWidget(new QLabel{tr("If you wish to make the fastest trade as possible, be sure to set the correct <b>buy and sell price type</b>."), this});
        toolBarLayout->addWidget(new QLabel{tr("Due to the fast nature of arbitrage, prices are not based on volume percentile."), this});

        mDataStack = new QStackedWidget{this};
        mainLayout->addWidget(mDataStack);

        mDataStack->addWidget(new CalculatingDataWidget{this});

        mDataProxy.setSortRole(Qt::UserRole);
        mDataProxy.setSourceModel(&mDataModel);

        mDataView = new AdjustableTableView{QStringLiteral("marketAnalysisOreReprocessingArbitrageView"), this};
        mDataStack->addWidget(mDataView);
        mDataView->setSortingEnabled(true);
        mDataView->setAlternatingRowColors(true);
        mDataView->setModel(&mDataProxy);
        mDataView->restoreHeaderState();

        mDataStack->setCurrentWidget(mDataView);

        installOnView(mDataView);
    }

    void OreReprocessingArbitrageWidget::setCharacter(std::shared_ptr<Character> character)
    {
        StandardModelProxyWidget::setCharacter((character) ? (character->getId()) : (Character::invalidId));
        mDataModel.setCharacter(std::move(character));
    }

    void OreReprocessingArbitrageWidget::clearData()
    {
        mDataModel.reset();
    }

    void OreReprocessingArbitrageWidget::recalculateData()
    {
        qDebug() << "Recomputing ore reprocessing arbitrage data...";

        const auto history = mMarketDataProvider.getHistory();
        if (history == nullptr)
            return;

        const auto orders = mMarketDataProvider.getOrders();
        if (orders == nullptr)
            return;

        mDataStack->setCurrentIndex(waitingLabelIndex);
        mDataStack->repaint();

        // TODO: set model data

        mDataStack->setCurrentWidget(mDataView);
        mDataStack->repaint();
    }
}
