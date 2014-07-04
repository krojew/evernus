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
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSettings>
#include <QGroupBox>
#include <QLabel>

#include "PriceSettings.h"

#include "PricePreferencesWidget.h"

namespace Evernus
{
    PricePreferencesWidget::PricePreferencesWidget(QWidget *parent)
        : QWidget{parent}
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto marginGroup = new QGroupBox{tr("Margins"), this};
        mainLayout->addWidget(marginGroup);

        auto marginLayout = new QHBoxLayout{};
        marginGroup->setLayout(marginLayout);

        marginLayout->addWidget(new QLabel{tr("Minimum:"), this});

        mMinMarginEdit = new QDoubleSpinBox{this};
        marginLayout->addWidget(mMinMarginEdit);
        mMinMarginEdit->setSuffix("%");
        mMinMarginEdit->setValue(settings.value(PriceSettings::minMarginKey, PriceSettings::minMarginDefault).toDouble());

        marginLayout->addWidget(new QLabel{tr("Preferred:"), this});

        mPreferredMarginEdit = new QDoubleSpinBox{this};
        marginLayout->addWidget(mPreferredMarginEdit);
        mPreferredMarginEdit->setSuffix("%");
        mPreferredMarginEdit->setValue(settings.value(PriceSettings::preferredMarginKey, PriceSettings::preferredMarginDefault).toDouble());

        auto pricesGroup = new QGroupBox{this};
        mainLayout->addWidget(pricesGroup);

        auto pricesLayout = new QHBoxLayout{};
        pricesGroup->setLayout(pricesLayout);

        pricesLayout->addWidget(new QLabel{tr("Price delta:"), this});

        mPriceDeltaEdit = new QDoubleSpinBox{this};
        pricesLayout->addWidget(mPriceDeltaEdit);
        mPriceDeltaEdit->setSingleStep(0.01);
        mPriceDeltaEdit->setMinimum(0.01);
        mPriceDeltaEdit->setMaximum(100000000.);
        mPriceDeltaEdit->setValue(settings.value(PriceSettings::priceDeltaKey, PriceSettings::priceDeltaDefault).toDouble());

        pricesLayout->addStretch();

        mainLayout->addStretch();
    }

    void PricePreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(PriceSettings::minMarginKey, mMinMarginEdit->value());
        settings.setValue(PriceSettings::preferredMarginKey, mPreferredMarginEdit->value());
        settings.setValue(PriceSettings::priceDeltaKey, mPriceDeltaEdit->value());
    }
}
