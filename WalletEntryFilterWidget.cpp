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
#include <QLabel>
#include <QDate>

#include "DateRangeWidget.h"

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

        mRangeEdit = new DateRangeWidget{this};
        mainLayout->addWidget(mRangeEdit);
        connect(mRangeEdit, &DateRangeWidget::rangeChanged, this, &WalletEntryFilterWidget::applyRange);

        mFilterEdit = new QLineEdit{this};
        mainLayout->addWidget(mFilterEdit);
        mFilterEdit->setPlaceholderText(tr("type in wildcard and press Enter"));
        mFilterEdit->setClearButtonEnabled(true);
        connect(mFilterEdit, &QLineEdit::returnPressed, this, &WalletEntryFilterWidget::applyWildcard);
    }

    void WalletEntryFilterWidget::setFilter(const QDate &from, const QDate &to, const QString &filter, int type)
    {
        mTypeCombo->blockSignals(true);

        mTypeCombo->setCurrentIndex(0);
        mRangeEdit->setRange(from, to);
        mFilterEdit->clear();

        mTypeCombo->blockSignals(false);

        emit filterChanged(from, to, mFilterEdit->text(), mCurrentType);
    }

    void WalletEntryFilterWidget::changeEntryType()
    {
        mCurrentType = mTypeCombo->currentData().toInt();
        emit filterChanged(mRangeEdit->getFrom(), mRangeEdit->getTo(), mFilterEdit->text(), mCurrentType);
    }

    void WalletEntryFilterWidget::applyWildcard()
    {
        emit filterChanged(mRangeEdit->getFrom(), mRangeEdit->getTo(), mFilterEdit->text(), mCurrentType);
    }

    void WalletEntryFilterWidget::applyRange(const QDate &from, const QDate &to)
    {
        emit filterChanged(from, to, mFilterEdit->text(), mCurrentType);
    }
}
