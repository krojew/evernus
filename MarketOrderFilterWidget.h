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
#pragma once

#include <QWidget>

#include "MarketOrderFilterProxyModel.h"

class QPushButton;
class QLineEdit;

namespace Evernus
{
    class MarketOrderFilterWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit MarketOrderFilterWidget(QWidget *parent = nullptr);
        virtual ~MarketOrderFilterWidget() = default;

    signals:
        void statusFilterChanged(const MarketOrderFilterProxyModel::StatusFilters &filter);
        void priceStatusFilterChanged(const MarketOrderFilterProxyModel::PriceStatusFilters &filter);

        void wildcardChanged(const QString &text);

    private slots:
        void setStatusFilter(const MarketOrderFilterProxyModel::StatusFilters &filter);
        void setPriceStatusFilter(const MarketOrderFilterProxyModel::PriceStatusFilters &filter);

        void applyWildcard();

    private:
        QPushButton *mStateFilterBtn = nullptr;
        QPushButton *mPriceStatusFilterBtn = nullptr;
        QLineEdit *mFilterEdit = nullptr;

        static QString getStateFilterButtonText(const MarketOrderFilterProxyModel::StatusFilters &filter);
        static QString getPriceStatusFilterButtonText(const MarketOrderFilterProxyModel::PriceStatusFilters &filter);
    };
}
