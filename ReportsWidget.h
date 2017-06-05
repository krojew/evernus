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
#pragma once

#include <QSortFilterProxyModel>
#include <QWidget>

#include "TypePerformanceModel.h"
#include "Character.h"

class QCheckBox;

namespace Evernus
{
    class AdjustableTableView;
    class EveDataProvider;

    class ReportsWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        ReportsWidget(const RepositoryProvider &repositoryProvider,
                      const EveDataProvider &dataProvider,
                      QWidget *parent = nullptr);
        ReportsWidget(const ReportsWidget &) = default;
        ReportsWidget(ReportsWidget &&) = default;
        virtual ~ReportsWidget() = default;

        void setCharacter(Character::IdType id);

        ReportsWidget &operator =(const ReportsWidget &) = default;
        ReportsWidget &operator =(ReportsWidget &&) = default;

    private:
        QCheckBox *mCombineBtn = nullptr;
        QCheckBox *mCombineWithCorpBtn = nullptr;

        TypePerformanceModel mPerformanceModel;
        QSortFilterProxyModel mPerformanceProxy;

        AdjustableTableView *mBestItemsView = nullptr;

        Character::IdType mCharacterId = Character::invalidId;

        void recalculateData();
    };
}
