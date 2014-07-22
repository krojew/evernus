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
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDateEdit>
#include <QLabel>

#include "WalletEntryFilterWidget.h"

namespace Evernus
{
    WalletEntryFilterWidget::WalletEntryFilterWidget(const QStringList &typeFilters, QWidget *parent)
        : QWidget{parent}
    {
        auto mainLayout = new QHBoxLayout{};
        setLayout(mainLayout);

        mainLayout->addWidget(new QLabel(tr("Show:"), this));

        mTypeCombo = new QComboBox{this};
        mainLayout->addWidget(mTypeCombo);

        for (auto i = 0; i < typeFilters.count(); ++i)
            mTypeCombo->addItem(typeFilters[i], i);

        connect(mTypeCombo, SIGNAL(currentIndexChanged(int)), SLOT(changeEntryType()));

        mainLayout->addWidget(new QLabel{tr("From:"), this});

        mFromEdit = new QDateEdit{this};
        mainLayout->addWidget(mFromEdit);
        mFromEdit->setCalendarPopup(true);
        connect(mFromEdit, &QDateEdit::dateChanged, this, &WalletEntryFilterWidget::fromChanged);

        mainLayout->addWidget(new QLabel{tr("To:"), this});

        mToEdit = new QDateEdit{this};
        mainLayout->addWidget(mToEdit);
        mToEdit->setCalendarPopup(true);
        connect(mToEdit, &QDateEdit::dateChanged, this, &WalletEntryFilterWidget::toChanged);

        mFilterEdit = new QLineEdit{this};
        mainLayout->addWidget(mFilterEdit);
        mFilterEdit->setPlaceholderText(tr("type in wildcard and press Enter"));
        mFilterEdit->setClearButtonEnabled(true);
        connect(mFilterEdit, &QLineEdit::returnPressed, this, &WalletEntryFilterWidget::applyWildcard);
    }

    void WalletEntryFilterWidget::setFilter(const QDate &from, const QDate &to, const QString &filter, int type)
    {
        mFromEdit->blockSignals(true);
        mToEdit->blockSignals(true);
        mTypeCombo->blockSignals(true);

        mTypeCombo->setCurrentIndex(0);
        mFromEdit->setDate(from);
        mToEdit->setDate(to);
        mFilterEdit->clear();

        mTypeCombo->blockSignals(false);
        mFromEdit->blockSignals(false);
        mToEdit->blockSignals(false);

        emit filterChanged(from, to, mFilterEdit->text(), mCurrentType);
    }

    void WalletEntryFilterWidget::changeEntryType()
    {
        mCurrentType = mTypeCombo->currentData().toInt();
        emit filterChanged(mFromEdit->date(), mToEdit->date(), mFilterEdit->text(), mCurrentType);
    }

    void WalletEntryFilterWidget::applyWildcard()
    {
        emit filterChanged(mFromEdit->date(), mToEdit->date(), mFilterEdit->text(), mCurrentType);
    }

    void WalletEntryFilterWidget::fromChanged(const QDate &date)
    {
        if (date > mToEdit->date())
            mToEdit->setDate(date.addDays(1));
        else
            emit filterChanged(date, mToEdit->date(), mFilterEdit->text(), mCurrentType);
    }

    void WalletEntryFilterWidget::toChanged(const QDate &date)
    {
        if (date < mFromEdit->date())
            mFromEdit->setDate(date.addDays(-1));
        else
            emit filterChanged(mFromEdit->date(), date, mFilterEdit->text(), mCurrentType);
    }
}
