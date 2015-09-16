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
#include <QFormLayout>
#include <QSettings>
#include <QGroupBox>

#include "ColorButton.h"
#include "UISettings.h"

#include "StatisticsPreferencesWidget.h"

namespace Evernus
{
    StatisticsPreferencesWidget::StatisticsPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto appearanceGroup = new QGroupBox{tr("Appearance"), this};
        mainLayout->addWidget(appearanceGroup);

        auto appearanceGroupLayout = new QFormLayout{appearanceGroup};

        QSettings settings;

        mAssetPlotColorBtn = new ColorButton{
            settings.value(UISettings::statisticsAssetPlotColorKey, UISettings::statisticsAssetPlotColorDefault).value<QColor>(),
            this
        };
        appearanceGroupLayout->addRow(tr("Asset value plot color:"), mAssetPlotColorBtn);
        mWalletPlotColorBtn = new ColorButton{
            settings.value(UISettings::statisticsWalletPlotColorKey, UISettings::statisticsWalletPlotColorDefault).value<QColor>(),
            this
        };
        appearanceGroupLayout->addRow(tr("Wallet balance plot color:"), mWalletPlotColorBtn);
        mCorpWalletPlotColorBtn = new ColorButton{
            settings.value(UISettings::statisticsCorpWalletPlotColorKey, UISettings::statisticsCorpWalletPlotColorDefault).value<QColor>(),
            this
        };
        appearanceGroupLayout->addRow(tr("Corp. wallet balance plot color:"), mCorpWalletPlotColorBtn);
        mBuyOrdersPlotColorBtn = new ColorButton{
            settings.value(UISettings::statisticsBuyOrderPlotColorKey, UISettings::statisticsBuyOrderPlotColorDefault).value<QColor>(),
            this
        };
        appearanceGroupLayout->addRow(tr("Buy order value plot color:"), mBuyOrdersPlotColorBtn);
        mSellOrdersPlotColorBtn = new ColorButton{
            settings.value(UISettings::statisticsSellOrderPlotColorKey, UISettings::statisticsSellOrderPlotColorDefault).value<QColor>(),
            this
        };
        appearanceGroupLayout->addRow(tr("Sell order value plot color:"), mSellOrdersPlotColorBtn);
        mTotalPlotColorBtn = new ColorButton{
            settings.value(UISettings::statisticsTotalPlotColorKey, UISettings::statisticsTotalPlotColorDefault).value<QColor>(),
            this
        };
        appearanceGroupLayout->addRow(tr("Total value plot color:"), mTotalPlotColorBtn);

        auto resetColors = new QPushButton(tr("Reset to default"), this);
        appearanceGroupLayout->addRow(resetColors);
        connect(resetColors, &QPushButton::clicked, this, &StatisticsPreferencesWidget::resetPlotColors);

        mainLayout->addStretch();
    }

    void StatisticsPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(UISettings::statisticsAssetPlotColorKey, mAssetPlotColorBtn->getColor());
        settings.setValue(UISettings::statisticsWalletPlotColorKey, mWalletPlotColorBtn->getColor());
        settings.setValue(UISettings::statisticsCorpWalletPlotColorKey, mCorpWalletPlotColorBtn->getColor());
        settings.setValue(UISettings::statisticsBuyOrderPlotColorKey, mBuyOrdersPlotColorBtn->getColor());
        settings.setValue(UISettings::statisticsSellOrderPlotColorKey, mSellOrdersPlotColorBtn->getColor());
        settings.setValue(UISettings::statisticsTotalPlotColorKey, mTotalPlotColorBtn->getColor());
    }

    void StatisticsPreferencesWidget::resetPlotColors()
    {
        mAssetPlotColorBtn->setColor(UISettings::statisticsAssetPlotColorDefault);
        mWalletPlotColorBtn->setColor(UISettings::statisticsWalletPlotColorDefault);
        mCorpWalletPlotColorBtn->setColor(UISettings::statisticsCorpWalletPlotColorDefault);
        mBuyOrdersPlotColorBtn->setColor(UISettings::statisticsBuyOrderPlotColorDefault);
        mSellOrdersPlotColorBtn->setColor(UISettings::statisticsSellOrderPlotColorDefault);
        mTotalPlotColorBtn->setColor(UISettings::statisticsTotalPlotColorDefault);
    }
}
