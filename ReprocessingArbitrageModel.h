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

#include <unordered_set>
#include <memory>
#include <vector>

#include <boost/optional.hpp>

#include <QAbstractTableModel>

#include "Character.h"
#include "PriceType.h"

namespace Evernus
{
    class ExternalOrder;

    class ReprocessingArbitrageModel
        : public QAbstractTableModel
    {
    public:
        using RegionList = std::unordered_set<uint>;

        using QAbstractTableModel::QAbstractTableModel;

        ReprocessingArbitrageModel() = default;
        ReprocessingArbitrageModel(const ReprocessingArbitrageModel &) = default;
        ReprocessingArbitrageModel(ReprocessingArbitrageModel &&) = default;
        virtual ~ReprocessingArbitrageModel() = default;

        virtual void setCharacter(std::shared_ptr<Character> character) = 0;
        virtual void setOrderData(const std::vector<ExternalOrder> &orders,
                                  PriceType dstPriceType,
                                  const RegionList &srcRegions,
                                  const RegionList &dstRegions,
                                  quint64 srcStation,
                                  quint64 dstStation,
                                  bool useStationTax,
                                  bool ignoreMinVolume,
                                  bool onlyHighSec,
                                  double baseYield,
                                  double sellVolumeLimit,
                                  const boost::optional<double> &customStationTax) = 0;

        virtual void reset() = 0;

        ReprocessingArbitrageModel &operator =(const ReprocessingArbitrageModel &) = default;
        ReprocessingArbitrageModel &operator =(ReprocessingArbitrageModel &&) = default;
    };
}
