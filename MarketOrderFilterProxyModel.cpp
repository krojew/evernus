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
#include <QSettings>
#include <QTimer>

#include "MarketOrderModel.h"
#include "EveDataProvider.h"
#include "PriceSettings.h"
#include "ExternalOrder.h"
#include "MarketOrder.h"
#include "ScriptUtils.h"
#include "UISettings.h"

#include "MarketOrderFilterProxyModel.h"

namespace Evernus
{
    const MarketOrderFilterProxyModel::StatusFilters MarketOrderFilterProxyModel::defaultStatusFilter = Changed | Active;
    const MarketOrderFilterProxyModel::PriceStatusFilters MarketOrderFilterProxyModel::defaultPriceStatusFilter = EveryPriceStatus;

    MarketOrderFilterProxyModel::MarketOrderFilterProxyModel(const EveDataProvider &dataProvider, QObject *parent)
        : LeafFilterProxyModel{parent}
        , mDataProvider{dataProvider}
    {
        QSettings settings;
        mStatusFilter = static_cast<StatusFilters>(
            settings.value(UISettings::marketOrderStateFilterKey, static_cast<int>(defaultStatusFilter)).toInt());
        mPriceStatusFilter = static_cast<PriceStatusFilters>(
            settings.value(UISettings::marketOrderPriceStatusFilterKey, static_cast<int>(defaultPriceStatusFilter)).toInt());
    }

    void MarketOrderFilterProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
    {
        Q_ASSERT(sourceModel == nullptr || dynamic_cast<MarketOrderModel *>(sourceModel) != nullptr);
        LeafFilterProxyModel::setSourceModel(sourceModel);
    }

    MarketOrderFilterProxyModel::StatusFilters MarketOrderFilterProxyModel::getStatusFilter() const
    {
        return mStatusFilter;
    }

    MarketOrderFilterProxyModel::PriceStatusFilters MarketOrderFilterProxyModel::getPriceStatusFilter() const
    {
        return mPriceStatusFilter;
    }

    void MarketOrderFilterProxyModel::setStatusFilter(const StatusFilters &filter)
    {
        mStatusFilter = filter;
        invalidateFilter();
    }

    void MarketOrderFilterProxyModel::setPriceStatusFilter(const PriceStatusFilters &filter)
    {
        mPriceStatusFilter = filter;
        invalidateFilter();
    }

    void MarketOrderFilterProxyModel::setTextFilter(const QString &text, bool script)
    {
        if (script)
        {
            mEngine.clearExceptions();
            mFilterFunction = mEngine.evaluate("(function process(order) {\nreturn " + text + ";\n})");

            if (mEngine.hasUncaughtException())
                emit scriptError(mEngine.uncaughtException().toString());

            setFilterWildcard(QString{});
        }
        else
        {
            mFilterFunction = QScriptValue{};
            setFilterWildcard(text);
        }
    }

    void MarketOrderFilterProxyModel::unscheduleScriptError()
    {
        const auto error = mEngine.uncaughtException().toString();

        mScriptErrorScheduled = false;
        mEngine.clearExceptions();

        emit scriptError(error);
    }

    bool MarketOrderFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        const auto parentResult = LeafFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
        if (!parentResult)
            return false;

        const auto index = sourceModel()->index(sourceRow, 0, sourceParent);
        const auto order = static_cast<MarketOrderModel *>(sourceModel())->getOrder(index);

        if (order == nullptr)
            return hasAcceptedChildren(sourceRow, sourceParent);

        return acceptsStatus(*order) && acceptsPriceStatus(*order) && acceptsByScript(*order);
    }

    bool MarketOrderFilterProxyModel::acceptsStatus(const MarketOrder &order) const
    {
        const auto type = static_cast<MarketOrderModel *>(sourceModel())->getType();
        if (type == MarketOrderModel::Type::Neither)
            return true;

        if ((mStatusFilter & Changed) && (order.getDelta() != 0))
            return true;
        if ((mStatusFilter & Active) && (order.getState() == MarketOrder::State::Active))
            return true;
        if ((mStatusFilter & Fulfilled) && (order.getState() == MarketOrder::State::Fulfilled) && (order.getVolumeRemaining() == 0))
            return true;
        if ((mStatusFilter & Cancelled) && (order.getState() == MarketOrder::State::Cancelled))
            return true;
        if ((mStatusFilter & Pending) && (order.getState() == MarketOrder::State::Pending))
            return true;
        if ((mStatusFilter & CharacterDeleted) && (order.getState() == MarketOrder::State::CharacterDeleted))
            return true;
        if ((mStatusFilter & Expired) && (order.getState() == MarketOrder::State::Fulfilled) && (order.getVolumeRemaining() != 0))
            return true;

        return false;
    }

    bool MarketOrderFilterProxyModel::acceptsPriceStatus(const MarketOrder &order) const
    {
        if (mPriceStatusFilter == EveryPriceStatus)
            return true;

        const auto type = static_cast<MarketOrderModel *>(sourceModel())->getType();
        if (type == MarketOrderModel::Type::Neither)
            return true;

        const auto price = (type == MarketOrderModel::Type::Buy) ?
                           (mDataProvider.getTypeBuyPrice(order.getTypeId(), order.getStationId(), order.getRange())) :
                           (mDataProvider.getTypeSellPrice(order.getTypeId(), order.getStationId()));

        if (price->isNew())
            return mPriceStatusFilter & NoData;

        QSettings settings;
        const auto maxAge = settings.value(PriceSettings::priceMaxAgeKey, PriceSettings::priceMaxAgeDefault).toInt();
        const auto tooOld = price->getUpdateTime() < QDateTime::currentDateTimeUtc().addSecs(-3600 * maxAge);

        if ((mPriceStatusFilter & DataTooOld) && (tooOld))
            return true;
        if ((mPriceStatusFilter & Ok) && (!tooOld) && (!price->isNew()))
            return true;

        return false;
    }

    bool MarketOrderFilterProxyModel::acceptsByScript(const MarketOrder &order) const
    {
        if (!mFilterFunction.isValid())
            return true;

        auto scriptOrder = ScriptUtils::wrapMarketOrder(mEngine, order);

        std::shared_ptr<ExternalOrder> overbidOrder;

        const auto type = static_cast<MarketOrderModel *>(sourceModel())->getType();
        switch (type) {
        case MarketOrderModel::Type::Buy:
            overbidOrder = mDataProvider.getTypeBuyPrice(order.getTypeId(), order.getStationId(), order.getRange());
            break;
        case MarketOrderModel::Type::Sell:
            overbidOrder = mDataProvider.getTypeSellPrice(order.getTypeId(), order.getStationId());
            break;
        default:
            break;
        }

        if (overbidOrder)
        {
            const auto diff = order.getPrice() - overbidOrder->getPrice();
            auto overbidObj = mEngine.newObject();

            overbidObj.setPrototype(ScriptUtils::wrapExternalOrder(mEngine, *overbidOrder));
            overbidObj.setProperty("diff", diff);
            overbidObj.setProperty("diffRatio", diff / order.getPrice());

            scriptOrder.setProperty("overbid", overbidObj);
        }

        auto result = mFilterFunction.call(QScriptValue{}, QScriptValueList{} << scriptOrder).toBool();

        if (mEngine.hasUncaughtException() && !mScriptErrorScheduled)
        {
            mScriptErrorScheduled = true;
            QTimer::singleShot(0, this, SLOT(unscheduleScriptError()));
        }

        return result;
    }
}
