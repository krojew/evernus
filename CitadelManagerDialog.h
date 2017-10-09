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

#include <QDialog>

#include "Character.h"

namespace Evernus
{
    class CitadelLocationWidget;
    class CitadelAccessCache;
    class CitadelRepository;
    class EveDataProvider;

    class CitadelManagerDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        CitadelManagerDialog(const EveDataProvider &dataProvider,
                             const CitadelRepository &citadelRepo,
                             CitadelAccessCache &citadelAccessCache,
                             Character::IdType charId,
                             QWidget *parent = nullptr);
        CitadelManagerDialog(const CitadelManagerDialog &) = default;
        CitadelManagerDialog(CitadelManagerDialog &&) = default;
        virtual ~CitadelManagerDialog() = default;

        CitadelManagerDialog &operator =(const CitadelManagerDialog &) = default;
        CitadelManagerDialog &operator =(CitadelManagerDialog &&) = default;

    signals:
        void citadelsChanged();

    private slots:
        void applyChanges();
        void refreshAccessCache();

    private:
        const CitadelRepository &mCitadelRepo;
        CitadelAccessCache &mCitadelAccessCache;

        CitadelLocationWidget *mIgnoredCitadelsWidget = nullptr;
    };
}
