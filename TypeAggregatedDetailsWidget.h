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

#include <map>

#include <QWidget>
#include <QDate>

#include "MarketHistoryEntry.h"

class QCPFinancial;
class QCustomPlot;
class QDateEdit;
class QSpinBox;
class QCPGraph;
class QCPBars;

namespace Evernus
{
    class TypeAggregatedDetailsWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        using History = std::map<QDate, MarketHistoryEntry>;

        explicit TypeAggregatedDetailsWidget(History history, QWidget *parent = nullptr, Qt::WindowFlags flags = 0);
        virtual ~TypeAggregatedDetailsWidget() = default;

    public slots:
        void handleNewPreferences();

    private slots:
        void applyFilter();

    private:
        History mHistory;

        QDateEdit *mFromEdit = nullptr;
        QDateEdit *mToEdit = nullptr;
        QSpinBox *mSMADaysEdit = nullptr;
        QCustomPlot *mHistoryPlot = nullptr;

        QCPFinancial *mHistoryValuesGraph = nullptr;
        QCPBars *mHistoryVolumeGraph = nullptr;
        QCPGraph *mSMAGraph = nullptr;
    };
}
