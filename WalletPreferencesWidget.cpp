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
#include <QSpinBox>

#include "WalletSettings.h"

#include "WalletPreferencesWidget.h"

namespace Evernus
{
    WalletPreferencesWidget::WalletPreferencesWidget(QWidget *parent)
        : QWidget{parent}
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto journalGroup = new QGroupBox{tr("Journal"), this};
        mainLayout->addWidget(journalGroup);

        auto journalLayout = new QVBoxLayout{};
        journalGroup->setLayout(journalLayout);

        mDeleteOldJournalBtn = new QCheckBox{tr("Delete old entries"), this};
        journalLayout->addWidget(mDeleteOldJournalBtn);
        connect(mDeleteOldJournalBtn, &QCheckBox::stateChanged, this, &WalletPreferencesWidget::deleteOldJournalToggled);

        mOldJournalDaysEdit = new QSpinBox{this};
        journalLayout->addWidget(mOldJournalDaysEdit);
        mOldJournalDaysEdit->setMinimum(1);
        mOldJournalDaysEdit->setValue(settings.value(WalletSettings::oldJournalDaysKey, WalletSettings::oldJournalDaysDefault).toInt());

        mDeleteOldJournalBtn->setChecked(settings.value(WalletSettings::deleteOldJournalKey, true).toBool());

        mainLayout->addStretch();
    }

    void WalletPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(WalletSettings::deleteOldJournalKey, mDeleteOldJournalBtn->isChecked());
        settings.setValue(WalletSettings::oldJournalDaysKey, mOldJournalDaysEdit->value());
    }

    void WalletPreferencesWidget::deleteOldJournalToggled(int state)
    {
        mOldJournalDaysEdit->setEnabled(state == Qt::Checked);
    }
}
