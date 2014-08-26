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
#include "APIUtils.h"

#include "ContractsXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<Contracts::value_type>::attribute(const QXmlName &name, const QStringRef &value)
    {
        const auto localName = name.localName(mNamePool);
        auto strValue = value.toString();

        if (localName == "contractID")
        {
            mCurrentElement->setId(convert<Contracts::value_type::IdType>(strValue));
        }
        else if (localName == "issuerID")
        {
            mCurrentElement->setIssuerId(convert<Character::IdType>(strValue));
        }
        else if (localName == "issuerCorpID")
        {
            mCurrentElement->setIssuerCorpId(convert<quint64>(strValue));
        }
        else if (localName == "assigneeID")
        {
            mCurrentElement->setAssigneeId(convert<quint64>(strValue));
        }
        else if (localName == "acceptorID")
        {
            mCurrentElement->setAcceptorId(convert<quint64>(strValue));
        }
        else if (localName == "startStationID")
        {
            mCurrentElement->setStartStationId(convert<uint>(strValue));
        }
        else if (localName == "endStationID")
        {
            mCurrentElement->setEndStationId(convert<uint>(strValue));
        }
        else if (localName == "type")
        {
            if (strValue == "ItemExchange")
                mCurrentElement->setType(Contract::Type::ItemExchange);
            else if (strValue == "Courier")
                mCurrentElement->setType(Contract::Type::Courier);
            else if (strValue == "Auction")
                mCurrentElement->setType(Contract::Type::Auction);
        }
        else if (localName == "status")
        {
            if (strValue == "Outstanding")
                mCurrentElement->setStatus(Contract::Status::Outstanding);
            else if (strValue == "Deleted")
                mCurrentElement->setStatus(Contract::Status::Deleted);
            else if (strValue == "Completed")
                mCurrentElement->setStatus(Contract::Status::Completed);
            else if (strValue == "Failed")
                mCurrentElement->setStatus(Contract::Status::Failed);
            else if (strValue == "CompletedByIssuer")
                mCurrentElement->setStatus(Contract::Status::CompletedByIssuer);
            else if (strValue == "CompletedByContractor")
                mCurrentElement->setStatus(Contract::Status::CompletedByContractor);
            else if (strValue == "Cancelled")
                mCurrentElement->setStatus(Contract::Status::Cancelled);
            else if (strValue == "Rejected")
                mCurrentElement->setStatus(Contract::Status::Rejected);
            else if (strValue == "Reversed")
                mCurrentElement->setStatus(Contract::Status::Reversed);
            else if (strValue == "InProgress")
                mCurrentElement->setStatus(Contract::Status::InProgress);
        }
        else if (localName == "title")
        {
            mCurrentElement->setTitle(std::move(strValue));
        }
        else if (localName == "forCorp")
        {
            mCurrentElement->setForCorp(strValue != "0");
        }
        else if (localName == "availability")
        {
            if (strValue == "Private")
                mCurrentElement->setAvailability(Contract::Availability::Private);
            else if (strValue == "Public")
                mCurrentElement->setAvailability(Contract::Availability::Public);
        }
        else if (localName == "dateIssued")
        {
            auto dt = QDateTime::fromString(strValue, APIUtils::eveTimeFormat);
            dt.setTimeSpec(Qt::UTC);

            mCurrentElement->setIssued(dt);
        }
        else if (localName == "dateExpired")
        {
            auto dt = QDateTime::fromString(strValue, APIUtils::eveTimeFormat);
            dt.setTimeSpec(Qt::UTC);

            mCurrentElement->setExpired(dt);
        }
        else if (localName == "dateAccepted")
        {
            auto dt = QDateTime::fromString(strValue, APIUtils::eveTimeFormat);
            dt.setTimeSpec(Qt::UTC);

            mCurrentElement->setAccepted(dt);
        }
        else if (localName == "dateCompleted")
        {
            auto dt = QDateTime::fromString(strValue, APIUtils::eveTimeFormat);
            dt.setTimeSpec(Qt::UTC);

            mCurrentElement->setCompleted(dt);
        }
        else if (localName == "numDays")
        {
            mCurrentElement->setNumDays(convert<int>(strValue));
        }
        else if (localName == "price")
        {
            mCurrentElement->setPrice(convert<double>(strValue));
        }
        else if (localName == "reward")
        {
            mCurrentElement->setReward(convert<double>(strValue));
        }
        else if (localName == "collateral")
        {
            mCurrentElement->setCollateral(convert<double>(strValue));
        }
        else if (localName == "buyout")
        {
            mCurrentElement->setBuyout(convert<double>(strValue));
        }
        else if (localName == "volume")
        {
            mCurrentElement->setVolume(convert<double>(strValue));
        }
    }
}
