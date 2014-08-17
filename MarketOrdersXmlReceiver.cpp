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

#include "MarketOrdersXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<MarketOrders::value_type>::endElement()
    {
        mCurrentElement->setFirstSeen(mCurrentElement->getIssued());
        mContainer.emplace_back(std::move(*mCurrentElement));
    }

    template<>
    void APIXmlReceiver<MarketOrders::value_type>::attribute(const QXmlName &name, const QStringRef &value)
    {
        const auto localName = name.localName(mNamePool);
        auto strValue = value.toString();

        if (localName == "orderID")
        {
            mCurrentElement->setId(convert<MarketOrders::value_type::IdType>(strValue));
        }
        else if (localName == "charID")
        {
            mCurrentElement->setCharacterId(convert<Character::IdType>(strValue));
        }
        else if (localName == "stationID")
        {
            mCurrentElement->setStationId(convert<quint64>(strValue));
        }
        else if (localName == "volEntered")
        {
            mCurrentElement->setVolumeEntered(convert<uint>(strValue));
        }
        else if (localName == "volRemaining")
        {
            mCurrentElement->setVolumeRemaining(convert<uint>(strValue));
        }
        else if (localName == "minVolume")
        {
            mCurrentElement->setMinVolume(convert<uint>(strValue));
        }
        else if (localName == "orderState")
        {
            mCurrentElement->setState(static_cast<MarketOrders::value_type::State>(convert<int>(strValue)));
        }
        else if (localName == "typeID")
        {
            mCurrentElement->setTypeId(convert<EveType::IdType>(strValue));
        }
        else if (localName == "range")
        {
            mCurrentElement->setRange(convert<short>(strValue));
        }
        else if (localName == "accountKey")
        {
            mCurrentElement->setAccountKey(convert<short>(strValue));
        }
        else if (localName == "duration")
        {
            mCurrentElement->setDuration(convert<short>(strValue));
        }
        else if (localName == "escrow")
        {
            mCurrentElement->setEscrow(convert<double>(strValue));
        }
        else if (localName == "price")
        {
            mCurrentElement->setPrice(convert<double>(strValue));
        }
        else if (localName == "bid")
        {
            mCurrentElement->setType((strValue == "0") ? (MarketOrders::value_type::Type::Sell) : (MarketOrders::value_type::Type::Buy));
        }
        else if (localName == "issued")
        {
            auto dt = QDateTime::fromString(strValue, APIUtils::eveTimeFormat);
            dt.setTimeSpec(Qt::UTC);

            mCurrentElement->setIssued(dt);
        }
    }
}
