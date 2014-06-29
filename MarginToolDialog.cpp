#include <functional>
#include <set>

#include <QDialogButtonBox>
#include <QStandardPaths>
#include <QTextStream>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSettings>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QFont>
#include <QFile>
#include <QDir>

#ifdef Q_OS_WIN
// see setNewWindowFlags()
#   include <windows.h>
#endif

#include "MarginToolSettings.h"
#include "PathSettings.h"

#include "MarginToolDialog.h"

namespace Evernus
{
    MarginToolDialog::MarginToolDialog(QWidget *parent)
        : QDialog{parent}
    {
        QSettings settings;

        QFont font;
        font.setBold(true);
        font.setPointSize(18);

        const auto alwaysOnTop = settings.value(MarginToolSettings::alwaysOnTopKey, true).toBool();

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mNameLabel = new QLabel{this};
        mainLayout->addWidget(mNameLabel);
        mNameLabel->setFont(font);

        auto infoLayout = new QHBoxLayout{};
        mainLayout->addLayout(infoLayout);

        auto marginGroup = new QGroupBox{this};
        infoLayout->addWidget(marginGroup);

        auto marginLayout = new QGridLayout{};
        marginGroup->setLayout(marginLayout);

        marginLayout->addWidget(new QLabel{tr("Margin:")}, 0, 0);

        mMarginLabel = new QLabel{"-", this};
        marginLayout->addWidget(mMarginLabel, 0, 1);
        mMarginLabel->setFont(font);

        marginLayout->addWidget(new QLabel{tr("Markup:")}, 1, 0);

        mMarkupLabel = new QLabel{"-", this};
        marginLayout->addWidget(mMarkupLabel, 1, 1);
        mMarkupLabel->setFont(font);

        auto priceGroup = new QGroupBox{this};
        infoLayout->addWidget(priceGroup);

        auto priceLayout = new QGridLayout{};
        priceGroup->setLayout(priceLayout);

        priceLayout->addWidget(new QLabel{tr("Max buy:")}, 0, 0);
        priceLayout->addWidget(new QLabel{tr("Min sell:")}, 1, 0);

        mBestBuyLabel = new QLabel{"-", this};
        priceLayout->addWidget(mBestBuyLabel, 0, 1);

        mBestSellLabel = new QLabel{"-", this};
        priceLayout->addWidget(mBestSellLabel, 1, 1);

        auto alwaysOnTopBtn = new QCheckBox{tr("Always on top"), this};
        mainLayout->addWidget(alwaysOnTopBtn);
        alwaysOnTopBtn->setChecked(alwaysOnTop);
        connect(alwaysOnTopBtn, &QCheckBox::stateChanged, this, &MarginToolDialog::toggleAlwaysOnTop);

        auto buttons = new QDialogButtonBox{QDialogButtonBox::Close, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto logPath = settings.value(PathSettings::marketLogsPath).toString();
        if (logPath.isEmpty())
            logPath = QStandardPaths::locate(QStandardPaths::DocumentsLocation, "EVE/logs/Marketlogs", QStandardPaths::LocateDirectory);

        if (!mWatcher.addPath(logPath))
        {
            QMessageBox::warning(this,
                                 tr("Margin tool error"),
                                 tr("Could not start watching market log path. Make sure the path exists (eg. export some logs) and try again."));
        }
        else
        {
            mKnownFiles = getKnownFiles(logPath);
        }

        connect(&mWatcher, &QFileSystemWatcher::directoryChanged, this, &MarginToolDialog::refreshData);

        setWindowTitle(tr("Margin tool"));
        setAttribute(Qt::WA_DeleteOnClose);
        setNewWindowFlags(alwaysOnTop);
    }

    void MarginToolDialog::toggleAlwaysOnTop(int state)
    {
        const auto alwaysOnTop = state == Qt::Checked;

        QSettings settings;
        settings.setValue(MarginToolSettings::alwaysOnTopKey, alwaysOnTop);

        setNewWindowFlags(alwaysOnTop);
    }

    void MarginToolDialog::refreshData(const QString &path)
    {
        const auto newFiles = getKnownFiles(path);

        auto fileDiff = newFiles;
        fileDiff -= mKnownFiles;

        if (!fileDiff.isEmpty())
        {
            const auto firstFile = *fileDiff.cbegin();

            QFile file{firstFile};
            if (file.open(QIODevice::ReadOnly))
            {
                std::multiset<double> buy;
                std::multiset<double, std::greater<double>> sell;

                QTextStream stream{&file};
                while (!stream.atEnd())
                {
                    const auto values = stream.readLine().split(',');
                    if (values.count() >= 14)
                    {
                        if (values[13] != "0")
                            continue;

                        if (values[7] == "True")
                            buy.emplace(values[0].toDouble());
                        else if (values[7] == "False")
                            sell.emplace(values[0].toDouble());
                    }
                }

                mBestBuyLabel->setText((buy.empty()) ? ("-") : (QString::number(*std::begin(buy))));
                mBestSellLabel->setText((sell.empty()) ? ("-") : (QString::number(*std::begin(sell))));
            }

            mKnownFiles = std::move(fileDiff);
        }
    }

    void MarginToolDialog::setNewWindowFlags(bool alwaysOnTop)
    {
#ifdef Q_OS_WIN
        // https://bugreports.qt-project.org/browse/QTBUG-30359
        if (alwaysOnTop)
            SetWindowPos(reinterpret_cast<HWND>(winId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        else
            SetWindowPos(reinterpret_cast<HWND>(winId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#else
        auto flags = windowFlags();
        if (alwaysOnTop)
            flags |= Qt::WindowStaysOnTopHint;
        else
            flags &= ~Qt::WindowStaysOnTopHint;

        setWindowFlags(flags);
        show();
#endif
    }

    QSet<QString> MarginToolDialog::getKnownFiles(const QString &path)
    {
        return QSet<QString>::fromList(QDir{path}.entryList(QStringList{"*.txt"}, QDir::Files | QDir::Readable));
    }
}
