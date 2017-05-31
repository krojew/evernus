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
#include <QMessageBox>
#include <QHeaderView>
#include <QCheckBox>
#include <QSettings>
#include <QAction>
#include <QLabel>
#include <QDebug>

#include "MarketAnalysisSettings.h"
#include "CalculatingDataWidget.h"
#include "AdjustableTableView.h"
#include "StationSelectButton.h"
#include "MarketDataProvider.h"
#include "RegionComboBox.h"
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

        const auto srcStationPath = settings.value(MarketAnalysisSettings::reprocessingSrcStationKey).toList();
        if (srcStationPath.size() == 4)
            mSrcStation = srcStationPath[3].toULongLong();
        const auto dstStationPath = settings.value(MarketAnalysisSettings::reprocessingDstStationKey).toList();
        if (dstStationPath.size() == 4)
            mDstStation = dstStationPath[3].toULongLong();

        toolBarLayout->addWidget(new QLabel{tr("Source:"), this});
        mSourceRegionCombo = new RegionComboBox{dataProvider, MarketAnalysisSettings::reprocessingSrcRegionKey, this};
        toolBarLayout->addWidget(mSourceRegionCombo);

        auto stationBtn = new StationSelectButton{dataProvider, srcStationPath, this};
        toolBarLayout->addWidget(stationBtn);
        connect(stationBtn, &StationSelectButton::stationChanged, this, [=](const auto &path) {
            changeStation(mSrcStation, path, MarketAnalysisSettings::reprocessingSrcStationKey);
        });

        toolBarLayout->addWidget(new QLabel{tr("Destination:"), this});
        mDestRegionCombo = new RegionComboBox{dataProvider, MarketAnalysisSettings::reprocessingDstRegionKey, this};
        toolBarLayout->addWidget(mDestRegionCombo);

        stationBtn = new StationSelectButton{dataProvider, dstStationPath, this};
        toolBarLayout->addWidget(stationBtn);
        connect(stationBtn, &StationSelectButton::stationChanged, this, [=](const auto &path) {
            changeStation(mDstStation, path, MarketAnalysisSettings::reprocessingDstStationKey);
        });

        toolBarLayout->addWidget(new QLabel{tr("Base yield:"), this});

        mStationEfficiencyEdit = new QDoubleSpinBox{this};
        toolBarLayout->addWidget(mStationEfficiencyEdit);
        mStationEfficiencyEdit->setRange(0., 100.);
        mStationEfficiencyEdit->setSuffix(locale().percent());
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
        toolBarLayout->addWidget(new QLabel{tr("If you wish to make the fastest trade as possible, be sure to set the correct <b>destination price type</b>. <b>Buying always uses sell orders.</b>"), this});
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

    void OreReprocessingArbitrageWidget::setPriceType(PriceType dst) noexcept
    {
        mDstPriceType = dst;
    }

    void OreReprocessingArbitrageWidget::recalculateData()
    {
        qDebug() << "Recomputing ore reprocessing arbitrage data...";

        const auto orders = mMarketDataProvider.getOrders();
        if (orders == nullptr)
            return;

        mDataStack->setCurrentIndex(waitingLabelIndex);
        mDataStack->repaint();

        mDataModel.setOrderData(*orders,
                                mDstPriceType,
                                mSourceRegionCombo->getSelectedRegionList(),
                                mDestRegionCombo->getSelectedRegionList(),
                                mSrcStation,
                                mDstStation,
                                mIncludeStationTaxBtn->isChecked(),
                                mIgnoreMinVolumeBtn->isChecked(),
                                mStationEfficiencyEdit->value());

        mDataView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);

        mDataStack->setCurrentWidget(mDataView);
        mDataStack->repaint();
    }

    void OreReprocessingArbitrageWidget::changeStation(quint64 &destination, const QVariantList &path, const QString &settingName)
    {
        QSettings settings;
        settings.setValue(settingName, path);

        if (path.size() == 4)
            destination = path[3].toULongLong();
        else
            destination = 0;

        if (QMessageBox::question(this, tr("Station change"), tr("Changing station requires data recalculation. Do you wish to do it now?")) == QMessageBox::No)
            return;

        recalculateData();
    }
}
