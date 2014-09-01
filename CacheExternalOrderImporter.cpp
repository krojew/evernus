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
#include <QDirIterator>
#include <QSettings>
#include <QDebug>

#include "EveCacheManager.h"
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
        const auto cachePaths = getEveCachePaths();
        if (cachePaths.isEmpty())
        {
            emit error(tr("Couldn't determine Eve cache path. Did you set it in the Preferences?"));
            return;
        }

        EveCacheManager manager{cachePaths};
        manager.addCacheFolderFilter("CachedMethodCalls");
        manager.addMethodFilter("GetOrders");

        try
        {
            manager.parseMachoNet();

            const auto &streams = manager.getStreams();

            std::vector<ExternalOrder> orders;
            orders.reserve(streams.size());

            LogTimeMap timeMap;

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

                fillOrders(convertTime(time->getValue()), orders, *dict->getChildren().front(), timeMap);
            }

            qDebug() << "Parsed" << orders.size() << "orders.";

            emit externalOrdersChanged(orders);
        }
        catch (const std::exception &e)
        {
            emit error(e.what());
        }
    }

    QStringList CacheExternalOrderImporter::getEveCachePaths()
    {
        const auto cachePathSegment = "MachoNet/87.237.38.200/";
        QDir appDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/CCP/EVE/";

        QStringList clientPaths;

        QSettings settings;
        const auto basePath = settings.value(PathSettings::eveCachePathKey).toString();
        if (basePath.isEmpty())
        {

            clientPaths = appDataPath.entryList(QStringList{"*_tranquility"}, QDir::Dirs | QDir::Readable);
            if (clientPaths.isEmpty())
            {
                clientPaths << QString{};
            }
            else
            {
                for (auto &path : clientPaths)
                    path += "/cache/";
            }
        }
        else
        {
            appDataPath = basePath;
            clientPaths << QString{};
        }

        auto max = 0u;

        for (const auto &path : clientPaths)
        {
            QDirIterator dirIt{appDataPath.filePath(path + cachePathSegment)};
            while (dirIt.hasNext())
            {
                dirIt.next();

                auto ok = false;
                const auto cur = dirIt.fileName().toUInt(&ok);

                if (ok && cur > max)
                    max = cur;
            }
        }

        if (max == 0)
            return QStringList{};

        QStringList result;
        for (const auto &path : clientPaths)
        {
            const auto combined = QString{"%1%2%3/"}.arg(path).arg(cachePathSegment).arg(max);
            if (appDataPath.exists(combined))
                result << appDataPath.filePath(combined);
        }

        return result;
    }

    void CacheExternalOrderImporter
    ::fillOrders(const QDateTime &updated, std::vector<ExternalOrder> &orders, const EveCacheNode::Base &node, LogTimeMap &timeMap)
    {
        const auto &children = node.getChildren();
        for (auto it = std::begin(children); it != std::end(children); ++it)
        {
            if (dynamic_cast<const EveCacheNode::DBRow *>(it->get()) != nullptr)
            {
                ++it;
                if (it == std::end(children))
                    return;

                parseDbRow(updated, orders, **it, timeMap);
            }
            else
            {
                fillOrders(updated, orders, **it, timeMap);
            }
        }
    }

    void CacheExternalOrderImporter
    ::parseDbRow(const QDateTime &updated, std::vector<ExternalOrder> &orders, const EveCacheNode::Base &node, LogTimeMap &timeMap)
    {
        const auto &children = node.getChildren();
        if (children.empty())
            return;

        ExternalOrder order;
        order.setUpdateTime(updated);

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
            bool boolV = false;

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
                    {
                        realV = realNode->getValue();
                    }
                    else
                    {
                        const auto boolNode = dynamic_cast<const EveCacheNode::Bool *>(value.get());
                        if (boolNode != nullptr)
                            boolV = boolNode->getValue();
                    }
                }
            }

            switch (typeKey) {
            case 139:
                order.setPrice(longV / 10000.);
                break;
            case 161:
                order.setVolumeRemaining(realV);
                break;
            case 131:
                order.setIssued(convertTime(longV));
                break;
            case 138:
                {
                    const auto it = timeMap.find(longV);
                    if (it != std::end(timeMap) && it->second > updated)
                        return;

                    order.setId(longV);

                    timeMap[longV] = updated;
                }
                break;
            case 160:
                order.setVolumeEntered(intV);
                break;
            case 137:
                order.setMinVolume(intV);
                break;
            case 155:
                order.setStationId(intV);
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
                order.setDuration(intV);
                break;
            case 116:
                order.setType((boolV) ? (ExternalOrder::Type::Buy) : (ExternalOrder::Type::Sell));
            }
        }

        if (order.getId() != ExternalOrder::invalidId)
            orders.emplace_back(std::move(order));
    }
}
