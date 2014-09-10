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
#include "LMeveTask.h"

namespace Evernus
{
    Character::IdType LMeveTask::getCharacterId() const noexcept
    {
        return mCharacterId;
    }

    void LMeveTask::setCharacterId(Character::IdType id) noexcept
    {
        mCharacterId = id;
    }

    EveType::IdType LMeveTask::getTypeId() const noexcept
    {
        return mTypeId;
    }

    void LMeveTask::setTypeId(EveType::IdType id) noexcept
    {
        mTypeId = id;
    }

    QString LMeveTask::getActivity() const &
    {
        return mActivity;
    }

    QString &&LMeveTask::getActivity() && noexcept
    {
        return std::move(mActivity);
    }

    void LMeveTask::setActivity(const QString &activity)
    {
        mActivity = activity;
    }
    void LMeveTask::setActivity(QString &&activity)
    {
        mActivity = std::move(activity);
    }

    LMeveTask::Counter LMeveTask::getRuns() const
    {
        return mRuns;
    }

    void LMeveTask::setRuns(const Counter &value)
    {
        mRuns = value;
    }

    LMeveTask::Counter LMeveTask::getRunsDone() const
    {
        return mRunsDone;
    }

    void LMeveTask::setRunsDone(const Counter &value)
    {
        mRunsDone = value;
    }

    LMeveTask::Counter LMeveTask::getRunsCompleted() const
    {
        return mRunsCompleted;
    }

    void LMeveTask::setRunsCompleted(const Counter &value)
    {
        mRunsCompleted = value;
    }

    LMeveTask::Counter LMeveTask::getJobsDone() const
    {
        return mJobsDone;
    }

    void LMeveTask::setJobsDone(const Counter &value)
    {
        mJobsDone = value;
    }

    LMeveTask::Counter LMeveTask::getJobsSuccess() const
    {
        return mJobsSuccess;
    }

    void LMeveTask::setJobsSuccess(const Counter &value)
    {
        mJobsSuccess = value;
    }

    LMeveTask::Counter LMeveTask::getJobsCompleted() const
    {
        return mJobsCompleted;
    }

    void LMeveTask::setJobsCompleted(const Counter &value)
    {
        mJobsCompleted = value;
    }
}
