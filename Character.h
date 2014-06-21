#pragma once

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <boost/optional.hpp>

#include <QString>

#include "Key.h"

namespace Evernus
{
    class Character
        : public Entity<quint64>
    {
    public:
        typedef boost::optional<Key::IdType> KeyIdType;
        typedef boost::multiprecision::cpp_dec_float_50 ISKType;

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

        using Entity::Entity;
        virtual ~Character() = default;

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

        KeyIdType getKeyId() const noexcept;
        void setKeyId(Key::IdType id);

        QString getCorporationName() const &;
        QString &&getCorporationName() && noexcept;
        void setCorporationName(const QString &name);
        void setCorporationName(QString &&name);

        QString getRace() const &;
        QString &&getRace() && noexcept;
        void setRace(const QString &race);
        void setRace(QString &&race);

        QString getBloodline() const &;
        QString &&getBloodline() && noexcept;
        void setBloodline(const QString &bloodline);
        void setBloodline(QString &&bloodline);

        QString getAncestry() const &;
        QString &&getAncestry() && noexcept;
        void setAncestry(const QString &ancestry);
        void setAncestry(QString &&ancestry);

        QString getGender() const &;
        QString &&getGender() && noexcept;
        void setGender(const QString &gender);
        void setGender(QString &&gender);

        uint getSkillPoints() const noexcept;
        void setSkillPoints(uint points) noexcept;

        ISKType getISK() const;
        void setISK(ISKType isk);

        OrderAmountSkills getOrderAmountSkills() const noexcept;
        void setOrderAmountSkills(const OrderAmountSkills &skills) noexcept;
        void setOrderAmountSkills(OrderAmountSkills &&skills) noexcept;

        TradeRangeSkills getTradeRangeSkills() const noexcept;
        void setTradeRangeSkills(const TradeRangeSkills &skills) noexcept;
        void setTradeRangeSkills(TradeRangeSkills &&skills) noexcept;

        FeeSkills getFeeSkills() const noexcept;
        void setFeeSkills(const FeeSkills &skills) noexcept;
        void setFeeSkills(FeeSkills &&skills) noexcept;

        ContractSkills getContractSkills() const noexcept;
        void setContractSkills(const ContractSkills &skills) noexcept;
        void setContractSkills(ContractSkills &&skills) noexcept;

        bool isEnabled() const noexcept;
        void setEnabled(bool flag) noexcept;

    private:
        QString mName;
        KeyIdType mKeyId;
        QString mCorporationName;
        QString mRace;
        QString mBloodline;
        QString mAncestry;
        QString mGender;
        uint mSkillPoints = 0;
        ISKType mISK = 0;

        OrderAmountSkills mOrderAmountSkills;
        TradeRangeSkills mTradeRangeSkills;
        FeeSkills mFeeSkills;
        ContractSkills mContractSkills;

        bool mEnabled = true;
    };
}
