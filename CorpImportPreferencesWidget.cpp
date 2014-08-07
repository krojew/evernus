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
#include <QGroupBox>
#include <QCheckBox>
#include <QSettings>

#include "ImportSettings.h"

#include "CorpImportPreferencesWidget.h"

namespace Evernus
{
    CorpImportPreferencesWidget::CorpImportPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mUpdateCorpDataBtn = new QCheckBox{tr("Import corporation data along with character"), this};
        mainLayout->addWidget(mUpdateCorpDataBtn);
        mUpdateCorpDataBtn->setChecked(settings.value(ImportSettings::updateCorpDataKey).toBool());

        mMakeCorpSnapshotsBtn = new QCheckBox{tr("Make value snapshots form corporation data"), this};
        mainLayout->addWidget(mMakeCorpSnapshotsBtn);
        mMakeCorpSnapshotsBtn->setChecked(settings.value(ImportSettings::makeCorpSnapshotsKey).toBool());

        mainLayout->addStretch();
    }

    void CorpImportPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(ImportSettings::updateCorpDataKey, mUpdateCorpDataBtn->isChecked());
        settings.setValue(ImportSettings::makeCorpSnapshotsKey, mMakeCorpSnapshotsBtn->isChecked());
    }
}
