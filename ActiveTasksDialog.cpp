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

    void ActiveTasksDialog::addNewTaskInfo(uint taskId, const QString &description)
    {
        if (mTaskItems.empty())
        {
            mHadError = false;
            mTotalProgressWidget->setValue(0);
            mTotalProgressWidget->setMaximum(1);
            mTaskWidget->clear();
        }
        else
        {
            mTotalProgressWidget->setMaximum(mTotalProgressWidget->maximum() + 1);
        }

        fillTaskItem(taskId, new QTreeWidgetItem{mTaskWidget}, description);
        mTaskWidget->resizeColumnToContents(0);
    }

    void ActiveTasksDialog::addNewSubTaskInfo(uint taskId, uint parentTask, const QString &description)
    {
        auto item = mTaskItems.find(parentTask);
        if (item == std::end(mTaskItems))
        {
            QMetaObject::invokeMethod(this, "addNewSubTaskInfo", Qt::QueuedConnection, Q_ARG(uint, taskId), Q_ARG(uint, parentTask), Q_ARG(QString, description));
            return;
        }

        mTotalProgressWidget->setMaximum(mTotalProgressWidget->maximum() + 1);

        fillTaskItem(taskId, new QTreeWidgetItem{item->second}, description);
        mTaskWidget->resizeColumnToContents(0);

        ++mSubTaskInfo[parentTask].mCount;
    }

    void ActiveTasksDialog::setTaskStatus(uint taskId, const QString &error)
    {
        auto item = mTaskItems.find(taskId);
        if (item == std::end(mTaskItems))
        {
            QMetaObject::invokeMethod(this, "setTaskStatus", Qt::QueuedConnection, Q_ARG(uint, taskId), Q_ARG(QString, error));
            return;
        }

        const auto subTaskInfo = mSubTaskInfo.find(taskId);
        const auto success = error.isEmpty() && (subTaskInfo == std::end(mSubTaskInfo) || !subTaskInfo->second.mError);

        mHadError = mHadError || !success;

        const auto parent = item->second->parent();

        item->second->setIcon(0, QIcon{(success) ? (":/images/accept.png") : (":/images/exclamation.png")});
        if (!success && !error.isEmpty())
        {
            auto errorItem = new QTreeWidgetItem{item->second};
            errorItem->setText(1, error);
        }

        mTaskItems.erase(item);
        mTotalProgressWidget->setValue(mTotalProgressWidget->value() + 1);

        if (parent != nullptr)
        {
            const auto it = mSubTaskInfo.find(parent->data(0, Qt::UserRole).toUInt());
            Q_ASSERT(it != std::end(mSubTaskInfo));

            it->second.mError = it->second.mError || !success;

            --it->second.mCount;
            if (it->second.mCount == 0)
            {
                setTaskStatus(it->first, QString{});
                mSubTaskInfo.erase(it);
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

    void ActiveTasksDialog::fillTaskItem(uint taskId, QTreeWidgetItem *item, const QString &description)
    {
        item->setIcon(0, QIcon{":/images/information.png"});
        item->setData(0, Qt::UserRole, taskId);
        item->setText(1, description);
        item->setExpanded(true);

        mTaskItems[taskId] = item;
    }
}
