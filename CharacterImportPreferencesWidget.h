#pragma once

#include <QWidget>

class QCheckBox;

namespace Evernus
{
    class CharacterImportPreferencesWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit CharacterImportPreferencesWidget(QWidget *parent = nullptr);
        virtual ~CharacterImportPreferencesWidget() = default;

    public slots:
        void applySettings();

    private:
        QCheckBox *mImportSkillsBox = nullptr;
        QCheckBox *mImportPortraitBox = nullptr;
    };
}
