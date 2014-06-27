#pragma once

#include <QWidget>

#include "Character.h"

class QComboBox;

namespace Evernus
{
    template<class T>
    class Repository;
    class Character;

    class MenuBarWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit MenuBarWidget(const Repository<Character> &characterRepository, QWidget *parent = nullptr);
        virtual ~MenuBarWidget() = default;

    public slots:
        void refreshCharacters();

    private slots:
        void changeCharacter(int index);

    signals:
        void currentCharacterChanged(Character::IdType id);

        void importAll();

    private:
        const Repository<Character> &mCharacterRepository;

        QComboBox *mCharacterCombo = nullptr;
    };
}
