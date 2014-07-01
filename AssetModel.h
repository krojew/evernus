#pragma once

#include <QAbstractItemModel>

namespace Evernus
{
    template<class T>
    class Repository;
    class AssetList;

    class AssetModel
        : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        explicit AssetModel(const Repository<AssetList> &assetRepository, QObject *parent = nullptr);
        virtual ~AssetModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex{}) const override;
        virtual QModelIndex parent(const QModelIndex &index) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

    private:
        const Repository<AssetList> &mAssetRepository;
    };
}
