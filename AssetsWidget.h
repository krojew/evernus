#pragma once

#include "CharacterBoundWidget.h"
#include "AssetModel.h"

namespace Evernus
{
    class AssetListRepository;
    class NameProvider;
    class APIManager;
    class AssetList;

    class AssetsWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        AssetsWidget(const AssetListRepository &assetRepository,
                     const NameProvider &nameProvider,
                     const APIManager &apiManager,
                     QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    public slots:
        void updateData();

    private:
        AssetModel mModel;

        virtual void handleNewCharacter(Character::IdType id) override;
    };
}
