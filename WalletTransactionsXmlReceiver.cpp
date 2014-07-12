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

#include "WalletTransactionsXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<WalletTransactions::value_type>::attribute(const QXmlName &name, const QStringRef &value)
    {
        const auto localName = name.localName(mNamePool);
        auto strValue = value.toString();

        if (localName == "transactionDateTime")
        {
            auto dt = QDateTime::fromString(strValue, APIUtils::eveTimeFormat);
            dt.setTimeSpec(Qt::UTC);

            mCurrentElement->setTimestamp(dt);
        }
        else if (localName == "transactionID")
        {
            mCurrentElement->setId(convert<WalletTransactions::value_type::IdType>(strValue));
        }
        else if (localName == "quantity")
        {
            mCurrentElement->setQuantity(convert<uint>(strValue));
        }
        else if (localName == "typeID")
        {
            mCurrentElement->setTypeId(convert<EveType::IdType>(strValue));
        }
        else if (localName == "price")
        {
            mCurrentElement->setPrice(convert<double>(strValue));
        }
        else if (localName == "clientID")
        {
            mCurrentElement->setClientId(convert<quint64>(strValue));
        }
        else if (localName == "clientName")
        {
            mCurrentElement->setClientName(std::move(strValue));
        }
        else if (localName == "stationID")
        {
            mCurrentElement->setLocationId(convert<quint64>(strValue));
        }
        else if (localName == "transactionType")
        {
            mCurrentElement->setType((strValue == "buy") ? (WalletTransaction::Type::Buy) : (WalletTransaction::Type::Sell));
        }
        else if (localName == "journalTransactionID")
        {
            mCurrentElement->setJournalId(convert<WalletJournalEntry::IdType>(strValue));
        }
    }
}
