#include "CharacterModel.h"

namespace Evernus
{
    CharacterModel::CharacterModel(const Repository<Character> &characterRepository, QObject *parent)
        : QAbstractTableModel{parent}
        , mCharacterRepository{characterRepository}
    {
    }

    int CharacterModel::columnCount(const QModelIndex &parent) const
    {
        return 2;
    }

    QVariant CharacterModel::data(const QModelIndex &index, int role) const
    {
        return QVariant{};
    }

    int CharacterModel::rowCount(const QModelIndex &parent) const
    {
        return 0;
    }
}
