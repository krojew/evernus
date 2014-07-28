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
#pragma once

#include <unordered_map>
#include <vector>

#include <QAbstractXmlReceiver>

#include "ExternalOrderImporter.h"
#include "ExternalOrder.h"

namespace Evernus
{
    class EveMarketDataExternalOrderImporterXmlReceiver
        : public QAbstractXmlReceiver
    {
    public:
        typedef std::vector<ExternalOrder> Result;

        EveMarketDataExternalOrderImporterXmlReceiver(const ExternalOrderImporter::TypeLocationPairs &target, const QXmlNamePool &namePool);
        virtual ~EveMarketDataExternalOrderImporterXmlReceiver() = default;

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
        std::unique_ptr<ExternalOrder> mCurrentElement;

        ExternalOrderImporter::TypeLocationPairs mDesired;
        std::unordered_map<ExternalOrderImporter::TypeLocationPair, ExternalOrder *, boost::hash<ExternalOrderImporter::TypeLocationPair>>
        mProcessedSell, mProcessedBuy;
    };
}
