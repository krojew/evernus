#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QSettings>

#include "ImportSettings.h"

#include "AssetsImportPreferencesWidget.h"

namespace Evernus
{
    AssetsImportPreferencesWidget::AssetsImportPreferencesWidget(QWidget *parent)
        : QWidget{parent}
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto importBox = new QGroupBox{tr("Assets import"), this};
        mainLayout->addWidget(importBox);

        auto importBoxLayout = new QVBoxLayout{};
        importBox->setLayout(importBoxLayout);

        mImportAssetsBox = new QCheckBox{tr("Import assets"), this};
        importBoxLayout->addWidget(mImportAssetsBox);
        mImportAssetsBox->setChecked(settings.value(ImportSettings::importAssetsKey, true).toBool());

        mainLayout->addStretch();
    }

    void AssetsImportPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(ImportSettings::importAssetsKey, mImportAssetsBox->isChecked());
    }
}
