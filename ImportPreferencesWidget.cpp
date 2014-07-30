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
#include <QSettings>
#include <QSpinBox>

#include "ImportSettings.h"

#include "ImportPreferencesWidget.h"

namespace Evernus
{
    ImportPreferencesWidget::ImportPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto timersBox = new QGroupBox{tr("Data age"), this};
        mainLayout->addWidget(timersBox);

        auto timersBoxLayout = new QFormLayout{};
        timersBox->setLayout(timersBoxLayout);

        mCharacterTimer = createTimerSpin(settings.value(ImportSettings::maxCharacterAgeKey, ImportSettings::importTimerDefault).toInt());
        timersBoxLayout->addRow(tr("Max. character update age:"), mCharacterTimer);

        mAssetListTimer = createTimerSpin(settings.value(ImportSettings::maxAssetListAgeKey, ImportSettings::importTimerDefault).toInt());
        timersBoxLayout->addRow(tr("Max. asset list update age:"), mAssetListTimer);

        mWalletTimer = createTimerSpin(settings.value(ImportSettings::maxWalletAgeKey, ImportSettings::importTimerDefault).toInt());
        timersBoxLayout->addRow(tr("Max. wallet update age:"), mWalletTimer);

        mMarketOrdersTimer = createTimerSpin(settings.value(ImportSettings::maxMarketOrdersAgeKey, ImportSettings::importTimerDefault).toInt());
        timersBoxLayout->addRow(tr("Max. market orders update age:"), mMarketOrdersTimer);

        mainLayout->addStretch();
    }

    void ImportPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(ImportSettings::maxCharacterAgeKey, mCharacterTimer->value());
        settings.setValue(ImportSettings::maxAssetListAgeKey, mAssetListTimer->value());
        settings.setValue(ImportSettings::maxWalletAgeKey, mWalletTimer->value());
        settings.setValue(ImportSettings::maxMarketOrdersAgeKey, mMarketOrdersTimer->value());
    }

    QSpinBox *ImportPreferencesWidget::createTimerSpin(int value)
    {
        auto spin = new QSpinBox{this};
        spin->setRange(1, 60 * 24);
        spin->setSuffix(tr("min"));
        spin->setValue(value);

        return spin;
    }
}
