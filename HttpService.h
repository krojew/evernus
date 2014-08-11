/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Http Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Http Public License for more details.
 *
 *  You should have received a copy of the GNU Http Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "SimpleCrypt.h"

#include "qxtwebslotservice.h"
#include "qxthtmltemplate.h"

class QxtHttpSessionManager;

namespace Evernus
{
    class CharacterRepository;
    class MarketOrderProvider;
    class EveDataProvider;

    class HttpService
        : public QxtWebSlotService
    {
        Q_OBJECT

    public:
        HttpService(const MarketOrderProvider &orderProvider,
                    const MarketOrderProvider &corpOrderProvider,
                    const EveDataProvider &dataProvider,
                    const CharacterRepository &characterRepo,
                    QxtHttpSessionManager *sm,
                    QObject *parent = nullptr);
        virtual ~HttpService() = default;

    public slots:
        void index(QxtWebRequestEvent *event);
        void characterOrders(QxtWebRequestEvent *event);

    protected:
        virtual void pageRequestedEvent(QxtWebRequestEvent *event) override;

    private:
        static const QString characterIdName;

        const MarketOrderProvider &mOrderProvider, &mCorpOrderProvider;
        const EveDataProvider &mDataProvider;
        const CharacterRepository &mCharacterRepo;

        SimpleCrypt mCrypt;

        QxtHtmlTemplate mMainTemplate, mIndexTemplate;

        void renderContent(QxtWebRequestEvent *event, const QString &content);
        void postUnauthorized(QxtWebRequestEvent *event);

        static bool isIndexAction(QxtWebRequestEvent *event);
    };
}
