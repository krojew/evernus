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
#include <QDesktopServices>
#include <QUrl>

#include "EveTypeProvider.h"

#include "LookupActionGroup.h"

namespace Evernus
{
    LookupActionGroup::LookupActionGroup(const EveTypeProvider &typeProvider, QObject *parent)
        : QActionGroup{parent}
        , mTypeProvider{typeProvider}
    {
        auto action = addAction(tr("Lookup item on eve-marketdata.com"));
        connect(action, &QAction::triggered, this, &LookupActionGroup::lookupOnEveMarketdata);

        action = addAction(tr("Lookup item on eve-central.com"));
        connect(action, &QAction::triggered, this, &LookupActionGroup::lookupOnEveCentral);
    }

    void LookupActionGroup::lookupOnEveMarketdata()
    {
        lookupOnWeb(QStringLiteral("http://eve-marketdata.com/price_check.php?type_id=%1"));
    }

    void LookupActionGroup::lookupOnEveCentral()
    {
        lookupOnWeb(QStringLiteral("https://eve-central.com/home/quicklook.html?typeid=%1"));
    }

    void LookupActionGroup::lookupOnWeb(const QString &baseUrl) const
    {
        const auto typeId = mTypeProvider.getTypeId();
        if (typeId != EveType::invalidId)
            QDesktopServices::openUrl(baseUrl.arg(typeId));
    }
}
