#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSettings>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>

#include "PathSettings.h"

#include "PathPreferencesWidget.h"

namespace Evernus
{
    PathPreferencesWidget::PathPreferencesWidget(QWidget *parent)
        : QWidget{parent}
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto marketLogGroup = new QGroupBox{tr("Market logs path"), this};
        mainLayout->addWidget(marketLogGroup);

        auto marketLogGroupLayout = new QVBoxLayout{};
        marketLogGroup->setLayout(marketLogGroupLayout);

        auto infoLabel = new QLabel{tr(
            "You can specify custom market logs path or leave empty to use the default one. Custom path is required on *nix systems."), this};
        infoLabel->setWordWrap(true);

        marketLogGroupLayout->addWidget(infoLabel);

        auto inputLayout = new QHBoxLayout{};
        marketLogGroupLayout->addLayout(inputLayout);

        mMarketLogPathEdit = new QLineEdit{settings.value(PathSettings::marketLogsPath).toString(), this};
        inputLayout->addWidget(mMarketLogPathEdit);

        auto browseBtn = new QPushButton{tr("Browse..."), this};
        inputLayout->addWidget(browseBtn);
        connect(browseBtn, &QPushButton::clicked, this, &PathPreferencesWidget::browseForFolder);

        mainLayout->addStretch();
    }

    void PathPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(PathSettings::marketLogsPath, mMarketLogPathEdit->text());
    }

    void PathPreferencesWidget::browseForFolder()
    {
        const auto path = QFileDialog::getExistingDirectory(this);
        if (!path.isEmpty())
            mMarketLogPathEdit->setText(path);
    }
}
