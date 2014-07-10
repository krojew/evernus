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
#include <QFormLayout>
#include <QSettings>
#include <QGroupBox>
#include <QCheckBox>
#include <QSpinBox>
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

        auto marginLayout = new QVBoxLayout{};
        marginGroup->setLayout(marginLayout);

        auto marginValuesLayout = new QHBoxLayout{};
        marginLayout->addLayout(marginValuesLayout);

        marginValuesLayout->addWidget(new QLabel{tr("Minimum:"), this}, 0, Qt::AlignRight);

        mMinMarginEdit = new QDoubleSpinBox{this};
        marginValuesLayout->addWidget(mMinMarginEdit);
        mMinMarginEdit->setSuffix("%");
        mMinMarginEdit->setValue(settings.value(PriceSettings::minMarginKey, PriceSettings::minMarginDefault).toDouble());

        marginValuesLayout->addWidget(new QLabel{tr("Preferred:"), this}, 0, Qt::AlignRight);

        mPreferredMarginEdit = new QDoubleSpinBox{this};
        marginValuesLayout->addWidget(mPreferredMarginEdit);
        mPreferredMarginEdit->setSuffix("%");
        mPreferredMarginEdit->setValue(settings.value(PriceSettings::preferredMarginKey, PriceSettings::preferredMarginDefault).toDouble());

#ifdef Q_OS_WIN
        mAltImportBtn = new QCheckBox{tr("Use alternative margin import method*"), this};
        marginLayout->addWidget(mAltImportBtn);
        mAltImportBtn->setChecked(settings.value(PriceSettings::priceAltImportKey, true).toBool());

        auto infoLabel = new QLabel{tr("* Gives faster results, but can sometimes be incorrect. If the price fluctuates after a few imports, turn it off."), this};
        infoLabel->setWordWrap(true);
        marginLayout->addWidget(infoLabel);
#endif

        auto pricesGroup = new QGroupBox{this};
        mainLayout->addWidget(pricesGroup);

        auto pricesLayout = new QFormLayout{};
        pricesGroup->setLayout(pricesLayout);

        mPriceDeltaEdit = new QDoubleSpinBox{this};
        pricesLayout->addRow(tr("Price delta:"), mPriceDeltaEdit);
        mPriceDeltaEdit->setSingleStep(0.01);
        mPriceDeltaEdit->setMinimum(0.01);
        mPriceDeltaEdit->setMaximum(100000000.);
        mPriceDeltaEdit->setValue(settings.value(PriceSettings::priceDeltaKey, PriceSettings::priceDeltaDefault).toDouble());

        mPriceMaxAgeEdit = new QSpinBox{this};
        pricesLayout->addRow(tr("Max price age:"), mPriceMaxAgeEdit);
        mPriceMaxAgeEdit->setMinimum(1);
        mPriceMaxAgeEdit->setMaximum(24 * 30);
        mPriceMaxAgeEdit->setSuffix("h");
        mPriceMaxAgeEdit->setValue(settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toUInt());

        mainLayout->addStretch();
    }

    void PricePreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(PriceSettings::minMarginKey, mMinMarginEdit->value());
        settings.setValue(PriceSettings::preferredMarginKey, mPreferredMarginEdit->value());
        settings.setValue(PriceSettings::priceDeltaKey, mPriceDeltaEdit->value());
#ifdef Q_OS_WIN
        settings.setValue(PriceSettings::priceAltImportKey, mAltImportBtn->isChecked());
#endif
        settings.setValue(PriceSettings::priceMaxAgeKey, mPriceMaxAgeEdit->value());
    }
}
