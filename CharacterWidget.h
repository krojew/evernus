#pragma once

#include <QWidget>

#include "Character.h"

class QLabel;

namespace Evernus
{
    template<class T>
    class Repository;

    class CharacterWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit CharacterWidget(const Repository<Character> &characterRepository, QWidget *parent = nullptr);
        virtual ~CharacterWidget() = default;

    public slots:
        void setCharacter(Character::IdType id);

    private:
        const Repository<Character> &mCharacterRepository;

        QLabel *mNameLabel = nullptr;
        QLabel *mBackgroundLabel = nullptr;
        QLabel *mCorporationLabel = nullptr;
        QLabel *mISKLabel = nullptr;
    };
}
