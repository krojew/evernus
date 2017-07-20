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
#include <QStandardPaths>
#include <QDebug>
#include <QDir>

#include "ChainableFileLogger.h"

namespace Evernus
{
    ChainableFileLogger *ChainableFileLogger::instance = nullptr;

    ChainableFileLogger::ChainableFileLogger()
        : mLogFile{getLogDir() + QStringLiteral("main.log")}
    {
        QDir{}.mkpath(getLogDir());

        if (!mLogFile.open(QIODevice::WriteOnly | QIODevice::Text))
            qWarning() << "Error opening main.log file at:" << getLogDir();
        else
            mPrevHandler = qInstallMessageHandler(&ChainableFileLogger::handleMessage);
    }

    ChainableFileLogger::~ChainableFileLogger()
    {
        qInstallMessageHandler(mPrevHandler);
    }

    void ChainableFileLogger::initialize()
    {
        static ChainableFileLogger instance{};
        ChainableFileLogger::instance = &instance;
    }

    void ChainableFileLogger::writeMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        if (mPrevHandler != nullptr)
            mPrevHandler(type, context, msg);

        const auto log = qFormatLogMessage(type, context, msg);
        if (Q_LIKELY(!log.isEmpty()))
        {
            std::lock_guard<std::mutex> lock{mStreamMutex};

            mStream << log << '\n';
            mStream.flush();
        }
    }

    void ChainableFileLogger::handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        Q_ASSERT(instance != nullptr);
        instance->writeMessage(type, context, msg);
    }

    QString ChainableFileLogger::getLogDir()
    {
        return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/log/");
    }
}
