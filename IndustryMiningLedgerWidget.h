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

#include "CharacterBoundWidget.h"
#include "MiningLedgerModel.h"
#include "EveTypeProvider.h"

namespace Evernus
{
    class MiningLedgerRepository;
    class AdjustableTableView;
    class CacheTimerProvider;
    class LookupActionGroup;
    class DateRangeWidget;
    class EveDataProvider;

    class IndustryMiningLedgerWidget
        : public CharacterBoundWidget
        , public EveTypeProvider
    {
        Q_OBJECT

    public:
        IndustryMiningLedgerWidget(const CacheTimerProvider &cacheTimerProvider,
                                   const EveDataProvider &dataProvider,
                                   const MiningLedgerRepository &ledgerRepo,
                                   QWidget *parent = nullptr);
        IndustryMiningLedgerWidget(const IndustryMiningLedgerWidget &) = default;
        IndustryMiningLedgerWidget(IndustryMiningLedgerWidget &&) = default;
        virtual ~IndustryMiningLedgerWidget() = default;

        virtual EveType::IdType getTypeId() const override;

        IndustryMiningLedgerWidget &operator =(const IndustryMiningLedgerWidget &) = default;
        IndustryMiningLedgerWidget &operator =(IndustryMiningLedgerWidget &&) = default;

    public slots:
        void refresh();

    private slots:
        void importData();

    private:
        DateRangeWidget *mRangeFilter = nullptr;
        AdjustableTableView *mDetailsView = nullptr;

        LookupActionGroup *mLookupGroup = nullptr;

        MiningLedgerModel mDetailsModel;
        QSortFilterProxyModel mDetailsProxy;

        virtual void handleNewCharacter(Character::IdType id) override;
    };
}
