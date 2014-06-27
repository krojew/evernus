#pragma once

#include <unordered_map>

#include <QWidget>

#include "FileDownload.h"
#include "Character.h"

class QDoubleSpinBox;
class QSpinBox;
class QLabel;

namespace Evernus
{
    template<class T>
    class Repository;
    class ButtonWithTimer;
    class APIManager;

    class CharacterWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        CharacterWidget(const Repository<Character> &characterRepository,
                        const APIManager &apiManager,
                        QWidget *parent = nullptr);
        virtual ~CharacterWidget() = default;

    signals:
        void importCharacter(Character::IdType id);

    public slots:
        void setCharacter(Character::IdType id);

        void refreshImportTimer();

    private slots:
        void setCorpStanding(double value);
        void setFactionStanding(double value);

        void setSkillLevel(int level);

        void downloadFinished();

        void requestUpdate();

    private:
        static const char * const skillFieldProperty;
        static const char * const downloadIdProperty;
        static const QString defaultPortrait;

        const Repository<Character> &mCharacterRepository;
        const APIManager &mAPIManager;

        Character::IdType mCharacterId = Character::invalidId;

        ButtonWithTimer *mImportBtn = nullptr;

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

        std::unordered_map<Character::IdType, FileDownload *> mPortraitDownloads;

        void updateStanding(const QString &type, double value) const;

        QSpinBox *createSkillEdit(QSpinBox *&target, const QString &skillField);

        static QString getPortraitPath(Character::IdType id);
    };
}
