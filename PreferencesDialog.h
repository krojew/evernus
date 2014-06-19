#pragma once

#include <QDialog>

class QTreeWidgetItem;
class QStackedWidget;

namespace Evernus
{
    class PreferencesDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit PreferencesDialog(QWidget *parent = nullptr);
        virtual ~PreferencesDialog() = default;

        virtual void accept() override;

    signals:
        void settingsInvalidated();

    private slots:
        void setCurrentPage(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    private:
        QStackedWidget *mPreferencesStack = nullptr;
    };
}
