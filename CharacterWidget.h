#pragma once

#include <QWidget>

#include "Character.h"

class QDoubleSpinBox;
class QSpinBox;
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

        void setSkillLevel(int level);

    private:
        static const char * const skillFieldProperty;

        const Repository<Character> &mCharacterRepository;

        Character::IdType mCharacterId = Character::invalidId;

        QLabel *mPortrait = nullptr;

        QLabel *mNameLabel = nullptr;
        QLabel *mBackgroundLabel = nullptr;
        QLabel *mCorporationLabel = nullptr;
        QLabel *mISKLabel = nullptr;

        QDoubleSpinBox *mCorpStandingEdit = nullptr;
        QDoubleSpinBox *mFactionStandingEdit = nullptr;

        QSpinBox *mTradeSkillEdit = nullptr;
        QSpinBox *mRetailSkillEdit = nullptr;
        QSpinBox *mWholesaleSkillEdit = nullptr;
        QSpinBox *mTycoonSkillEdit = nullptr;
        QSpinBox *mMarketingSkillEdit = nullptr;
        QSpinBox *mProcurementSkillEdit = nullptr;
        QSpinBox *mDaytradingSkillEdit = nullptr;
        QSpinBox *mVisibilitySkillEdit = nullptr;
        QSpinBox *mAccountingSkillEdit = nullptr;
        QSpinBox *mBrokerRelationsSkillEdit = nullptr;
        QSpinBox *mMarginTradingSkillEdit = nullptr;
        QSpinBox *mContractingSkillEdit = nullptr;
        QSpinBox *mCorporationContractingSkillEdit = nullptr;

        void updateStanding(const QString &type, double value) const;

        QSpinBox *createSkillEdit(QSpinBox *&target, const QString &skillField);
    };
}
