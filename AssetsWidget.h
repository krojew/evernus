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
