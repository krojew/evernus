#pragma once

#include "CharacterBoundWidget.h"
#include "AssetModel.h"

class QTreeView;
class QLabel;

namespace Evernus
{
    class AssetListRepository;
    class EveDataProvider;
    class APIManager;
    class AssetList;

    class AssetsWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        AssetsWidget(const AssetListRepository &assetRepository,
                     const EveDataProvider &nameProvider,
                     const APIManager &apiManager,
                     QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    public slots:
        void updateData();

    private:
        QTreeView *mAssetView = nullptr;
        QLabel *mInfoLabel = nullptr;

        AssetModel mModel;

        virtual void handleNewCharacter(Character::IdType id) override;

        void setNewInfo();
    };
}
