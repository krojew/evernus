#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QGroupBox>
#include <QLabel>
#include <QFont>

#include "Repository.h"

#include "CharacterWidget.h"

namespace Evernus
{
    CharacterWidget::CharacterWidget(const Repository<Character> &characterRepository, QWidget *parent)
        : QWidget{parent}
        , mCharacterRepository{characterRepository}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto infoGroup = new QGroupBox{tr("Character info"), this};
        mainLayout->addWidget(infoGroup);

        auto infoLayout = new QHBoxLayout{};
        infoGroup->setLayout(infoLayout);

        auto backgroundLayout = new QVBoxLayout{};
        infoLayout->addLayout(backgroundLayout);

        mNameLabel = new QLabel{this};
        backgroundLayout->addWidget(mNameLabel);

        QFont font;
        font.setBold(true);
        font.setPointSize(20);

        mNameLabel->setFont(font);

        mBackgroundLabel = new QLabel{this};
        backgroundLayout->addWidget(mBackgroundLabel);

        mCorporationLabel = new QLabel{this};
        backgroundLayout->addWidget(mCorporationLabel);

        mISKLabel = new QLabel{this};
        backgroundLayout->addWidget(mISKLabel);
    }

    void CharacterWidget::setCharacter(Character::IdType id)
    {
        qDebug() << "Switching character to" << id;

        if (id == Character::invalidId)
        {
            mNameLabel->setText(QString{});
            mBackgroundLabel->setText(QString{});
            mCorporationLabel->setText(QString{});
            mISKLabel->setText(QString{});
        }
        else
        {
            try
            {
                const auto character = mCharacterRepository.find(id);

                mNameLabel->setText(character.getName());
                mBackgroundLabel->setText(QString{"%1 %2, %3, %4"}
                    .arg(character.getGender())
                    .arg(character.getRace())
                    .arg(character.getBloodline())
                    .arg(character.getAncestry()));
                mCorporationLabel->setText(character.getCorporationName());
                mISKLabel->setText(QString::fromStdString(character.getISK().str(20)));
            }
            catch (const Repository<Character>::NotFoundException &)
            {
                QMessageBox::warning(this, tr("Character error"), tr("Character not found in DB. Refresh characters."));
            }
        }
    }
}
