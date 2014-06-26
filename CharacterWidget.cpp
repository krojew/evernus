#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QGroupBox>
#include <QLabel>
#include <QFont>

#include "DatabaseUtils.h"
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

        backgroundLayout->addStretch();

        auto standingsGroup = new QGroupBox{tr("Station owner standings"), this};
        mainLayout->addWidget(standingsGroup);

        auto standingsLayout = new QHBoxLayout{};
        standingsGroup->setLayout(standingsLayout);

        standingsLayout->addWidget(new QLabel{tr("Corporation standing:"), this}, 0, Qt::AlignVCenter | Qt::AlignRight);

        mCorpStandingEdit = new QDoubleSpinBox{this};
        standingsLayout->addWidget(mCorpStandingEdit);
        mCorpStandingEdit->setSingleStep(0.01);
        connect(mCorpStandingEdit, SIGNAL(valueChanged(double)), SLOT(setCorpStanding(double)));

        standingsLayout->addWidget(new QLabel{tr("Faction standing:"), this}, 0, Qt::AlignVCenter | Qt::AlignRight);

        mFactionStandingEdit = new QDoubleSpinBox{this};
        standingsLayout->addWidget(mFactionStandingEdit);
        mFactionStandingEdit->setSingleStep(0.01);
        connect(mFactionStandingEdit, SIGNAL(valueChanged(double)), SLOT(setFactionStanding(double)));
    }

    void CharacterWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;

        qDebug() << "Switching character to" << mCharacterId;

        mCorpStandingEdit->blockSignals(true);
        mFactionStandingEdit->blockSignals(true);

        if (mCharacterId == Character::invalidId)
        {
            mNameLabel->setText(QString{});
            mBackgroundLabel->setText(QString{});
            mCorporationLabel->setText(QString{});
            mISKLabel->setText(QString{});
            mCorpStandingEdit->setValue(0.);
            mFactionStandingEdit->setValue(0.);
        }
        else
        {
            try
            {
                const auto character = mCharacterRepository.find(mCharacterId);

                mNameLabel->setText(character.getName());
                mBackgroundLabel->setText(QString{"%1 %2, %3, %4"}
                    .arg(character.getGender())
                    .arg(character.getRace())
                    .arg(character.getBloodline())
                    .arg(character.getAncestry()));
                mCorporationLabel->setText(character.getCorporationName());
                mISKLabel->setText(character.getISKPresentation());

                mCorpStandingEdit->setValue(character.getCorpStanding());
                mFactionStandingEdit->setValue(character.getFactionStanding());
            }
            catch (const Repository<Character>::NotFoundException &)
            {
                QMessageBox::warning(this, tr("Character error"), tr("Character not found in DB. Refresh characters."));
            }
        }

        mCorpStandingEdit->blockSignals(false);
        mFactionStandingEdit->blockSignals(false);
    }

    void CharacterWidget::setCorpStanding(double value)
    {
        updateStanding("corp_standing", value);
    }

    void CharacterWidget::setFactionStanding(double value)
    {
        updateStanding("faction_standing", value);
    }

    void CharacterWidget::updateStanding(const QString &type, double value) const
    {
        Q_ASSERT(mCharacterId != Character::invalidId);

        auto query = mCharacterRepository.prepare(QString{"UPDATE %1 SET %2 = :standing WHERE %3 = :id"}
            .arg(mCharacterRepository.getTableName())
            .arg(type)
            .arg(mCharacterRepository.getIdColumn()));
        query.bindValue(":standing", value);
        query.bindValue(":id", mCharacterId);
        DatabaseUtils::execQuery(query);
    }
}
