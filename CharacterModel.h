#pragma once

#include <QAbstractTableModel>

#include "Character.h"

namespace Evernus
{
    template<class T>
    class Repository;

    class CharacterModel
        : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        explicit CharacterModel(const Repository<Character> &characterRepository, QObject *parent = nullptr);
        virtual ~CharacterModel() = default;

        virtual Qt::ItemFlags flags(const QModelIndex &index) const override;
        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
        virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex{}) override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

    public slots:
        void reset();

    private:
        const Repository<Character> &mCharacterRepository;

        std::vector<Character> mData;
    };
}
