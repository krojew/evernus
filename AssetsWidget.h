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

#include "CharacterBoundWidget.h"
#include "ExternalOrderImporter.h"
#include "AssetModel.h"

class QItemSelection;
class QRadioButton;
class QLabel;

namespace Evernus
{
    class FilterTextRepository;
    class LeafFilterProxyModel;
    class CacheTimerProvider;
    class EveDataProvider;
    class StyledTreeView;
    class AssetProvider;
    class StationView;
    class AssetList;

    class AssetsWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        AssetsWidget(const AssetProvider &assetProvider,
                     const EveDataProvider &dataProvider,
                     const CacheTimerProvider &cacheTimerProvider,
                     const FilterTextRepository &filterRepo,
                     QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    signals:
        void importPricesFromWeb(const ExternalOrderImporter::TypeLocationPairs &target);
        void importPricesFromFile(const ExternalOrderImporter::TypeLocationPairs &target);

        void setDestinationInEve(quint64 locationId);

    public slots:
        void updateData();

    private slots:
        void prepareItemImportFromWeb();
        void prepareItemImportFromFile();

        void applyWildcard(const QString &text);

        void setCustomStation(quint64 id);

        void setDestinationForCurrent();
        void handleSelection(const QItemSelection &selected);

        void setCombine(int state);

    private:
        const AssetProvider &mAssetProvider;

        QRadioButton *mUseAssetStationBtn = nullptr;
        StyledTreeView *mAssetView = nullptr;
        StationView *mStationView = nullptr;
        QLabel *mInfoLabel = nullptr;

        AssetModel mModel;
        LeafFilterProxyModel *mModelProxy = nullptr;

        quint64 mCustomStationId = 0;

        QAction *mSetDestinationAct = nullptr;

        virtual void handleNewCharacter(Character::IdType id) override;

        void resetModel();
        void setNewInfo();

        ExternalOrderImporter::TypeLocationPairs getImportTarget() const;

        static void buildImportTarget(ExternalOrderImporter::TypeLocationPairs &target, const Item &item, quint64 locationId);
    };
}
