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

        mainLayout->addStretch();
    }

    void PricePreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(PriceSettings::minMarginKey, mMinMarginEdit->value());
        settings.setValue(PriceSettings::preferredMarginKey, mPreferredMarginEdit->value());
    }
}
