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
