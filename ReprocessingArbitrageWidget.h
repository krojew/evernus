/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for m details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <memory>

#include <QSortFilterProxyModel>

#include "ReprocessingArbitrageModel.h"
#include "StandardModelProxyWidget.h"
#include "PriceType.h"

class QStackedWidget;
class QDoubleSpinBox;
class QCheckBox;
class QSpinBox;
class QAction;

namespace Evernus
{
    class ReprocessingArbitrageModel;
    class AdjustableTableView;
    class MarketDataProvider;
    class EveDataProvider;
    class RegionComboBox;
    class Character;

    class ReprocessingArbitrageWidget
        : public StandardModelProxyWidget
    {
        Q_OBJECT

    public:
        explicit ReprocessingArbitrageWidget(const EveDataProvider &dataProvider,
                                             const MarketDataProvider &marketDataProvider,
                                             const QString &viewConfigName,
                                             QWidget *parent = nullptr);
        ReprocessingArbitrageWidget(const ReprocessingArbitrageWidget &) = default;
        ReprocessingArbitrageWidget(ReprocessingArbitrageWidget &&) = default;
        virtual ~ReprocessingArbitrageWidget() = default;

        void setCharacter(std::shared_ptr<Character> character);
        void clearData();

        void setPriceType(PriceType dst) noexcept;

        ReprocessingArbitrageWidget &operator =(const ReprocessingArbitrageWidget &) = default;
        ReprocessingArbitrageWidget &operator =(ReprocessingArbitrageWidget &&) = default;

    public slots:
        void recalculateData();

    protected:
        void setSourceModel(ReprocessingArbitrageModel *model);

    private:
        static const auto waitingLabelIndex = 0;

        const MarketDataProvider &mMarketDataProvider;

        RegionComboBox *mSourceRegionCombo = nullptr;
        RegionComboBox *mDestRegionCombo = nullptr;
        QDoubleSpinBox *mStationEfficiencyEdit = nullptr;
        QSpinBox *mSellVolumeLimitEdit = nullptr;
        QCheckBox *mIncludeStationTaxBtn = nullptr;
        QCheckBox *mIgnoreMinVolumeBtn = nullptr;
        QStackedWidget *mDataStack = nullptr;
        AdjustableTableView *mDataView = nullptr;

        QSortFilterProxyModel mDataProxy;
        ReprocessingArbitrageModel *mDataModel = nullptr;

        PriceType mDstPriceType = PriceType::Buy;

        quint64 mSrcStation = 0;
        quint64 mDstStation = 0;

        void changeStation(quint64 &destination, const QVariantList &path, const QString &settingName);
    };
}
