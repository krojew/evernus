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
#include <QString>
#include <QDate>

#include "MarketHistoryEntry.h"

class QCPFinancial;
class QCustomPlot;
class QCPItemLine;
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
        void addTrendLine();

    protected:
        virtual void resizeEvent(QResizeEvent *event) override;

    private:
        static const QString settingsSizeKey;

        History mHistory;

        QDateEdit *mFromEdit = nullptr;
        QDateEdit *mToEdit = nullptr;
        QSpinBox *mSMADaysEdit = nullptr;
        QSpinBox *mMACDFastDaysEdit = nullptr;
        QSpinBox *mMACDSlowDaysEdit = nullptr;
        QSpinBox *mMACDEMADaysEdit = nullptr;
        QCustomPlot *mHistoryPlot = nullptr;

        QCPFinancial *mHistoryValuesGraph = nullptr;
        QCPBars *mHistoryVolumeGraph = nullptr;
        QCPBars *mHistoryVolumeFlagGraph = nullptr;
        QCPGraph *mSMAGraph = nullptr;
        QCPGraph *mRSIGraph = nullptr;
        QCPGraph *mMACDGraph = nullptr;
        QCPGraph *mMACDEMAGraph = nullptr;
        QCPBars *mMACDDivergenceGraph = nullptr;
        QCPGraph *mBollingerUpperGraph = nullptr;
        QCPGraph *mBollingerLowerGraph = nullptr;

        QCPItemLine *mTrendLine = nullptr;

        void deleteTrendLine() noexcept;
        void applyGraphFormats();
    };
}
