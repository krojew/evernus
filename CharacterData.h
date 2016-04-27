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

#include <boost/optional.hpp>

#include <QString>

namespace Evernus
{
    struct CharacterData
    {
        typedef double ISKType;

        struct OrderAmountSkills
        {
            int mTrade = 0;
            int mRetail = 0;
            int mWholesale = 0;
            int mTycoon = 0;
        };

        struct TradeRangeSkills
        {
            int mMarketing = 0;
            int mProcurement = 0;
            int mDaytrading = 0;
            int mVisibility = 0;
        };

        struct FeeSkills
        {
            int mAccounting = 0;
            int mBrokerRelations = 0;
            int mMarginTrading = 0;
        };

        struct ContractSkills
        {
            int mContracting = 0;
            int mCorporationContracting = 0;
        };

        QString mName;
        QString mCorporationName;
        quint64 mCorporationId = 0;
        QString mRace;
        QString mBloodline;
        QString mAncestry;
        QString mGender;
        ISKType mISK = 0;
        float mCorpStanding = 0.f;
        float mFactionStanding = 0.f;
        boost::optional<double> mBrokersFee;

        OrderAmountSkills mOrderAmountSkills;
        TradeRangeSkills mTradeRangeSkills;
        FeeSkills mFeeSkills;
        ContractSkills mContractSkills;
    };
}
