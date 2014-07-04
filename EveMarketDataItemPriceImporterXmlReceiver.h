#pragma once

#include <vector>

#include <QAbstractXmlReceiver>

#include "ItemPriceImporter.h"
#include "ItemPrice.h"

namespace Evernus
{
    class EveMarketDataItemPriceImporterXmlReceiver
        : public QAbstractXmlReceiver
    {
    public:
        typedef std::vector<ItemPrice> Result;

        EveMarketDataItemPriceImporterXmlReceiver(const ItemPriceImporter::TypeLocationPairs &target, const QXmlNamePool &namePool);
        virtual ~EveMarketDataItemPriceImporterXmlReceiver() = default;

        virtual void atomicValue(const QVariant &value) override;
        virtual void attribute(const QXmlName &name, const QStringRef &value) override;
        virtual void characters(const QStringRef &value) override;
        virtual void comment(const QString &value) override;
        virtual void endDocument() override;
        virtual void endElement() override;
        virtual void endOfSequence() override;
        virtual void namespaceBinding(const QXmlName &name) override;
        virtual void processingInstruction(const QXmlName &target, const QString &value) override;
        virtual void startDocument() override;
        virtual void startElement(const QXmlName &name) override;
        virtual void startOfSequence() override;

        Result getResult() const &;
        Result &&getResult() && noexcept;

    private:
        const QXmlNamePool &mNamePool;

        Result mResult;

        ItemPriceImporter::TypeLocationPairs mProcessedSell, mProcessedBuy, mDesired;
    };
}
