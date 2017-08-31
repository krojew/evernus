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

#include "WalletJournalXmlReceiver.h"

namespace Evernus
{
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
            // TODO: convert to ESI
            //mCurrentElement->setRefTypeId(convert<uint>(strValue));
        }
        else if (localName == "ownerName1")
        {
            mCurrentElement->setOwnerName1(strValue);
        }
        else if (localName == "ownerID1")
        {
            mCurrentElement->setOwnerId1(convert<uint>(strValue));
        }
        else if (localName == "ownerName2")
        {
            mCurrentElement->setOwnerName2(strValue);
        }
        else if (localName == "ownerID2")
        {
            mCurrentElement->setOwnerId2(convert<uint>(strValue));
        }
        else if (localName == "argID1")
        {
            mCurrentElement->setExtraInfoId(convert<quint64>(strValue));
        }
        else if (localName == "amount")
        {
            mCurrentElement->setAmount(convert<double>(strValue));
        }
        else if (localName == "balance")
        {
            mCurrentElement->setBalance(convert<double>(strValue));
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
            mCurrentElement->setTaxAmount(convert<WalletJournal::value_type::TaxAmountType::value_type>(strValue));
        }
    }
}
