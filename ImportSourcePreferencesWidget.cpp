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
#include <QComboBox>
#include <QSettings>

#include "ImportSourcePreferencesWidget.h"

namespace Evernus
{
    ImportSourcePreferencesWidget::ImportSourcePreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto sourceGroup = new QGroupBox{tr("Default import sources"), this};
        mainLayout->addWidget(sourceGroup);

        auto sourceGroupLayout = new QFormLayout{};
        sourceGroup->setLayout(sourceGroupLayout);

        mPriceSourceCombo = new QComboBox{this};
        sourceGroupLayout->addRow(tr("Prices:"), mPriceSourceCombo);

        QSettings settings;
        const auto priceSource = static_cast<ImportSettings::PriceImportSource>(
            settings.value(ImportSettings::priceImportSourceKey, static_cast<int>(ImportSettings::priceImportSourceDefault)).toInt());

        addPriceSourceItem(tr("Web"), ImportSettings::PriceImportSource::Web, priceSource);
        addPriceSourceItem(tr("File"), ImportSettings::PriceImportSource::File, priceSource);
        addPriceSourceItem(tr("Cache"), ImportSettings::PriceImportSource::Cache, priceSource);

        mainLayout->addStretch();
    }

    void ImportSourcePreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(ImportSettings::priceImportSourceKey, mPriceSourceCombo->currentData().toInt());
    }

    void ImportSourcePreferencesWidget::addPriceSourceItem(const QString &text,
                                                           ImportSettings::PriceImportSource value,
                                                           ImportSettings::PriceImportSource current)
    {
        mPriceSourceCombo->addItem(text, static_cast<int>(value));
        if (value == current)
            mPriceSourceCombo->setCurrentIndex(mPriceSourceCombo->count() - 1);
    }
}
