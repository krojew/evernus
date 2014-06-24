#pragma once

#include <boost/optional.hpp>

#include <QString>

#include "CharacterData.h"
#include "Key.h"

namespace Evernus
{
    class Character
        : public Entity<quint64>
    {
    public:
        typedef boost::optional<Key::IdType> KeyIdType;

        using Entity::Entity;
        virtual ~Character() = default;

        KeyIdType getKeyId() const noexcept;
        void setKeyId(KeyIdType id);

        QString getName() const &;
        QString &&getName() && noexcept;
        void setName(const QString &name);
        void setName(QString &&name);

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

        CharacterData::ISKType getISK() const;
        void setISK(CharacterData::ISKType isk);

        float getCorpStanding() const noexcept;
        void setCorpStanding(float standing) noexcept;

        float getFactionStanding() const noexcept;
        void setFactionStanding(float standing) noexcept;

        CharacterData::OrderAmountSkills getOrderAmountSkills() const noexcept;
        void setOrderAmountSkills(const CharacterData::OrderAmountSkills &skills) noexcept;
        void setOrderAmountSkills(CharacterData::OrderAmountSkills &&skills) noexcept;

        CharacterData::TradeRangeSkills getTradeRangeSkills() const noexcept;
        void setTradeRangeSkills(const CharacterData::TradeRangeSkills &skills) noexcept;
        void setTradeRangeSkills(CharacterData::TradeRangeSkills &&skills) noexcept;

        CharacterData::FeeSkills getFeeSkills() const noexcept;
        void setFeeSkills(const CharacterData::FeeSkills &skills) noexcept;
        void setFeeSkills(CharacterData::FeeSkills &&skills) noexcept;

        CharacterData::ContractSkills getContractSkills() const noexcept;
        void setContractSkills(const CharacterData::ContractSkills &skills) noexcept;
        void setContractSkills(CharacterData::ContractSkills &&skills) noexcept;

        CharacterData getCharacterData() const &;
        CharacterData &&getCharacterData() && noexcept;
        void setCharacterData(const CharacterData &data);
        void setCharacterData(CharacterData &&data);

        bool isEnabled() const noexcept;
        void setEnabled(bool flag) noexcept;

    private:
        CharacterData mData;
        KeyIdType mKeyId;

        bool mEnabled = true;
    };
}
