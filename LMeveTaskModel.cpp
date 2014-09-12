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
#include <QLocale>

#include "EveDataProvider.h"

#include "LMeveTaskModel.h"

namespace Evernus
{
    LMeveTaskModel::LMeveTaskModel(const EveDataProvider &dataProvider, QObject *parent)
        : QAbstractTableModel{parent}
        , mDataProvider{dataProvider}
    {
    }

    int LMeveTaskModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);
        return numColumns;
    }

    QVariant LMeveTaskModel::data(const QModelIndex &index, int role) const
    {
        if (!index.isValid())
            return QVariant{};

        const auto &data = mData[index.row()];

        if (role == Qt::UserRole)
        {
            switch (index.column()) {
            case typeColumn:
                return mDataProvider.getTypeName(data->getTypeId());
            case activityColumn:
                return data->getActivity();
            case runsColumn:
                if (data->getRuns())
                    return *data->getRuns();
                break;
            case runsDoneColumn:
                if (data->getRunsDone())
                    return *data->getRunsDone();
                break;
            case runsCompletedColumn:
                if (data->getRunsCompleted())
                    return *data->getRunsCompleted();
                break;
            case jobsDoneColumn:
                if (data->getJobsDone())
                    return *data->getJobsDone();
                break;
            case jobsSuccessColumn:
                if (data->getJobsSuccess())
                    return *data->getJobsSuccess();
                break;
            case jobsCompletedColumn:
                if (data->getJobsCompleted())
                    return *data->getJobsCompleted();
            }
        }
        else if (role == Qt::DisplayRole)
        {
            QLocale locale;
            switch (index.column()) {
            case typeColumn:
                return mDataProvider.getTypeName(data->getTypeId());
            case activityColumn:
                return data->getActivity();
            case runsColumn:
                if (data->getRuns())
                    return locale.toString(*data->getRuns());
                break;
            case runsDoneColumn:
                if (data->getRunsDone())
                    return locale.toString(*data->getRunsDone());
                break;
            case runsCompletedColumn:
                if (data->getRunsCompleted())
                    return locale.toString(*data->getRunsCompleted());
                break;
            case jobsDoneColumn:
                if (data->getJobsDone())
                    return locale.toString(*data->getJobsDone());
                break;
            case jobsSuccessColumn:
                if (data->getJobsSuccess())
                    return locale.toString(*data->getJobsSuccess());
                break;
            case jobsCompletedColumn:
                if (data->getJobsCompleted())
                    return locale.toString(*data->getJobsCompleted());
            }
        }

        return QVariant{};
    }

    QVariant LMeveTaskModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        {
            switch (section) {
            case typeColumn:
                return tr("Item");
            case activityColumn:
                return tr("Activity");
            case runsColumn:
                return tr("Runs");
            case runsDoneColumn:
                return tr("Runs completed and in progress");
            case runsCompletedColumn:
                return tr("Runs completed");
            case jobsDoneColumn:
                return tr("Jobs completed and in progress");
            case jobsSuccessColumn:
                return tr("Successful jobs");
            case jobsCompletedColumn:
                return tr("Jobs completed");
            }
        }

        return QVariant{};
    }

    int LMeveTaskModel::rowCount(const QModelIndex &parent) const
    {
        return (parent.isValid()) ? (0) : (static_cast<int>(mData.size()));
    }

    void LMeveTaskModel::setTasks(const TaskList &data)
    {
        beginResetModel();
        mData = data;
        endResetModel();
    }

    void LMeveTaskModel::setTasks(TaskList &&data)
    {
        beginResetModel();
        mData = std::move(data);
        endResetModel();
    }
}
