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

#include <vector>
#include <memory>

#include "MarketOrder.h"

#include "qxtwebslotservice.h"
#include "qxthtmltemplate.h"

#include "Character.h"

class QxtHttpSessionManager;

namespace Evernus
{
    class MarketOrderProvider;
    class EveDataProvider;

    class IGBService
        : public QxtWebSlotService
    {
        Q_OBJECT

    public:
        IGBService(const MarketOrderProvider &orderProvider,
                   const EveDataProvider &dataProvider,
                   QxtHttpSessionManager *sm,
                   QObject *parent = nullptr);
        virtual ~IGBService() = default;

    signals:
        void openMarginTool();

    public slots:
        void index(QxtWebRequestEvent *event);
        void active(QxtWebRequestEvent *event);
        void fulfilled(QxtWebRequestEvent *event);
        void openMarginTool(QxtWebRequestEvent *event);

    private:
        const MarketOrderProvider &mOrderProvider;
        const EveDataProvider &mDataProvider;

        QxtHtmlTemplate mMainTemplate, mOrderTemplate;

        void renderContent(QxtWebRequestEvent *event, const QString &content);
        QString renderOrderList(const std::vector<std::shared_ptr<MarketOrder>> &orders,
                                QStringList &idContainer,
                                QStringList &typeIdContainer) const;

        std::vector<std::shared_ptr<MarketOrder>> filterAndSort(const std::vector<std::shared_ptr<MarketOrder>> &orders,
                                                                MarketOrder::State state,
                                                                bool needsDelta) const;

        static Character::IdType getCharacterId(QxtWebRequestEvent *event);
    };
}