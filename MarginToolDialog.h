#pragma once

#include <QFileSystemWatcher>
#include <QDialog>
#include <QSet>

class QLabel;

namespace Evernus
{
    class MarginToolDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit MarginToolDialog(QWidget *parent = nullptr);
        virtual ~MarginToolDialog() = default;

    private slots:
        void toggleAlwaysOnTop(int state);

        void refreshData(const QString &path);

    private:
        QFileSystemWatcher mWatcher;

        QLabel *mNameLabel = nullptr;
        QLabel *mBestBuyLabel = nullptr;
        QLabel *mBestSellLabel = nullptr;
        QLabel *mMarginLabel = nullptr;
        QLabel *mMarkupLabel = nullptr;

        QSet<QString> mKnownFiles;

        void setNewWindowFlags(bool alwaysOnTop);

        static QSet<QString> getKnownFiles(const QString &path);
    };
}
