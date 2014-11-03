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

#include "CommonScriptAPI.h"

namespace Evernus
{
    namespace CommonScriptAPI
    {
        void insertAPI(QScriptEngine &engine, const EveDataProvider &dataProvider)
        {
            engine.globalObject().setProperty("getTypeName", engine.newFunction(
                [](QScriptContext *context, QScriptEngine *engine, void *arg) -> QScriptValue {
                Q_UNUSED(engine);

                if (context->argumentCount() != 1)
                    return context->throwError("Missing argument.");

                const auto dataProvider = static_cast<const EveDataProvider *>(arg);
                return dataProvider->getTypeName(context->argument(0).toUInt32());
            }, const_cast<EveDataProvider *>(&dataProvider)));
           engine.globalObject().setProperty("getLocationName", engine.newFunction(
               [](QScriptContext *context, QScriptEngine *engine, void *arg) -> QScriptValue {
               Q_UNUSED(engine);

               if (context->argumentCount() != 1)
                   return context->throwError("Missing argument.");

               const auto dataProvider = static_cast<const EveDataProvider *>(arg);
               return dataProvider->getLocationName(context->argument(0).toUInt32());
           }, const_cast<EveDataProvider *>(&dataProvider)));
        }
    }
}
