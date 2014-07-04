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
        auto &current = mResult.back();

        if (name.localName(mNamePool) == "buysell")
            current.setType((value == "s") ? (ItemPrice::Type::Sell) : (ItemPrice::Type::Buy));
        else if (name.localName(mNamePool) == "typeID")
            current.setTypeId(value.toUInt());
        else if (name.localName(mNamePool) == "stationID")
            current.setLocationId(value.toULongLong());
        else if (name.localName(mNamePool) == "price")
            current.setValue(value.toDouble());
        else if (name.localName(mNamePool) == "updated")
        {
            const auto dt = QDateTime::fromString(value.toString(), "yyyy-MM-dd HH:mm:ss");
            current.setUpdateTime(dt);
        }
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
        const auto &current = mResult.back();
        const auto key = std::make_pair(current.getTypeId(), current.getLocationId());

        if (mDesired.find(key) == std::end(mDesired))
        {
            mResult.pop_back();
        }
        else if (current.getType() == ItemPrice::Type::Buy)
        {
            if (mProcessedBuy.find(key) != std::end(mProcessedBuy))
                mResult.pop_back();
            else
                mProcessedBuy.emplace(key);
        }
        else
        {
            if (mProcessedSell.find(key) != std::end(mProcessedSell))
                mResult.pop_back();
            else
                mProcessedSell.emplace(key);
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
        mResult.emplace_back();
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
