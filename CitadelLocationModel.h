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
#include <vector>
#include <memory>

#include <QAbstractItemModel>

namespace Evernus
{
    class EveDataProvider;

    class CitadelLocationModel
        : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        explicit CitadelLocationModel(const EveDataProvider &dataProvider, QObject *parent = nullptr);
        CitadelLocationModel(const CitadelLocationModel &) = default;
        CitadelLocationModel(CitadelLocationModel &&) = default;
        virtual ~CitadelLocationModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex{}) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

        void refresh();

        CitadelLocationModel &operator =(const CitadelLocationModel &) = default;
        CitadelLocationModel &operator =(CitadelLocationModel &&) = default;

    private:
        struct LocationNode
        {
            enum class Type
            {
                Region,
                Constellation,
                SolarSystem,
                Citadel
            };

            quint64 mId = 0;
            LocationNode *mParent = nullptr;
            size_t mRow = 0;
            QString mName;
            Type mType = Type::Region;
            bool mSelected = false;

            LocationNode(quint64 id, LocationNode *parent, size_t row, const QString &name, Type type);
        };

        using LocationCache = std::unordered_map<uint, LocationNode *>;
        using LocationList = std::vector<std::unique_ptr<LocationNode>>;

        const EveDataProvider &mDataProvider;

        mutable LocationList mRegions;
        mutable std::unordered_map<uint, LocationList> mConstellations, mSolarSystems, mCitadels;
        LocationCache mSolarSystemMap;

        void fillStaticData();

        Qt::CheckState getNodeCheckState(const LocationNode &node) const noexcept;
        Qt::CheckState getNodeCheckState(const LocationList &children) const noexcept;
        Qt::CheckState getRegionNodeCheckState(const LocationNode &node) const noexcept;
        Qt::CheckState getConstellationNodeCheckState(const LocationNode &node) const noexcept;
        Qt::CheckState getSolarSystemNodeCheckState(const LocationNode &node) const noexcept;
        Qt::CheckState getCitadelNodeCheckState(const LocationNode &node) const noexcept;

        void setCheckState(const QModelIndex &index, bool checked);
        void setCheckState(const QModelIndex &parent, const LocationList &children, bool checked);
    };
}
