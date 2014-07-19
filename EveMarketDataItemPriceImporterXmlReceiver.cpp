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
#include "EveMarketDataItemPriceImporterXmlReceiver.h"

namespace Evernus
{
    EveMarketDataItemPriceImporterXmlReceiver
    ::EveMarketDataItemPriceImporterXmlReceiver(const ItemPriceImporter::TypeLocationPairs &target, const QXmlNamePool &namePool)
        : QAbstractXmlReceiver{}
        , mNamePool{namePool}
        , mDesired{target}
    {
    }

    void EveMarketDataItemPriceImporterXmlReceiver::atomicValue(const QVariant &value)
    {
        Q_UNUSED(value);
    }

    void EveMarketDataItemPriceImporterXmlReceiver::attribute(const QXmlName &name, const QStringRef &value)
    {
        auto &current = *mCurrentElement;

        if (name.localName(mNamePool) == "buysell")
            current.setType((value == "s") ? (ItemPrice::Type::Sell) : (ItemPrice::Type::Buy));
        else if (name.localName(mNamePool) == "typeID")
            current.setTypeId(value.toUInt());
        else if (name.localName(mNamePool) == "stationID")
            current.setLocationId(value.toULongLong());
        else if (name.localName(mNamePool) == "price")
            current.setValue(value.toDouble());
    }

    void EveMarketDataItemPriceImporterXmlReceiver::characters(const QStringRef &value)
    {
        Q_UNUSED(value);
    }

    void EveMarketDataItemPriceImporterXmlReceiver::comment(const QString &value)
    {
        Q_UNUSED(value);
    }

    void EveMarketDataItemPriceImporterXmlReceiver::endDocument()
    {
    }

    void EveMarketDataItemPriceImporterXmlReceiver::endElement()
    {
        const auto &current = *mCurrentElement;
        const auto key = std::make_pair(current.getTypeId(), current.getLocationId());

        if (mDesired.find(key) != std::end(mDesired))
        {
            if (current.getType() == ItemPrice::Type::Buy)
            {
                const auto it = mProcessedBuy.find(key);
                if (it == std::end(mProcessedBuy))
                {
                    mResult.emplace_back(std::move(current));
                    mProcessedBuy.emplace(key, &mResult.back());
                }
                else if (it->second->getValue() < current.getValue())
                {
                    *it->second = std::move(current);
                }
            }
            else
            {
                const auto it = mProcessedSell.find(key);
                if (it == std::end(mProcessedSell))
                {
                    mResult.emplace_back(std::move(current));
                    mProcessedSell.emplace(key, &mResult.back());
                }
                else if (it->second->getValue() > current.getValue())
                {
                    *it->second = std::move(current);
                }
            }
        }
    }

    void EveMarketDataItemPriceImporterXmlReceiver::endOfSequence()
    {
    }

    void EveMarketDataItemPriceImporterXmlReceiver::namespaceBinding(const QXmlName &name)
    {
        Q_UNUSED(name);
    }

    void EveMarketDataItemPriceImporterXmlReceiver::processingInstruction(const QXmlName &target, const QString &value)
    {
        Q_UNUSED(target);
        Q_UNUSED(value);
    }

    void EveMarketDataItemPriceImporterXmlReceiver::startDocument()
    {
    }

    void EveMarketDataItemPriceImporterXmlReceiver::startElement(const QXmlName &name)
    {
        Q_UNUSED(name);
        mCurrentElement = std::make_unique<ItemPrice>();
        mCurrentElement->setUpdateTime(QDateTime::currentDateTimeUtc());
    }

    void EveMarketDataItemPriceImporterXmlReceiver::startOfSequence()
    {
    }

    EveMarketDataItemPriceImporterXmlReceiver::Result EveMarketDataItemPriceImporterXmlReceiver::getResult() const &
    {
        return mResult;
    }

    EveMarketDataItemPriceImporterXmlReceiver::Result &&EveMarketDataItemPriceImporterXmlReceiver::getResult() && noexcept
    {
        return std::move(mResult);
    }
}
