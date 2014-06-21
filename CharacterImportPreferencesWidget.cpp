#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QSettings>

#include "ImportSettings.h"

#include "CharacterImportPreferencesWidget.h"

namespace Evernus
{
    CharacterImportPreferencesWidget::CharacterImportPreferencesWidget(QWidget *parent)
        : QWidget{parent}
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto importBox = new QGroupBox{tr("Character import"), this};
        mainLayout->addWidget(importBox);

        auto importBoxLayout = new QVBoxLayout{};
        importBox->setLayout(importBoxLayout);

        mImportSkillsBox = new QCheckBox{tr("Import skills"), this};
        importBoxLayout->addWidget(mImportSkillsBox);
        mImportSkillsBox->setChecked(settings.value(ImportSettings::importSkillsKey, true).toBool());

        mainLayout->addStretch();
    }

    void CharacterImportPreferencesWidget::applySettings()
    {
        QSettings settings;
        settings.setValue(ImportSettings::importSkillsKey, mImportSkillsBox->isChecked());
    }
}
