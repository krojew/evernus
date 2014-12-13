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
#include <QDebug>

#include "PriceSettings.h"

#include "FPCController.h"

namespace Evernus
{
    FPCController::FPCController(QObject *parent)
        : QObject{parent}
    {
        handleNewPreferences();

        connect(&mShortcut, &QxtGlobalShortcut::activated, this, &FPCController::trigger);
    }

    void FPCController::handleNewPreferences()
    {
        QSettings settings;

        auto shortcut = QKeySequence::fromString(settings.value(PriceSettings::fpcShourtcutKey).toString());
        if (!shortcut.isEmpty()) {
            mShortcut.setEnabled(settings.value(PriceSettings::fpcKey, PriceSettings::fpcDefault).toBool());
            mShortcut.setShortcut(shortcut);
        }
    }

    void FPCController::changeExecutor(QObject *executor)
    {
        if (executor == mExecutor)
            return;

        qDebug() << "Changing FPC executor to:" << executor;

        if (mExecutor != nullptr)
            disconnect(mExecutor, SLOT(executeFPC()));

        mExecutor = executor;

        if (mExecutor != nullptr)
            connect(this, SIGNAL(execute()), mExecutor, SLOT(executeFPC()));
    }

    void FPCController::trigger()
    {
        qDebug() << "FPC triggered.";
        emit execute();
    }
}
