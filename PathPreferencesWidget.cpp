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
#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QSettings>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>

#include "PathSettings.h"

#include "PathPreferencesWidget.h"

namespace Evernus
{
    PathPreferencesWidget::PathPreferencesWidget(QWidget *parent)
        : QWidget(parent)
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto marketLogGroup = new QGroupBox{tr("Market logs path"), this};
        mainLayout->addWidget(marketLogGroup);

        auto marketLogGroupLayout = new QFormLayout{};
        marketLogGroup->setLayout(marketLogGroupLayout);

        auto infoLabel = new QLabel{tr(
            "You can specify custom market logs path or leave empty to use the default one. Custom path is required on *nix systems."), this};
        infoLabel->setWordWrap(true);

        marketLogGroupLayout->addRow(infoLabel);

        auto inputLayout = new QHBoxLayout{};
        marketLogGroupLayout->addRow(inputLayout);

        mMarketLogPathEdit = new QLineEdit{settings.value(PathSettings::marketLogsPathKey).toString(), this};
        inputLayout->addWidget(mMarketLogPathEdit);

        auto browseBtn = new QPushButton{tr("Browse..."), this};
        inputLayout->addWidget(browseBtn);
        connect(browseBtn, &QPushButton::clicked, this, &PathPreferencesWidget::browseForMarketLogsFolder);

        mDeleteLogsBtn = new QCheckBox{tr("Delete parsed logs"), this};
        marketLogGroupLayout->addRow(mDeleteLogsBtn);
        mDeleteLogsBtn->setChecked(settings.value(PathSettings::deleteLogsKey, true).toBool());

        mCharacterLogWildcardEdit = new QLineEdit{
            settings.value(PathSettings::characterLogWildcardKey, PathSettings::characterLogWildcardDefault).toString(), this};
        marketLogGroupLayout->addRow(tr("Character log file name wildcard:"), mCharacterLogWildcardEdit);

        mCorporationLogWildcardEdit = new QLineEdit{
            settings.value(PathSettings::corporationLogWildcardKey, PathSettings::corporationLogWildcardDefault).toString(), this};
        marketLogGroupLayout->addRow(tr("Corporation log file name wildcard:"), mCorporationLogWildcardEdit);

#ifdef Q_OS_WIN
        auto eveGroup = new QGroupBox{tr("Eve path"), this};
        mainLayout->addWidget(eveGroup);

        auto eveLayout = new QHBoxLayout{};
        eveGroup->setLayout(eveLayout);

        mEvePathEdit = new QLineEdit{settings.value(PathSettings::evePathKey).toString(), this};
        eveLayout->addWidget(mEvePathEdit);

        browseBtn = new QPushButton{tr("Browse..."), this};
        eveLayout->addWidget(browseBtn);
        connect(browseBtn, &QPushButton::clicked, this, &PathPreferencesWidget::browseForEveFolder);
#else
        auto eveGroup = new QGroupBox{tr("Eve cache path"), this};
        mainLayout->addWidget(eveGroup);

        auto eveLayout = new QHBoxLayout{};
        eveGroup->setLayout(eveLayout);

        mEveCachePathEdit = new QLineEdit{settings.value(PathSettings::eveCachePathKey).toString(), this};
        eveLayout->addWidget(mEveCachePathEdit);

        browseBtn = new QPushButton{tr("Browse..."), this};
        eveLayout->addWidget(browseBtn);
        connect(browseBtn, &QPushButton::clicked, this, &PathPreferencesWidget::browseForEveFolder);
#endif

        mainLayout->addStretch();
    }

    void PathPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(PathSettings::marketLogsPathKey, mMarketLogPathEdit->text());
        settings.setValue(PathSettings::deleteLogsKey, mDeleteLogsBtn->isChecked());
        settings.setValue(PathSettings::characterLogWildcardKey, mCharacterLogWildcardEdit->text());
        settings.setValue(PathSettings::corporationLogWildcardKey, mCorporationLogWildcardEdit->text());
#ifdef Q_OS_WIN
        settings.setValue(PathSettings::evePathKey, mEvePathEdit->text());
#else
        settings.setValue(PathSettings::eveCachePathKey, mEveCachePathEdit->text());
#endif
    }

    void PathPreferencesWidget::browseForMarketLogsFolder()
    {
        const auto path = QFileDialog::getExistingDirectory(this);
        if (!path.isEmpty())
            mMarketLogPathEdit->setText(path);
    }

    void PathPreferencesWidget::browseForEveFolder()
    {
        const auto path = QFileDialog::getExistingDirectory(this);
        if (!path.isEmpty())
#ifdef Q_OS_WIN
            mEvePathEdit->setText(path);
#else
            mEveCachePathEdit->setText(path);
#endif
    }
}
