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

    ChainableFileLogger::ChainableFileLogger(const QString &fileName)
        : mLogFile{QStringLiteral("%1/%2").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(fileName)}
    {
        QDir{}.mkpath(QStandardPaths::writableLocation(QStandardPaths::DataLocation));

        if (!mLogFile.open(QIODevice::WriteOnly | QIODevice::Text))
            qWarning() << "Error opening log file:" << fileName;
        else
            mPrevHandler = qInstallMessageHandler(&ChainableFileLogger::handleMessage);
    }

    ChainableFileLogger::~ChainableFileLogger()
    {
        qInstallMessageHandler(mPrevHandler);
    }

    void ChainableFileLogger::initialize()
    {
        static ChainableFileLogger instance{"main.log"};
        ChainableFileLogger::instance = &instance;
    }

    void ChainableFileLogger::writeMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        if (mPrevHandler != nullptr)
            mPrevHandler(type, context, msg);

        const auto log = qFormatLogMessage(type, context, msg);
        if (!log.isEmpty())
        {
            mStream << log;
            mStream.flush();
        }
    }

    void ChainableFileLogger::handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        Q_ASSERT(instance != nullptr);
        instance->writeMessage(type, context, msg);
    }
}
