#pragma once

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
        QString mRace;
        QString mBloodline;
        QString mAncestry;
        QString mGender;
        ISKType mISK = 0;
        float mCorpStanding = 0.f;
        float mFactionStanding = 0.f;

        OrderAmountSkills mOrderAmountSkills;
        TradeRangeSkills mTradeRangeSkills;
        FeeSkills mFeeSkills;
        ContractSkills mContractSkills;
    };
}
