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
#include <QStandardPaths>
#include <QStringBuilder>
#include <QSettings>
#include <QDebug>
#include <QDir>

#include "EveCacheManager.h"
#include "ExternalOrder.h"
#include "PathSettings.h"

#include "CacheExternalOrderImporter.h"

namespace Evernus
{
    namespace
    {
        QDateTime convertTime(qint64 cacheTime)
        {
            return QDateTime::fromTime_t(cacheTime / 10000000ll - 11644473600ll, Qt::UTC);
        }
    }

    void CacheExternalOrderImporter::fetchExternalOrders(const TypeLocationPairs &target) const
    {
        if (target.empty())
        {
            emit externalOrdersChanged(std::vector<ExternalOrder>{});
            return;
        }

        const auto cachePath = getEveCachePath();
        if (cachePath.isEmpty())
        {
            emit error(tr("Couldn't determine Eve cache path."));
            return;
        }

        if (!QDir{cachePath}.exists())
        {
            emit error(tr("Eve cache path doesn't exist."));
            return;
        }

        EveCacheManager manager{cachePath};
        manager.addCacheFolderFilter("CachedMethodCalls");
        manager.addMethodFilter("GetOrders");

        try
        {
            manager.parseMachoNet();

            const auto &streams = manager.getStreams();

            std::vector<ExternalOrder> orders;
            orders.reserve(streams.size());

            for (const auto &stream : streams)
            {
                const auto &children = stream->getChildren();
                if (children.size() < 2)
                {
                    qDebug() << "Invalid order stream size!";
                    continue;
                }

                const auto dict = dynamic_cast<const EveCacheNode::Dictionary *>(children[1].get());
                if (dict == nullptr)
                {
                    qDebug() << "Cannot find order dictionary!";
                    continue;
                }

                if (dict->getChildren().empty() ||
                    dynamic_cast<const EveCacheNode::Object *>(dict->getChildren().front().get()) == nullptr)
                {
                    qDebug() << "Invalid order dictionary!";
                    continue;
                }

                const auto timeContainer = dict->getByName("version");
                if (timeContainer == nullptr || timeContainer->getChildren().empty())
                {
                    qDebug() << "Invalid time container!";
                    continue;
                }

                const auto time = dynamic_cast<const EveCacheNode::LongLong *>(timeContainer->getChildren().front().get());
                if (time == nullptr)
                {
                    qDebug() << "Missing timestamp!";
                    continue;
                }

                fillOrders(convertTime(time->getValue()), orders, *dict->getChildren().front());
            }

            qDebug() << "Parsed" << orders.size() << "orders.";

            emit externalOrdersChanged(orders);
        }
        catch (const std::exception &e)
        {
            emit error(e.what());
        }
    }

    QString CacheExternalOrderImporter::getEveCachePath()
    {
        QSettings settings;

        const auto machoNetPathSegment = "MachoNet/";
        const auto tranquilityIpPathSegment = "87.237.38.200/";

#ifdef Q_OS_WIN
        qDebug() << "Looking for eve cache path...";

        auto basePath = settings.value(PathSettings::evePathKey).toString();
        qDebug() << "Eve path:" << basePath;

        if (basePath.isEmpty())
            return QString{};

        const QString tranquilityPathSegment = "_tranquility";
        const QString cachePathSegment = "cache";

        basePath.remove(":").replace("\\", "_").replace(" ", "_").replace("/", "_");
        basePath += tranquilityPathSegment % "/" % cachePathSegment % "/";

        qDebug() << "Combined base path:" << basePath;

        const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % "/CCP/EVE/";
        qDebug() << "App data path:" << appDataPath;

        basePath.prepend(appDataPath);
#else
        auto basePath = settings.value(PathSettings::eveCachePathKey).toString();
        qDebug() << "Eve path:" << basePath;

        if (basePath.isEmpty())
            return QString{};

        basePath += "/";
#endif
        return basePath % machoNetPathSegment % tranquilityIpPathSegment;
    }

    void CacheExternalOrderImporter
    ::fillOrders(const QDateTime &updated, std::vector<ExternalOrder> &orders, const EveCacheNode::Base &node)
    {
        const auto &children = node.getChildren();
        for (auto it = std::begin(children); it != std::end(children); ++it)
        {
            if (dynamic_cast<const EveCacheNode::DBRow *>(it->get()) != nullptr)
            {
                ++it;
                if (it == std::end(children))
                    return;

                parseDbRow(updated, orders, **it);
            }
            else
            {
                fillOrders(updated, orders, **it);
            }
        }
    }

    void CacheExternalOrderImporter
    ::parseDbRow(const QDateTime &updated, std::vector<ExternalOrder> &orders, const EveCacheNode::Base &node)
    {
        const auto &children = node.getChildren();
        if (children.empty())
            return;

        ExternalOrder order;

        for (auto it = std::begin(children); it != std::end(children); ++it)
        {
            const auto &value = *it;

            ++it;
            if (it == std::end(children))
                return;

            const auto key = dynamic_cast<const EveCacheNode::Marker *>(it->get());
            const auto ident = dynamic_cast<const EveCacheNode::Ident *>(it->get());

            auto typeKey = -1;
            if (key != nullptr)
            {
                typeKey = key->getId();
            }
            else if (ident != nullptr)
            {
                if (ident->getName() == "issueDate")
                    typeKey = 131;
                else
                    return;
            }
            else
            {
                return;
            }

            int intV = 0;
            qint64 longV = 0;
            double realV = 0.;

            const auto intNode = dynamic_cast<const EveCacheNode::Int *>(value.get());
            if (intNode != nullptr)
            {
                intV = intNode->getValue();
            }
            else
            {
                const auto longNode = dynamic_cast<const EveCacheNode::LongLong *>(value.get());
                if (longNode != nullptr)
                {
                    longV = longNode->getValue();
                }
                else
                {
                    const auto realNode = dynamic_cast<const EveCacheNode::Real *>(value.get());
                    if (realNode != nullptr)
                        realV = realNode->getValue();
                }
            }
            qDebug() << typeKey << intV << longV << realV;

            switch (typeKey) {
            case 139:
                order.setValue(longV / 10000.);
                break;
            case 161:
                // volume remaining
                break;
            case 131:
                // issued
                break;
            case 138:
                order.setId(longV);
                break;
            case 160:
                // volume entered
                break;
            case 137:
                // min volume
                break;
            case 155:
                order.setLocationId(intV);
                break;
            case 141:
                order.setRegionId(intV);
                break;
            case 150:
                order.setSolarSystemId(intV);
                break;
            case 41:
                // jumps
                break;
            case 74:
                order.setTypeId(intV);
                break;
            case 140:
                order.setRange(intV);
                break;
            case 126:
                // duration
                break;
            case 116:
                order.setType((intV == 0) ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));
            }
        }

        if (order.getId() != ExternalOrder::invalidId)
            orders.emplace_back(std::move(order));
    }
}
