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

#include <unordered_map>

#include "ReprocessingArbitrageModel.h"
#include "ModelWithTypes.h"
#include "EveType.h"

namespace Evernus
{
    class EveDataProvider;

    class OreReprocessingArbitrageModel
        : public ReprocessingArbitrageModel
        , public ModelWithTypes
    {
        Q_OBJECT

    public:
        using RegionList = std::unordered_set<uint>;

        explicit OreReprocessingArbitrageModel(const EveDataProvider &dataProvider,
                                               QObject *parent = nullptr);
        OreReprocessingArbitrageModel(const OreReprocessingArbitrageModel &) = default;
        OreReprocessingArbitrageModel(OreReprocessingArbitrageModel &&) = default;
        virtual ~OreReprocessingArbitrageModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

        virtual EveType::IdType getTypeId(const QModelIndex &index) const override;

        virtual void setCharacter(std::shared_ptr<Character> character) override;
        virtual void setOrderData(const std::vector<ExternalOrder> &orders,
                                  PriceType dstPriceType,
                                  const RegionList &srcRegions,
                                  const RegionList &dstRegions,
                                  quint64 srcStation,
                                  quint64 dstStation,
                                  bool useStationTax,
                                  bool ignoreMinVolume,
                                  double baseYield,
                                  double sellVolumeLimit) override;

        virtual void reset() override;

        OreReprocessingArbitrageModel &operator =(const OreReprocessingArbitrageModel &) = default;
        OreReprocessingArbitrageModel &operator =(OreReprocessingArbitrageModel &&) = default;

    private:
        enum
        {
            nameColumn,
            volumeColumn,
            totalProfitColumn,
            totalCostColumn,
            differenceColumn,
            marginColumn,

            numColumns
        };

        struct ItemData
        {
            EveType::IdType mId = EveType::invalidId;
            double mTotalProfit = 0.;
            double mTotalCost = 0.;
            double mMargin = 0.;
            quint64 mVolume = 0;
        };

        const EveDataProvider &mDataProvider;

        std::shared_ptr<Character> mCharacter;

        std::unordered_map<uint, int CharacterData::ReprocessingSkills::*> mReprocessingSkillMap;
        std::vector<ItemData> mData;

        void insertSkillMapping(const QString &groupName, int CharacterData::ReprocessingSkills::* skill);
    };
}
