#pragma once

#include <QNetworkAccessManager>
#include <QSaveFile>

namespace Evernus
{
    class FileDownload
        : public QObject
    {
        Q_OBJECT

    public:
        FileDownload(const QUrl &addr, const QString &dest, QObject *parent = nullptr);
        virtual ~FileDownload() = default;

    signals:
        void finished();

    private slots:
        void process();
        void finish();

    private:
        QNetworkAccessManager mNetworkManager;
        QSaveFile mOutput;
    };
}
