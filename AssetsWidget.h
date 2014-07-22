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
#include "ItemPriceImporter.h"
#include "AssetModel.h"

class QLineEdit;
class QLabel;

namespace Evernus
{
    class LeafFilterProxyModel;
    class CacheTimerProvider;
    class EveDataProvider;
    class StyledTreeView;
    class AssetProvider;
    class AssetList;

    class AssetsWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        AssetsWidget(const AssetProvider &assetProvider,
                     const EveDataProvider &nameProvider,
                     const CacheTimerProvider &cacheTimerProvider,
                     QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    signals:
        void importPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &target);
        void importPricesFromFile(const ItemPriceImporter::TypeLocationPairs &target);

    public slots:
        void updateData();

    private slots:
        void prepareItemImportFromWeb();
        void prepareItemImportFromFile();

        void applyWildcard();

    private:
        const AssetProvider &mAssetProvider;

        QLineEdit *mFilterEdit = nullptr;
        StyledTreeView *mAssetView = nullptr;
        QLabel *mInfoLabel = nullptr;

        AssetModel mModel;
        LeafFilterProxyModel *mModelProxy = nullptr;

        virtual void handleNewCharacter(Character::IdType id) override;

        void setNewInfo();

        ItemPriceImporter::TypeLocationPairs getImportTarget() const;

        static void buildImportTarget(ItemPriceImporter::TypeLocationPairs &target, const Item &item, quint64 locationId);
    };
}
