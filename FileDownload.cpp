#include <stdexcept>

#include <QNetworkReply>
#include <QFileInfo>
#include <QDir>

#include "FileDownload.h"

namespace Evernus
{
    FileDownload::FileDownload(const QUrl &addr, const QString &dest, QObject *parent)
        : QObject{parent}
        , mOutput{dest}
    {
        QFileInfo info{dest};
        QDir{}.mkpath(info.dir().path());

        if (!mOutput.open(QIODevice::WriteOnly | QIODevice::Truncate))
            throw std::runtime_error(QString{tr("Error creating file: %1")}.arg(dest).toStdString());

        auto reply = mNetworkManager.get(QNetworkRequest{addr});
        connect(reply, &QNetworkReply::readyRead, this, &FileDownload::process);
        connect(reply, &QNetworkReply::finished, this, &FileDownload::finish);
    }

    void FileDownload::process()
    {
        auto reply = static_cast<QNetworkReply *>(sender());
        mOutput.write(reply->readAll());
    }

    void FileDownload::finish()
    {
        sender()->deleteLater();
        mOutput.commit();

        emit finished();
    }
}
