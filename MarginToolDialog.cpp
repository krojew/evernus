#include <QDialogButtonBox>
#include <QStandardPaths>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QSettings>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QFont>

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

        const auto alwaysOnTop = settings.value(MarginToolSettings::alwaysOnTopKey, true).toBool();

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto infoLayout = new QHBoxLayout{};
        mainLayout->addLayout(infoLayout);

        auto marginGroup = new QGroupBox{this};
        infoLayout->addWidget(marginGroup);

        auto marginLayout = new QGridLayout{};
        marginGroup->setLayout(marginLayout);

        QFont font;
        font.setBold(true);

        marginLayout->addWidget(new QLabel{tr("Margin:")}, 0, 0);

        mMarginLabel = new QLabel{"-", this};
        marginLayout->addWidget(mMarginLabel, 0, 1);
        mMarginLabel->setFont(font);

        marginLayout->addWidget(new QLabel{tr("Markup:")}, 1, 0);

        mMarkupLabel = new QLabel{"-", this};
        marginLayout->addWidget(mMarkupLabel, 1, 1);
        mMarkupLabel->setFont(font);

        auto settingsGroup = new QGroupBox{this};
        infoLayout->addWidget(settingsGroup);

        auto settingsLayout = new QVBoxLayout{};
        settingsGroup->setLayout(settingsLayout);

        auto alwaysOnTopBtn = new QCheckBox{tr("Always on top"), this};
        settingsLayout->addWidget(alwaysOnTopBtn);
        alwaysOnTopBtn->setChecked(alwaysOnTop);
        connect(alwaysOnTopBtn, &QCheckBox::stateChanged, this, &MarginToolDialog::toggleAlwaysOnTop);

        auto buttons = new QDialogButtonBox{QDialogButtonBox::Close, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto logPath = settings.value(PathSettings::marketLogsPath).toString();
        if (logPath.isEmpty())
            logPath = QStandardPaths::locate(QStandardPaths::DocumentsLocation, "EVE/logs/marketlogs", QStandardPaths::LocateDirectory);

        if (!mWatcher.addPath(logPath))
        {
            QMessageBox::warning(this,
                                 tr("Margin tool error"),
                                 tr("Could not start watching market log path. Make sure the path exists (eg. export some logs) and try again."));
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

    void MarginToolDialog::refreshData()
    {

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
}
