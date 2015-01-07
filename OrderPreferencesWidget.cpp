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
#include <QGroupBox>
#include <QCheckBox>
#include <QSettings>
#include <QSpinBox>

#include "OrderSettings.h"

#include "OrderPreferencesWidget.h"

namespace Evernus
{
    OrderPreferencesWidget::OrderPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto mainGroup = new QGroupBox{this};
        mainLayout->addWidget(mainGroup);

        auto mainGroupLayout = new QFormLayout{mainGroup};

        QSettings settings;

        mMarketOrderMaxAgeEdit = new QSpinBox{this};
        mainGroupLayout->addRow(tr("Max. market order age:"), mMarketOrderMaxAgeEdit);
        mMarketOrderMaxAgeEdit->setMinimum(1);
        mMarketOrderMaxAgeEdit->setMaximum(365);
        mMarketOrderMaxAgeEdit->setSuffix(tr(" days"));
        mMarketOrderMaxAgeEdit->setValue(settings.value(OrderSettings::marketOrderMaxAgeKey, OrderSettings::marketOrderMaxAgeDefault).toUInt());

        mOldMarketOrdersDaysEdit = new QSpinBox{this};

        mDeleteOldMarketOrdersBtn = new QCheckBox{tr("Delete old fulfilled orders"), this};
        mainGroupLayout->addRow(mDeleteOldMarketOrdersBtn);
        mDeleteOldMarketOrdersBtn->setChecked(settings.value(OrderSettings::deleteOldMarketOrdersKey, OrderSettings::deleteOldMarketOrdersDefault).toBool());
        connect(mDeleteOldMarketOrdersBtn, &QCheckBox::stateChanged, mOldMarketOrdersDaysEdit, &QSpinBox::setEnabled);

        mainGroupLayout->addRow(tr("Delete older than:"), mOldMarketOrdersDaysEdit);
        mOldMarketOrdersDaysEdit->setMinimum(14);
        mOldMarketOrdersDaysEdit->setMaximum(365 * 2);
        mOldMarketOrdersDaysEdit->setSuffix(tr(" days"));
        mOldMarketOrdersDaysEdit->setValue(settings.value(OrderSettings::oldMarketOrderDaysKey, OrderSettings::oldMarketOrderDaysDefault).toUInt());
        mOldMarketOrdersDaysEdit->setEnabled(mDeleteOldMarketOrdersBtn->isChecked());

        mainLayout->addStretch();
    }

    void OrderPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(OrderSettings::marketOrderMaxAgeKey, mMarketOrderMaxAgeEdit->value());
        settings.setValue(OrderSettings::deleteOldMarketOrdersKey, mDeleteOldMarketOrdersBtn->isChecked());
        settings.setValue(OrderSettings::oldMarketOrderDaysKey, mOldMarketOrdersDaysEdit->value());
    }
}
