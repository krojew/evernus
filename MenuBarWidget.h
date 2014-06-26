#pragma once

#include <QWidget>

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

    private:
        const Repository<Character> &mCharacterRepository;

        QComboBox *mCharacterCombo = nullptr;
    };
}
