#pragma once

#include <QAbstractTableModel>

namespace Evernus
{
    template<class T>
    class Repository;
    class Character;

    class CharacterModel
        : public QAbstractTableModel
    {
    public:
        explicit CharacterModel(const Repository<Character> &characterRepository, QObject *parent = nullptr);
        virtual ~CharacterModel() = default;

        virtual int columnCount(const QModelIndex &parent = QModelIndex{}) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual int rowCount(const QModelIndex &parent = QModelIndex{}) const override;

    private:
        const Repository<Character> &mCharacterRepository;
    };
}
