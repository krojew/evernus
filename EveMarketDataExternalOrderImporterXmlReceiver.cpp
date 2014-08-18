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
#include "EveMarketDataExternalOrderImporterXmlReceiver.h"

namespace Evernus
{
    EveMarketDataExternalOrderImporterXmlReceiver::EveMarketDataExternalOrderImporterXmlReceiver(const QXmlNamePool &namePool)
        : QAbstractXmlReceiver{}
        , mNamePool{namePool}
    {
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::atomicValue(const QVariant &value)
    {
        Q_UNUSED(value);
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::attribute(const QXmlName &name, const QStringRef &value)
    {
        auto &current = *mCurrentElement;

        if (name.localName(mNamePool) == "buysell")
        {
            current.setType((value == "s") ? (ExternalOrder::Type::Sell) : (ExternalOrder::Type::Buy));
        }
        else if (name.localName(mNamePool) == "orderID")
        {
            current.setId(value.toULongLong());
        }
        else if (name.localName(mNamePool) == "typeID")
        {
            current.setTypeId(value.toUInt());
        }
        else if (name.localName(mNamePool) == "stationID")
        {
            current.setStationId(value.toULongLong());
        }
        else if (name.localName(mNamePool) == "price")
        {
            current.setPrice(value.toDouble());
        }
        else if (name.localName(mNamePool) == "solarsystemID")
        {
            current.setSolarSystemId(value.toUInt());
        }
        else if (name.localName(mNamePool) == "regionID")
        {
            current.setRegionId(value.toUInt());
        }
        else if (name.localName(mNamePool) == "range")
        {
            current.setRange(value.toShort());
        }
        else if (name.localName(mNamePool) == "volEntered")
        {
            current.setVolumeEntered(value.toUInt());
        }
        else if (name.localName(mNamePool) == "volRemaining")
        {
            current.setVolumeRemaining(value.toUInt());
        }
        else if (name.localName(mNamePool) == "minVolume")
        {
            current.setMinVolume(value.toUInt());
        }
        else if (name.localName(mNamePool) == "issued")
        {
            auto dt = QDateTime::fromString(value.toString(), emdDateFormat);
            dt.setTimeSpec(Qt::UTC);

            current.setIssued(dt);

            if (mCurrentExpires.isValid())
                current.setDuration(dt.daysTo(mCurrentExpires));
        }
        else if (name.localName(mNamePool) == "expires")
        {
            mCurrentExpires = QDateTime::fromString(value.toString(), emdDateFormat);
            mCurrentExpires.setTimeSpec(Qt::UTC);

            if (current.getIssued().isValid())
                current.setDuration(current.getIssued().daysTo(mCurrentExpires));
        }
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::characters(const QStringRef &value)
    {
        Q_UNUSED(value);
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::comment(const QString &value)
    {
        Q_UNUSED(value);
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::endDocument()
    {
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::endElement()
    {
        mResult.emplace_back(std::move(*mCurrentElement));
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::endOfSequence()
    {
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::namespaceBinding(const QXmlName &name)
    {
        Q_UNUSED(name);
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::processingInstruction(const QXmlName &target, const QString &value)
    {
        Q_UNUSED(target);
        Q_UNUSED(value);
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::startDocument()
    {
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::startElement(const QXmlName &name)
    {
        Q_UNUSED(name);
        mCurrentElement = std::make_unique<ExternalOrder>();
        mCurrentElement->setUpdateTime(QDateTime::currentDateTimeUtc());
        mCurrentExpires = QDateTime{};
    }

    void EveMarketDataExternalOrderImporterXmlReceiver::startOfSequence()
    {
    }

    EveMarketDataExternalOrderImporterXmlReceiver::Result EveMarketDataExternalOrderImporterXmlReceiver::getResult() const &
    {
        return mResult;
    }

    EveMarketDataExternalOrderImporterXmlReceiver::Result &&EveMarketDataExternalOrderImporterXmlReceiver::getResult() && noexcept
    {
        return std::move(mResult);
    }
}
