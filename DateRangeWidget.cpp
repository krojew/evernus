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
#include <QDateEdit>
#include <QLabel>

#include "DateRangeWidget.h"

namespace Evernus
{
    DateRangeWidget::DateRangeWidget(QWidget *parent)
        : QWidget{parent}
    {
        auto mainLayout = new QHBoxLayout{};
        setLayout(mainLayout);
        mainLayout->setContentsMargins(QMargins{});

        mainLayout->addWidget(new QLabel{tr("From:"), this});

        mFromEdit = new QDateEdit{this};
        mainLayout->addWidget(mFromEdit);
        mFromEdit->setCalendarPopup(true);
        connect(mFromEdit, &QDateEdit::dateChanged, this, &DateRangeWidget::fromChanged);

        mainLayout->addWidget(new QLabel{tr("To:"), this});

        mToEdit = new QDateEdit{this};
        mainLayout->addWidget(mToEdit);
        mToEdit->setCalendarPopup(true);
        connect(mToEdit, &QDateEdit::dateChanged, this, &DateRangeWidget::toChanged);
    }

    QDate DateRangeWidget::getFrom() const
    {
        return mFromEdit->date();
    }

    QDate DateRangeWidget::getTo() const
    {
        return mToEdit->date();
    }

    void DateRangeWidget::setRange(const QDate &from, const QDate &to)
    {
        mFromEdit->blockSignals(true);
        mToEdit->blockSignals(true);

        mFromEdit->setDate(from);
        mToEdit->setDate(to);

        mFromEdit->blockSignals(false);
        mToEdit->blockSignals(false);
    }

    void DateRangeWidget::fromChanged(const QDate &date)
    {
        if (date > mToEdit->date())
            mToEdit->setDate(date.addDays(1));
        else
            emit rangeChanged(date, mToEdit->date());
    }

    void DateRangeWidget::toChanged(const QDate &date)
    {
        if (date < mFromEdit->date())
            mFromEdit->setDate(date.addDays(-1));
        else
            emit rangeChanged(mFromEdit->date(), date);
    }
}
