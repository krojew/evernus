#include <QDialogButtonBox>
#include <QProgressBar>
#include <QTreeWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSettings>

#include "UISettings.h"

#include "ActiveTasksDialog.h"

namespace Evernus
{
    ActiveTasksDialog::ActiveTasksDialog(QWidget *parent)
        : QDialog{parent}
    {
        QSettings settings;

        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        mTaskWidget = new QTreeWidget{this};
        mainLayout->addWidget(mTaskWidget, 1);
        mTaskWidget->setHeaderHidden(true);
        mTaskWidget->setColumnCount(2);

        auto progressLayout = new QHBoxLayout{};
        mainLayout->addLayout(progressLayout);

        mAutoCloseBtn = new QCheckBox{tr("Close automatically"), this};
        progressLayout->addWidget(mAutoCloseBtn);
        mAutoCloseBtn->setChecked(settings.value(UISettings::autoCloseTasksKey).toBool());
        connect(mAutoCloseBtn, &QCheckBox::toggled, this, &ActiveTasksDialog::autoCloseSave);

        mTotalProgressWidget = new QProgressBar{this};
        progressLayout->addWidget(mTotalProgressWidget);
        mTotalProgressWidget->setMinimumWidth(500);
        mTotalProgressWidget->setValue(0);
        mTotalProgressWidget->setMaximum(0);

        auto btnBox = new QDialogButtonBox{QDialogButtonBox::Close, this};
        mainLayout->addWidget(btnBox);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setWindowTitle(tr("Active Tasks"));
    }

    void ActiveTasksDialog::addNewTaskInfo(quint32 taskId, const QString &description)
    {
        if (mTaskItems.empty())
        {
            mHadError = false;
            mTotalProgressWidget->setValue(0);
            mTotalProgressWidget->setMaximum(1);
        }
        else
        {
            mTotalProgressWidget->setMaximum(mTotalProgressWidget->maximum() + 1);
        }

        fillTaskItem(taskId, new QTreeWidgetItem{mTaskWidget}, description);
        mTaskWidget->resizeColumnToContents(0);
    }

    void ActiveTasksDialog::addNewSubTaskInfo(quint32 taskId, quint32 parentTask, const QString &description)
    {
        mTotalProgressWidget->setMaximum(mTotalProgressWidget->maximum() + 1);

        auto item = mTaskItems.find(parentTask);
        Q_ASSERT(item != std::end(mTaskItems));

        fillTaskItem(taskId, new QTreeWidgetItem{item->second}, description);
        mTaskWidget->resizeColumnToContents(0);

        ++mSubTaskCount[parentTask];
    }

    void ActiveTasksDialog::setTaskStatus(quint32 taskId, bool success)
    {
        auto item = mTaskItems.find(taskId);
        Q_ASSERT(item != std::end(mTaskItems));

        mHadError = mHadError && !success;

        const auto parent = item->second->parent();

        item->second->setIcon(0, QIcon{(success) ? (":/images/accept.png") : (":/images/exclamation.png")});

        mTaskItems.erase(item);
        mTotalProgressWidget->setValue(mTotalProgressWidget->value() + 1);

        if (parent != nullptr)
        {
            const auto it = mSubTaskCount.find(parent->data(0, Qt::UserRole).toUInt());
            Q_ASSERT(it != std::end(mSubTaskCount));

            --it->second;
            if (it->second == 0)
            {
                setTaskStatus(it->first, success);
                mSubTaskCount.erase(it);
            }
        }
        else if (mTaskItems.empty() && mAutoCloseBtn->isChecked() && !mHadError)
        {
            close();
        }
    }

    void ActiveTasksDialog::autoCloseSave(bool enabled)
    {
        QSettings settings;
        settings.setValue(UISettings::autoCloseTasksKey, enabled);
    }

    void ActiveTasksDialog::fillTaskItem(quint32 taskId, QTreeWidgetItem *item, const QString &description)
    {
        item->setIcon(0, QIcon{":/images/information.png"});
        item->setData(0, Qt::UserRole, taskId);
        item->setText(1, description);
        item->setExpanded(true);

        mTaskItems[taskId] = item;
    }
}
