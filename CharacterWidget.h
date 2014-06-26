#pragma once

#include <QWidget>

#include "Character.h"

class QDoubleSpinBox;
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

    private slots:
        void setCorpStanding(double value);
        void setFactionStanding(double value);

    private:
        const Repository<Character> &mCharacterRepository;

        Character::IdType mCharacterId = Character::invalidId;

        QLabel *mNameLabel = nullptr;
        QLabel *mBackgroundLabel = nullptr;
        QLabel *mCorporationLabel = nullptr;
        QLabel *mISKLabel = nullptr;

        QDoubleSpinBox *mCorpStandingEdit = nullptr;
        QDoubleSpinBox *mFactionStandingEdit = nullptr;

        void updateStanding(const QString &type, double value) const;
    };
}
