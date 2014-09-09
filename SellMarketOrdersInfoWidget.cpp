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
#include <QSettings>
#include <QLabel>
#include <QFont>

#include "MarketOrderSellModel.h"
#include "PriceSettings.h"
#include "FlowLayout.h"
#include "PriceUtils.h"

#include "SellMarketOrdersInfoWidget.h"

namespace Evernus
{
    SellMarketOrdersInfoWidget::SellMarketOrdersInfoWidget(const MarketOrderSellModel &model, QWidget *parent)
        : GenericMarketOrdersInfoWidget(model, parent)
        , mModel(model)
    {
        QFont font;
        font.setBold(true);

        auto mainLayout = layout();

        mainLayout->addWidget(new QLabel{tr("Total income:"), this});

        mTotalIncomeLabel = new QLabel{this};
        mainLayout->addWidget(mTotalIncomeLabel);
        mTotalIncomeLabel->setFont(font);
        mTotalIncomeLabel->setStyleSheet("color: darkGreen;");

        mainLayout->addWidget(new QLabel{tr("Total cost:"), this});

        mTotalCostLabel = new QLabel{this};
        mainLayout->addWidget(mTotalCostLabel);
        mTotalCostLabel->setFont(font);
        mTotalCostLabel->setStyleSheet("color: darkRed;");

        mainLayout->addWidget(new QLabel{tr("Total margin:"), this});

        mTotalMarginLabel = new QLabel{this};
        mainLayout->addWidget(mTotalMarginLabel);
        mTotalMarginLabel->setFont(font);
    }

    void SellMarketOrdersInfoWidget::updateData()
    {
        GenericMarketOrdersInfoWidget::updateData();

        const auto curLocale = locale();
        const auto income = mModel.getTotalIncome();
        const auto cost = mModel.getTotalCost();
        const auto margin = 100. * (income - cost) / income;

        mTotalIncomeLabel->setText(curLocale.toCurrencyString(income, "ISK"));
        mTotalCostLabel->setText(curLocale.toCurrencyString(cost, "ISK"));
        mTotalMarginLabel->setText(QString{"%1%2"}.arg(curLocale.toString(margin, 'f', 2)).arg(curLocale.percent()));

        QSettings settings;
        if (margin < settings.value(PriceSettings::minMarginKey, PriceSettings::minMarginDefault).toDouble())
            mTotalMarginLabel->setStyleSheet("color: red;");
        else if (margin < settings.value(PriceSettings::preferredMarginKey, PriceSettings::preferredMarginDefault).toDouble())
            mTotalMarginLabel->setStyleSheet("color: #ffa500;");
        else
            mTotalMarginLabel->setStyleSheet("color: green;");
    }
}
