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
#include <unordered_map>

#include "APIUtils.h"

#include "WalletJournalXmlReceiver.h"

namespace Evernus
{
    namespace
    {
        QString xmlRefIdToRefType(uint id)
        {
            // this assumes ref type ids do not change over time, which seems reasonable
            static const std::unordered_map<uint, QString> mapping{
                { 1, QStringLiteral("player_trading") },
                { 2, QStringLiteral("market_transaction") },
                { 10, QStringLiteral("player_donation") },
                { 13, QStringLiteral("office_rental_fee") },
                { 17, QStringLiteral("bounty_prize_historical") },
                { 19, QStringLiteral("insurance") },
                { 7, QStringLiteral("mission_reward") },
                { 34, QStringLiteral("mission_reward_bonus") },
                { 35, QStringLiteral("cspa") },
                { 37, QStringLiteral("corp_account_withdrawal") },
                { 40, QStringLiteral("logo_change_fee") },
                { 42, QStringLiteral("market_escrow") },
                { 46, QStringLiteral("broker_fee") },
                { 50, QStringLiteral("alliance_maintenance_fee") },
                { 54, QStringLiteral("sales_tax") },
                { 55, QStringLiteral("jump_clone_installation_fee") },
                { 56, QStringLiteral("manufacturing") },
                { 63, QStringLiteral("contract") },
                { 64, QStringLiteral("contract") },
                { 65, QStringLiteral("contract") },
                { 66, QStringLiteral("contract") },
                { 67, QStringLiteral("contract") },
                { 68, QStringLiteral("contract") },
                { 69, QStringLiteral("contract") },
                { 70, QStringLiteral("contract") },
                { 71, QStringLiteral("contract") },
                { 72, QStringLiteral("contract") },
                { 73, QStringLiteral("contract") },
                { 74, QStringLiteral("contract") },
                { 75, QStringLiteral("contract") },
                { 77, QStringLiteral("contract") },
                { 78, QStringLiteral("contract") },
                { 79, QStringLiteral("contract") },
                { 80, QStringLiteral("contract") },
                { 81, QStringLiteral("contract") },
                { 82, QStringLiteral("contract") },
                { 83, QStringLiteral("contract") },
                { 84, QStringLiteral("contract") },
                { 102, QStringLiteral("contract") },
                { 16, QStringLiteral("bounty_prizes") },
                { 17, QStringLiteral("bounty_prizes") },
                { 85, QStringLiteral("bounty_prizes") },
                { 87, QStringLiteral("medal_creation_fee") },
                { 88, QStringLiteral("medal_issuing_fee") },
                { 96, QStringLiteral("customs_office_import_duty") },
                { 97, QStringLiteral("customs_office_export_duty") },
                { 99, QStringLiteral("corporate_reward_payout") },
                { 120, QStringLiteral("industry_facility_tax") },
                { 125, QStringLiteral("project_discovery_reward") },
                { 127, QStringLiteral("reprocessing_fee") },
                { 128, QStringLiteral("jump_clone_activation_fee") },
                { 8, QStringLiteral("jump_clone_activation_fee") },
            };

            const auto it = mapping.find(id);
            return (it == std::end(mapping)) ? (QStringLiteral("unknown")) : (it->second);
        }
    }

    template<>
    void APIXmlReceiver<WalletJournal::value_type>::attribute(const QXmlName &name, const QStringRef &value)
    {
        const auto localName = name.localName(mNamePool);
        const auto strValue = value.toString();

        if (localName == "date")
        {
            auto dt = QDateTime::fromString(strValue, APIUtils::eveTimeFormat);
            dt.setTimeSpec(Qt::UTC);

            mCurrentElement->setTimestamp(dt);
        }
        else if (localName == "refID")
        {
            mCurrentElement->setId(convert<WalletJournal::value_type::IdType>(strValue));
        }
        else if (localName == "refTypeID")
        {
            mCurrentElement->setRefType(xmlRefIdToRefType(convert<uint>(strValue)));
        }
        else if (localName == "ownerID1")
        {
            const auto value = convert<WalletJournal::value_type::PartyIdType::value_type>(strValue);
            if (value != 0)
                mCurrentElement->setFirstPartyId(value);
        }
        else if (localName == "ownerID2")
        {
            const auto value = convert<WalletJournal::value_type::PartyIdType::value_type>(strValue);
            if (value != 0)
                mCurrentElement->setSecondPartyId(value);
        }
        else if (localName == "argID1")
        {
            const auto value = convert<WalletJournal::value_type::ExtraInfoIdType::value_type>(strValue);
            if (value != 0)
                mCurrentElement->setExtraInfoId(value);
        }
        else if (localName == "amount")
        {
            mCurrentElement->setAmount(convert<WalletJournal::value_type::ISKType::value_type>(strValue));
        }
        else if (localName == "balance")
        {
            mCurrentElement->setBalance(convert<WalletJournal::value_type::ISKType::value_type>(strValue));
        }
        else if (localName == "reason" && !strValue.isEmpty())
        {
            mCurrentElement->setReason(strValue);
        }
        else if (localName == "taxReceiverID" && !strValue.isEmpty())
        {
            mCurrentElement->setTaxReceiverId(convert<WalletJournal::value_type::TaxReceiverType::value_type>(strValue));
        }
        else if (localName == "taxAmount" && !strValue.isEmpty())
        {
            mCurrentElement->setTaxAmount(convert<WalletJournal::value_type::ISKType::value_type>(strValue));
        }
    }
}
