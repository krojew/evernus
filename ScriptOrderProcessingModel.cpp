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
#include <QScriptEngine>

#include "EveDataProvider.h"

#include "ScriptOrderProcessingModel.h"

namespace Evernus
{
    ScriptOrderProcessingModel::ScriptOrderProcessingModel(const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , mDataProvider{dataProvider}
    {
    }

    int ScriptOrderProcessingModel::columnCount(const QModelIndex &parent) const
    {
        return (parent.isValid()) ? (0) : (mMaxColumns);
    }

    QVariant ScriptOrderProcessingModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        if (role == Qt::DisplayRole)
            return mData[index.row()].at(index.column());

        return QVariant{};
    }

    int ScriptOrderProcessingModel::rowCount(const QModelIndex &parent) const
    {
        return (parent.isValid()) ? (0) : (static_cast<int>(mData.size()));
    }

    void ScriptOrderProcessingModel::clear()
    {
        beginResetModel();

        mMaxColumns = 0;
        mData.clear();

        endResetModel();
    }

    void ScriptOrderProcessingModel::reset(const MarketOrderRepository::EntityList &orders, const QString &script, Mode mode)
    {
        beginResetModel();

        mMaxColumns = 0;
        mData.clear();

        QScriptEngine engine;
        engine.globalObject().setProperty("getTypeName", engine.newFunction(
            &ScriptOrderProcessingModel::getTypeName, const_cast<EveDataProvider *>(&mDataProvider)));
        engine.globalObject().setProperty("getLocationName", engine.newFunction(
            &ScriptOrderProcessingModel::getLocationName, const_cast<EveDataProvider *>(&mDataProvider)));

        const auto makeOrderObject = [&engine](const auto &order) {
            auto orderObject = engine.newObject();
            orderObject.setProperty("id", QString::number(order->getId()));
            orderObject.setProperty("characterId", QString::number(order->getCharacterId()));
            orderObject.setProperty("stationId", static_cast<quint32>(order->getLocationId()));
            orderObject.setProperty("volumeEntered", order->getVolumeEntered());
            orderObject.setProperty("volumeRemaining", order->getVolumeRemaining());
            orderObject.setProperty("minVolume", order->getMinVolume());
            orderObject.setProperty("delta", order->getDelta());
            orderObject.setProperty("state", static_cast<int>(order->getState()));
            orderObject.setProperty("typeId", order->getTypeId());
            orderObject.setProperty("range", order->getRange());
            orderObject.setProperty("accountKey", order->getAccountKey());
            orderObject.setProperty("duration", order->getDuration());
            orderObject.setProperty("escrow", order->getEscrow());
            orderObject.setProperty("price", order->getPrice());
            orderObject.setProperty("type", static_cast<int>(order->getType()));
            orderObject.setProperty("issued", engine.newDate(order->getIssued()));
            orderObject.setProperty("firstSeen", engine.newDate(order->getFirstSeen()));
            orderObject.setProperty("lastSeen", engine.newDate(order->getLastSeen()));
            orderObject.setProperty("corporationId", order->getCorporationId());

            return orderObject;
        };

        if (mode == Mode::ForEach)
        {
            auto processFunction = engine.evaluate("(function process(order) {\n" + script + "\n})");
            if (engine.hasUncaughtException())
            {
                endResetModel();
                emit error(engine.uncaughtException().toString());
                return;
            }

            for (const auto &order : orders)
            {
                const auto value
                    = processFunction.call(QScriptValue{}, QScriptValueList{} << makeOrderObject(order)).toVariant().toList();
                if (engine.hasUncaughtException())
                {
                    endResetModel();
                    emit error(engine.uncaughtException().toString());
                    return;
                }

                const auto size = value.size();
                if (size > mMaxColumns)
                    mMaxColumns = size;

                mData.emplace_back(value);
            }
        }
        else
        {
            auto processFunction = engine.evaluate("(function process(orders) {\n" + script + "\n})");
            if (engine.hasUncaughtException())
            {
                endResetModel();
                emit error(engine.uncaughtException().toString());
                return;
            }

            auto arguments = engine.newArray(static_cast<uint>(orders.size()));
            for (auto i = 0u; i < orders.size(); ++i)
                arguments.setProperty(i, makeOrderObject(orders[i]));

            const auto value = processFunction.call(QScriptValue{}, QScriptValueList{} << arguments).toVariant().toList();
            if (engine.hasUncaughtException())
            {
                endResetModel();
                emit error(engine.uncaughtException().toString());
                return;
            }

            mMaxColumns = value.size();
            mData.emplace_back(value);
        }

        endResetModel();
    }

    QScriptValue ScriptOrderProcessingModel::getTypeName(QScriptContext *context, QScriptEngine *engine, void *arg)
    {
        if (context->argumentCount() != 1)
            return context->throwError("Missing argument.");

        const auto dataProvider = static_cast<const EveDataProvider *>(arg);
        return dataProvider->getTypeName(context->argument(0).toUInt32());
    }

    QScriptValue ScriptOrderProcessingModel::getLocationName(QScriptContext *context, QScriptEngine *engine, void *arg)
    {
        if (context->argumentCount() != 1)
            return context->throwError("Missing argument.");

        const auto dataProvider = static_cast<const EveDataProvider *>(arg);
        return dataProvider->getLocationName(context->argument(0).toUInt32());
    }
}
