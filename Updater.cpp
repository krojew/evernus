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
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDomDocument>
#include <QMessageBox>
#include <QSettings>
#include <QDebug>
#include <QUrl>

#include "EvernusApplication.h"
#include "PriceSettings.h"

#include "Updater.h"

namespace Evernus
{
    void Updater::performVersionMigration() const
    {
        QSettings settings;

        const auto curVersion
            = settings.value(EvernusApplication::versionKey, QCoreApplication::applicationVersion()).toString().split('.');
        if (curVersion.size() != 2)
            return;

        const auto majorVersion = curVersion[0].toUInt();
        const auto minorVersion = curVersion[1].toUInt();

        if (majorVersion == 0 && minorVersion < 3)
            settings.setValue(PriceSettings::autoAddCustomItemCostKey, false);
    }

    Updater &Updater::getInstance()
    {
        static Updater updater;
        return updater;
    }

    void Updater::checkForUpdates(bool quiet) const
    {
        if (mCheckingForUpdates)
            return;

        qDebug() << "Checking for updates...";

        mCheckingForUpdates = true;

        auto reply = mAccessManager.get(QNetworkRequest{QUrl{"http://evernus.com/latest_version.xml"}});
        connect(reply, &QNetworkReply::finished, this, [quiet, this]{
            finishCheck(quiet);
        });
    }

    void Updater::finishCheck(bool quiet) const
    {
        mCheckingForUpdates = false;

        auto reply = static_cast<QNetworkReply *>(sender());
        reply->deleteLater();

        const auto error = reply->error();
        if (error != QNetworkReply::NoError)
        {
            if (!quiet)
                QMessageBox::warning(nullptr, tr("Error"), tr("Error contacting update server: %1").arg(error));

            return;
        }

        QString docError;

        QDomDocument doc;
        if (!doc.setContent(reply, false, &docError))
        {
            if (!quiet)
                QMessageBox::warning(nullptr, tr("Error"), tr("Error parsing response from the update server: %1").arg(docError));

            return;
        }

        const auto nextVersionString = doc.documentElement().attribute("id");
        const auto nextVersion = nextVersionString.split('.');
        if (nextVersion.count() != 2)
        {
            if (!quiet)
                QMessageBox::warning(nullptr, tr("Error"), tr("Missing update version information!"));

            return;
        }

        const auto curVersion = QCoreApplication::applicationVersion().split('.');

        const auto nextMajor = nextVersion[0].toUInt();
        const auto curMajor = curVersion[0].toUInt();

        if ((nextMajor < curMajor) || (nextMajor == curMajor && nextVersion[1].toUInt() <= curVersion[1].toUInt()))
        {
            if (!quiet)
                QMessageBox::information(nullptr, tr("No update found"), tr("Your current version is up-to-date."));

            return;
        }

        const auto ret = QMessageBox::question(
            nullptr, tr("Update found"), tr("A new version is available: %1\nDo you wish to download it now?").arg(nextVersionString));
        if (ret == QMessageBox::Yes)
            QDesktopServices::openUrl(QUrl{"http://evernus.com/download"});
    }
}
