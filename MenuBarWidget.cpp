#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>

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
        mainLayout->setContentsMargins(QMargins{});

        auto importBtn = new QPushButton{QIcon{":/images/arrow_refresh.png"}, tr("Import all"), this};
        mainLayout->addWidget(importBtn);
        importBtn->setFlat(true);
        connect(importBtn, &QPushButton::clicked, this, &MenuBarWidget::importAll);

        mCharacterCombo = new QComboBox{this};
        mainLayout->addWidget(mCharacterCombo);
        mCharacterCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        connect(mCharacterCombo, SIGNAL(currentIndexChanged(int)), SLOT(changeCharacter(int)), Qt::QueuedConnection);

        refreshCharacters();
    }

    void MenuBarWidget::refreshCharacters()
    {
        const auto index = mCharacterCombo->currentIndex();
        const auto prevId = (index == -1) ? (Character::invalidId) : (mCharacterCombo->currentData().value<Character::IdType>());
        auto emitPrevious = false;

        mCharacterCombo->blockSignals(true);
        mCharacterCombo->clear();

        auto characters = mCharacterRepository.exec(
            QString{"SELECT id, name FROM %1 WHERE enabled != 0 AND key_id IS NOT NULL"}.arg(mCharacterRepository.getTableName()));

        while (characters.next())
        {
            const auto id = characters.value("id").value<Character::IdType>();

            mCharacterCombo->addItem(characters.value("name").toString(), id);
            if (id == prevId)
            {
                emitPrevious = true;
                mCharacterCombo->setCurrentIndex(mCharacterCombo->count() - 1);
            }
        }

        mCharacterCombo->model()->sort(0);

        mCharacterCombo->blockSignals(false);

        if (!emitPrevious)
            QMetaObject::invokeMethod(this, "changeCharacter", Qt::QueuedConnection, Q_ARG(int, mCharacterCombo->currentIndex()));
    }

    void MenuBarWidget::changeCharacter(int index)
    {
        const auto id = (index == -1) ? (Character::invalidId) : (mCharacterCombo->currentData().value<Character::IdType>());
        emit currentCharacterChanged(id);
    }
}
