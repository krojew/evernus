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
#include <unordered_set>
#include <algorithm>
#include <stdexcept>
#include <limits>

#include <QCoreApplication>
#include <QMessageBox>
#include <QSettings>
#include <QFile>

#include "FavoriteItemRepository.h"
#include "CharacterRepository.h"
#include "MarketOrderProvider.h"
#include "ItemCostProvider.h"
#include "EveDataProvider.h"
#include "ExternalOrder.h"
#include "OrderSettings.h"
#include "PriceSettings.h"
#include "IGBSettings.h"
#include "PriceUtils.h"

#include "qxthttpsessionmanager.h"
#include "qxtwebevent.h"

#include "IGBService.h"

namespace Evernus
{
    const QString IGBService::limitToStationsCookie = "limitToStations";
    const QString IGBService::orderHtmlTemplate = "<li><a class='order' id='order-%2' href='#' onclick='showOrder(%2);'>%1</a></li>";
    const QString IGBService::setDestinationMessage = "setDestination";

    IGBService::IGBService(const MarketOrderProvider &orderProvider,
                           const MarketOrderProvider &corpOrderProvider,
                           const EveDataProvider &dataProvider,
                           const ItemCostProvider &itemCostProvider,
                           const FavoriteItemRepository &favoriteItemRepo,
                           const CharacterRepository &characterRepository,
                           QxtHttpSessionManager *sm,
                           QObject *parent)
        : QxtWebSlotService(sm, parent)
        , mOrderProvider(orderProvider)
        , mCorpOrderProvider(corpOrderProvider)
        , mDataProvider(dataProvider)
        , mItemCostProvider(itemCostProvider)
        , mFavoriteItemRepo(favoriteItemRepo)
        , mCharacterRepository(characterRepository)
    {
        mMainTemplate.open(":/html/igb_template.html");
        mOrderTemplate.open(":/html/igb_order_template.html");
        mFavoriteTemplate.open(":/html/igb_favorite_template.html");

        QFile orderScript{":/html/igb_order_js_template.html"};
        if (!orderScript.open(QIODevice::ReadOnly))
            throw std::runtime_error{tr("Error reading IGB script template.").toStdString()};

        QSettings settings;

        mMainTemplate["active-link-text"] = tr("Character Active Orders");
        mMainTemplate["fulfilled-link-text"] = tr("Character Fulfilled Orders");
        mMainTemplate["overbid-link-text"] = tr("Character Overbid Orders");
        mMainTemplate["below-margin-link-text"] = tr("Character Below Min. Margin Orders");
        mMainTemplate["corp-active-link-text"] = tr("Corporation Active Orders");
        mMainTemplate["corp-fulfilled-link-text"] = tr("Corporation Fulfilled Orders");
        mMainTemplate["corp-overbid-link-text"] = tr("Corporation Overbid Orders");
        mMainTemplate["corp-below-margin-link-text"] = tr("Corporation Below Min. Margin Orders");
        mMainTemplate["favorite-link-text"] = tr("Favorite Items");
        mMainTemplate["open-margin-tool-link-text"] = tr("Open Margin Tool");
        mMainTemplate["port"] = settings.value(IGBSettings::portKey, IGBSettings::portDefault).toString();
        mMainTemplate["set-destination-message"] = setDestinationMessage;

        mOrderTemplate["order-script"] = orderScript.readAll();
        mOrderTemplate["prev-order-text"] = tr("Show Previous Entry");
        mOrderTemplate["next-order-text"] = tr("Show Next Entry");
        mOrderTemplate["current-order-text"] = tr("Current entry:");
        mOrderTemplate["scan-delay-text"] = tr("Scan delay:");
        mOrderTemplate["start-scan-text"] = tr("Start Scan");
        mOrderTemplate["stop-scan-text"] = tr("Stop Scan");
        mOrderTemplate["stop-at-end-text"] = tr("Stop at end");

        mFavoriteTemplate.copyArguments(mOrderTemplate);
        mFavoriteTemplate["favorite-text"] = tr("Favorite items:");

        mOrderTemplate["sell-orders-text"] = tr("Sell orders:");
        mOrderTemplate["buy-orders-text"] = tr("Buy orders:");
        mOrderTemplate["limit-to-stations-text"] = tr("Limit to current station, if available");
        mOrderTemplate["limit-to-stations-cookie"] = limitToStationsCookie;
    }

    void IGBService::index(QxtWebRequestEvent *event)
    {
        renderContent(event, tr("Please select a category from the menu."));
    }

    void IGBService::active(QxtWebRequestEvent *event)
    {
        showOrders(event,
                   [=] {
                       return mOrderProvider.getSellOrders(getCharacterId(event));
                   },
                   [=] {
                       return mOrderProvider.getBuyOrders(getCharacterId(event));
                   },
                   MarketOrder::State::Active,
                   false);
    }

    void IGBService::fulfilled(QxtWebRequestEvent *event)
    {
        showOrders(event,
                   [=] {
                       return mOrderProvider.getSellOrders(getCharacterId(event));
                   },
                   [=] {
                       return mOrderProvider.getBuyOrders(getCharacterId(event));
                   },
                   MarketOrder::State::Fulfilled,
                   true);
    }

    void IGBService::overbid(QxtWebRequestEvent *event)
    {
        showOrders(event,
                   [=] {
                       QSettings settings;
                       const auto stationOnly
                           = settings.value(OrderSettings::limitSellToStationKey, OrderSettings::limitSellToStationDefault).toBool();

                       auto orders = mOrderProvider.getSellOrders(getCharacterId(event));
                       for (auto it = std::begin(orders); it != std::end(orders);)
                       {
                           const auto price = (stationOnly) ?
                                              (mDataProvider.getTypeStationSellPrice((*it)->getTypeId(), (*it)->getStationId())) :
                                              (mDataProvider.getTypeRegionSellPrice((*it)->getTypeId(), mDataProvider.getStationRegionId((*it)->getStationId())));
                           if (price->isNew() || price->getPrice() >= (*it)->getPrice())
                               it = orders.erase(it);
                           else
                               ++it;
                       }

                       return orders;
                   },
                   [=] {
                       auto orders = mOrderProvider.getBuyOrders(getCharacterId(event));
                       for (auto it = std::begin(orders); it != std::end(orders);)
                       {
                           const auto price = mDataProvider.getTypeBuyPrice((*it)->getTypeId(), (*it)->getStationId(), (*it)->getRange());
                           if (price->isNew() || price->getPrice() <= (*it)->getPrice())
                               it = orders.erase(it);
                           else
                               ++it;
                       }

                       return orders;
                   },
                   MarketOrder::State::Active,
                   false);
    }

    void IGBService::belowMargin(QxtWebRequestEvent *event)
    {
        try
        {
            const auto character = mCharacterRepository.find(getCharacterId(event));
            const auto taxes = PriceUtils::calculateTaxes(*character);

            QSettings settings;
            const auto min = settings.value(PriceSettings::minMarginKey, PriceSettings::minMarginDefault).toDouble();

            showOrders(event,
                       [=, &character, &taxes] {
                           auto orders = mOrderProvider.getSellOrders(character->getId());
                           for (auto it = std::begin(orders); it != std::end(orders);)
                           {
                               const auto cost = mItemCostProvider.fetchForCharacterAndType(character->getId(), (*it)->getTypeId());
                               if (PriceUtils::getMargin(cost->getAdjustedCost(), (*it)->getPrice(), taxes) > min)
                                   it = orders.erase(it);
                               else
                                   ++it;
                           }

                           return orders;
                       },
                       [=, &character, &taxes] {
                           QSettings settings;
                           const auto stationOnly
                               = settings.value(OrderSettings::limitSellToStationKey, OrderSettings::limitSellToStationDefault).toBool();

                           auto orders = mOrderProvider.getBuyOrders(character->getId());
                           for (auto it = std::begin(orders); it != std::end(orders);)
                           {
                               const auto price = (stationOnly) ?
                                                  (mDataProvider.getTypeStationSellPrice((*it)->getTypeId(), (*it)->getStationId())) :
                                                  (mDataProvider.getTypeRegionSellPrice((*it)->getTypeId(), mDataProvider.getStationRegionId((*it)->getStationId())));
                               if (price->isNew() || PriceUtils::getMargin((*it)->getPrice(), price->getPrice() - PriceUtils::getPriceDelta(), taxes) > min)
                                   it = orders.erase(it);
                               else
                                   ++it;
                           }

                           return orders;
                       },
                       MarketOrder::State::Active,
                       false);
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            renderContent(event, tr("Character not found!"));
        }
    }

    void IGBService::corpActive(QxtWebRequestEvent *event)
    {
        showOrders(event,
                   [=] {
                       return mCorpOrderProvider.getSellOrdersForCorporation(getCorporationId(event));
                   },
                   [=] {
                       return mCorpOrderProvider.getBuyOrdersForCorporation(getCorporationId(event));
                   },
                   MarketOrder::State::Active,
                   false);
    }

    void IGBService::corpFulfilled(QxtWebRequestEvent *event)
    {
        showOrders(event,
                   [=] {
                       return mCorpOrderProvider.getSellOrdersForCorporation(getCorporationId(event));
                   },
                   [=] {
                       return mCorpOrderProvider.getBuyOrdersForCorporation(getCorporationId(event));
                   },
                   MarketOrder::State::Fulfilled,
                   true);
    }

    void IGBService::corpOverbid(QxtWebRequestEvent *event)
    {
        showOrders(event,
                   [=] {
                       QSettings settings;
                       const auto stationOnly
                           = settings.value(OrderSettings::limitSellToStationKey, OrderSettings::limitSellToStationDefault).toBool();

                       auto orders = mCorpOrderProvider.getSellOrdersForCorporation(getCorporationId(event));
                       for (auto it = std::begin(orders); it != std::end(orders);)
                       {
                           const auto price = (stationOnly) ?
                                              (mDataProvider.getTypeStationSellPrice((*it)->getTypeId(), (*it)->getStationId())) :
                                              (mDataProvider.getTypeRegionSellPrice((*it)->getTypeId(), mDataProvider.getStationRegionId((*it)->getStationId())));
                           if (price->isNew() || price->getPrice() > (*it)->getPrice())
                               it = orders.erase(it);
                           else
                               ++it;
                       }

                       return orders;
                   },
                   [=] {
                       auto orders = mCorpOrderProvider.getBuyOrdersForCorporation(getCorporationId(event));
                       for (auto it = std::begin(orders); it != std::end(orders);)
                       {
                           const auto price = mDataProvider.getTypeBuyPrice((*it)->getTypeId(), (*it)->getStationId(), (*it)->getRange());
                           if (price->isNew() || price->getPrice() < (*it)->getPrice())
                               it = orders.erase(it);
                           else
                               ++it;
                       }

                       return orders;
                   },
                   MarketOrder::State::Fulfilled,
                   true);
    }

    void IGBService::corpBelowMargin(QxtWebRequestEvent *event)
    {
        try
        {
            const auto character = mCharacterRepository.find(getCharacterId(event));
            const auto taxes = PriceUtils::calculateTaxes(*character);

            QSettings settings;
            const auto min = settings.value(PriceSettings::minMarginKey, PriceSettings::minMarginDefault).toDouble();

            showOrders(event,
                       [=, &character, &taxes] {
                           auto orders = mCorpOrderProvider.getSellOrdersForCorporation(character->getCorporationId());
                           for (auto it = std::begin(orders); it != std::end(orders);)
                           {
                               const auto cost = mItemCostProvider.fetchForCharacterAndType(character->getId(), (*it)->getTypeId());
                               if (PriceUtils::getMargin(cost->getAdjustedCost(), (*it)->getPrice(), taxes) > min)
                                   it = orders.erase(it);
                               else
                                   ++it;
                           }

                           return orders;
                       },
                       [=, &character, &taxes] {
                           QSettings settings;
                           const auto stationOnly
                               = settings.value(OrderSettings::limitSellToStationKey, OrderSettings::limitSellToStationDefault).toBool();

                           auto orders = mCorpOrderProvider.getBuyOrdersForCorporation(character->getCorporationId());
                           for (auto it = std::begin(orders); it != std::end(orders);)
                           {
                               const auto price = (stationOnly) ?
                                                  (mDataProvider.getTypeStationSellPrice((*it)->getTypeId(), (*it)->getStationId())) :
                                                  (mDataProvider.getTypeRegionSellPrice((*it)->getTypeId(), mDataProvider.getStationRegionId((*it)->getStationId())));
                               if (price->isNew() || PriceUtils::getMargin((*it)->getPrice(), price->getPrice() - PriceUtils::getPriceDelta(), taxes) > min)
                                   it = orders.erase(it);
                               else
                                   ++it;
                           }

                           return orders;
                       },
                       MarketOrder::State::Active,
                       false);
        }
        catch (const CharacterRepository::NotFoundException &)
        {
            renderContent(event, tr("Character not found!"));
        }
    }

    void IGBService::favorite(QxtWebRequestEvent *event)
    {
        QStringList idContainer, typeIdContainer;
        QString result;

        const auto items = mFavoriteItemRepo.fetchAll();
        auto i = 0u;

        for (const auto &item : items)
        {
            idContainer << QString::number(++i);
            typeIdContainer << QString::number(item->getId());

            result.append(orderHtmlTemplate.arg(mDataProvider.getTypeName(item->getId())).arg(idContainer.last()));
        }

        QSettings settings;

        mFavoriteTemplate["order-ids"] = idContainer.join(", ");
        mFavoriteTemplate["type-ids"] = typeIdContainer.join(", ");
        mFavoriteTemplate["scan-delay"] = settings.value(IGBSettings::scanDelayKey, IGBSettings::scanDelayDefault).toString();
        mFavoriteTemplate["favorite-items"] = result;

        renderContent(event, mFavoriteTemplate.render());
    }

    void IGBService::openMarginTool(QxtWebRequestEvent *event)
    {
        emit openMarginTool();

        postEvent(new QxtWebPageEvent(event->sessionID, event->requestID, QByteArray{}));
    }

    void IGBService::update(QxtWebRequestEvent *event)
    {
        mPollingRequests.emplace_back(event);
    }

    void IGBService::setDestinationInEve(quint64 locationId)
    {
        sendEventToIGB(QString{"{ message: '%1', id: %2 }"}.arg(setDestinationMessage).arg(locationId).toLatin1());
    }

    void IGBService::renderContent(QxtWebRequestEvent *event, const QString &content)
    {
        const auto trustedHeader = "EVE_TRUSTED";

        mMainTemplate["content"] = (!event->headers.contains(trustedHeader) || event->headers.values(trustedHeader).first() != "Yes") ?
                                   (QString{"<h3>%1</h3>%2"}.arg(tr("Website trust is required. Please add it to Trusted Sites.")).arg(content)) :
                                   (content);

        postEvent(new QxtWebPageEvent(event->sessionID, event->requestID, mMainTemplate.render().toUtf8()));

        mPollingRequests.clear();
    }

    QString IGBService::renderOrderList(const std::vector<std::shared_ptr<MarketOrder>> &orders,
                                        QStringList &idContainer,
                                        QStringList &typeIdContainer,
                                        uint stationId) const
    {
        QString result;
        for (const auto &order : orders)
        {
            if (stationId != 0 && stationId != order->getStationId())
                continue;

            idContainer << QString::number(order->getId());
            typeIdContainer << QString::number(order->getTypeId());

            result.append(orderHtmlTemplate.arg(mDataProvider.getTypeName(order->getTypeId())).arg(order->getId()));
        }

        return result;
    }

    template<class SellFunc, class BuyFunc>
    void IGBService::showOrders(QxtWebRequestEvent *event,
                                const SellFunc &sellFunc,
                                const BuyFunc &buyFunc,
                                MarketOrder::State state,
                                bool needsDelta)
    {
        QStringList idContainer, typeIdContainer;
        QSettings settings;

        const auto stationId = getStationIdForFiltering(event);

        mOrderTemplate["sell-orders"] = renderOrderList(
            filterAndSort(sellFunc(), state, needsDelta), idContainer, typeIdContainer, stationId);
        mOrderTemplate["buy-orders-start"] = QString::number(idContainer.size() + 1);
        mOrderTemplate["buy-orders"] = renderOrderList(
            filterAndSort(buyFunc(), state, needsDelta), idContainer, typeIdContainer, stationId);
        mOrderTemplate["order-ids"] = idContainer.join(", ");
        mOrderTemplate["type-ids"] = typeIdContainer.join(", ");
        mOrderTemplate["scan-delay"] = settings.value(IGBSettings::scanDelayKey, IGBSettings::scanDelayDefault).toString();

        renderContent(event, mOrderTemplate.render());
    }

    IGBService::OrderList IGBService::filterAndSort(const OrderList &orders, MarketOrder::State state, bool needsDelta) const
    {
        std::unordered_set<EveType::IdType> usedTypes;

        OrderList result;
        result.reserve(orders.size());

        for (const auto &order : orders)
        {
            if ((order->getState() == state) && (!needsDelta || order->getDelta() != 0) && (usedTypes.find(order->getTypeId()) == std::end(usedTypes)))
            {
                result.emplace_back(order);
                usedTypes.emplace(order->getTypeId());
            }
        }

        std::sort(std::begin(result), std::end(result), [this](const auto &o1, const auto &o2) {
            return mDataProvider.getTypeName(o1->getTypeId()) < mDataProvider.getTypeName(o2->getTypeId());
        });
        return result;
    }

    uint IGBService::getStationIdForFiltering(QxtWebRequestEvent *event) const
    {
        if (!event->cookies.contains(limitToStationsCookie))
            return 0;

        const auto value = event->cookies.values(limitToStationsCookie).first();
        return (value.compare("false", Qt::CaseInsensitive) == 0) ? (0) : (getStationId(event));
    }

    void IGBService::sendEventToIGB(QByteArray message)
    {
        if (mPollingRequests.empty())
        {
            QMessageBox::information(nullptr, tr("Evernus"), tr("You have to open Evernus in EVE In-Game Browser first."));
            return;
        }

        for (const auto event : mPollingRequests)
        {
            auto response = new QxtWebPageEvent(event->sessionID, event->requestID, std::move(message));
            response->contentType = "application/json";

            postEvent(response);
        }

        mPollingRequests.clear();
    }

    Character::IdType IGBService::getCharacterId(QxtWebRequestEvent *event)
    {
        const auto charIdHeader = "EVE_CHARID";

        if (!event->headers.contains(charIdHeader))
            return Character::invalidId;

        return event->headers.values(charIdHeader).first().toULongLong();
    }

    uint IGBService::getCorporationId(QxtWebRequestEvent *event)
    {
        const auto corpIdHeader = "EVE_CORPID";

        if (!event->headers.contains(corpIdHeader))
            return std::numeric_limits<uint>::max();    // 0 fetches orders without a corp

        return event->headers.values(corpIdHeader).first().toUInt();
    }

    uint IGBService::getStationId(QxtWebRequestEvent *event)
    {
        const auto stationIdHeader = "EVE_STATIONID";

        if (!event->headers.contains(stationIdHeader))
            return 0;

        return event->headers.values(stationIdHeader).first().toUInt();
    }
}
