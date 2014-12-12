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
#include <QNetworkCookie>
#include <QSettings>

#include "PersistentCookieJar.h"

namespace Evernus
{
    PersistentCookieJar::PersistentCookieJar(QString configKey, QObject *parent)
        : QNetworkCookieJar{parent}
        , mConfigKey{std::move(configKey)}
    {
        QList<QNetworkCookie> cookies;
        QSettings settings;

        const auto savedCookies = settings.value(mConfigKey).toList();
        for (const auto &cookie : savedCookies)
            cookies << QNetworkCookie::parseCookies(cookie.toByteArray());

        setAllCookies(cookies);
    }

    PersistentCookieJar::~PersistentCookieJar()
    {
        try
        {
            QVariantList cookies;
            QListIterator<QNetworkCookie> it{allCookies()};
            while (it.hasNext())
            {
                const auto &cookie = it.next();
                if (!cookie.isSessionCookie())
                    cookies << cookie.toRawForm();
            }

            QSettings settings;
            settings.setValue(mConfigKey, cookies);
        }
        catch (...)
        {
        }
    }
}
