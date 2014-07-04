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

class QTreeView;
class QLabel;

namespace Evernus
{
    class EveDataProvider;
    class AssetProvider;
    class APIManager;
    class AssetList;

    class AssetsWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        AssetsWidget(const AssetProvider &assetProvider,
                     const EveDataProvider &nameProvider,
                     const APIManager &apiManager,
                     QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    signals:
        void importPricesFromWeb(const ItemPriceImporter::TypeLocationPairs &target);

    public slots:
        void updateData();

    private slots:
        void prepareItemImportFromWeb();

    private:
        const AssetProvider &mAssetProvider;

        QTreeView *mAssetView = nullptr;
        QLabel *mInfoLabel = nullptr;

        AssetModel mModel;

        virtual void handleNewCharacter(Character::IdType id) override;

        void setNewInfo();

        static void buildImportTarget(ItemPriceImporter::TypeLocationPairs &target, const Item &item, quint64 locationId);
    };
}
