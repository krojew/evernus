/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <unordered_map>

#include "CharacterBoundWidget.h"
#include "FileDownload.h"

class QDoubleSpinBox;
class QSpinBox;
class QLabel;

namespace Evernus
{
    template<class T>
    class Repository;
    class CacheTimerProvider;

    class CharacterWidget
        : public CharacterBoundWidget
    {
        Q_OBJECT

    public:
        CharacterWidget(const Repository<Character> &characterRepository,
                        const CacheTimerProvider &cacheTimerProvider,
                        QWidget *parent = nullptr);
        virtual ~CharacterWidget() = default;

    public slots:
        void updateData();

    private slots:
        void setCorpStanding(double value);
        void setFactionStanding(double value);

        void setSkillLevel(int level);

        void downloadFinished();

    private:
        static const char * const skillFieldProperty;
        static const char * const downloadIdProperty;
        static const QString defaultPortrait;

        const Repository<Character> &mCharacterRepository;

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

        virtual void handleNewCharacter(Character::IdType id) override;

        void updateStanding(const QString &type, double value) const;

        QSpinBox *createSkillEdit(QSpinBox *&target, const QString &skillField);

        static QString getPortraitPath(Character::IdType id);
    };
}
