#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>

#include "Repository.h"
#include "Character.h"

#include "MenuBarWidget.h"

namespace Evernus
{
    MenuBarWidget::MenuBarWidget(const Repository<Character> &characterRepository, QWidget *parent)
        : QWidget{parent}
        , mCharacterRepository{characterRepository}
    {
        auto mainLayout = new QHBoxLayout{};
        setLayout(mainLayout);

        mainLayout->addWidget(new QLabel{tr("Character:"), this});

        mCharacterCombo = new QComboBox{this};
        mainLayout->addWidget(mCharacterCombo);
        mCharacterCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        connect(mCharacterCombo, SIGNAL(currentIndexChanged(int)), SLOT(changeCharacter(int)), Qt::QueuedConnection);

        refreshCharacters();
    }

    void MenuBarWidget::refreshCharacters()
    {
        mCharacterCombo->clear();

        auto characters = mCharacterRepository.exec(
            QString{"SELECT id, name FROM %1 WHERE enabled != 0 AND key_id IS NOT NULL"}.arg(mCharacterRepository.getTableName()));
        while (characters.next())
            mCharacterCombo->addItem(characters.value("name").toString(), characters.value("id").value<Character::IdType>());
    }

    void MenuBarWidget::changeCharacter(int index)
    {
        Q_UNUSED(index);

        emit currentCharacterChanged(mCharacterCombo->currentData().value<Character::IdType>());
    }
}
