#pragma once

#include <unordered_map>

#include <QDialog>

class QTreeWidgetItem;
class QProgressBar;
class QTreeWidget;
class QCheckBox;

namespace Evernus
{
    class ActiveTasksDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit ActiveTasksDialog(QWidget *parent = nullptr);
        virtual ~ActiveTasksDialog() = default;

    public slots:
        void addNewTaskInfo(uint taskId, const QString &description);
        void addNewSubTaskInfo(uint taskId, uint parentTask, const QString &description);
        void setTaskStatus(uint taskId, const QString &error);

    private slots:
        void autoCloseSave(bool enabled);

    private:
        struct SubTaskInfo
        {
            size_t mCount = 0;
            bool mError = false;
        };

        QTreeWidget *mTaskWidget = nullptr;
        QProgressBar *mTotalProgressWidget = nullptr;
        QCheckBox *mAutoCloseBtn = nullptr;

        std::unordered_map<uint, QTreeWidgetItem *> mTaskItems;
        std::unordered_map<uint, SubTaskInfo> mSubTaskInfo;

        bool mHadError = false;

        void fillTaskItem(uint taskId, QTreeWidgetItem *item, const QString &description);
    };
}
