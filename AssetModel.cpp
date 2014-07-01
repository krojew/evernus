#include "AssetModel.h"

namespace Evernus
{
    AssetModel::AssetModel(const Repository<AssetList> &assetRepository, QObject *parent)
        : QAbstractItemModel{parent}
        , mAssetRepository{assetRepository}
    {
    }

    int AssetModel::columnCount(const QModelIndex &parent) const
    {
        return 0;
    }

    QVariant AssetModel::data(const QModelIndex &index, int role) const
    {
        return QVariant{};
    }

    QModelIndex AssetModel::index(int row, int column, const QModelIndex &parent) const
    {
        return QModelIndex{};
    }

    QModelIndex AssetModel::parent(const QModelIndex &index) const
    {
        return QModelIndex{};
    }

    int AssetModel::rowCount(const QModelIndex &parent) const
    {
        return 0;
    }
}
