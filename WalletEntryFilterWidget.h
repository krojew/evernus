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

#include <QWidget>

class QLineEdit;
class QDateEdit;
class QDate;

namespace Evernus
{
    class WalletEntryFilterWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        enum class EntryType
        {
            All,
            Incomig,
            Outgoing
        };

        explicit WalletEntryFilterWidget(QWidget *parent = nullptr);
        virtual ~WalletEntryFilterWidget() = default;

    signals:
        void filterChanged(const QDate &from, const QDate &to, const QString &filter, EntryType type);

    private slots:
        void changeEntryType();
        void applyKeywords();

        void fromChanged(const QDate &date);
        void toChanged(const QDate &date);

    private:
        QDateEdit *mFromEdit = nullptr;
        QDateEdit *mToEdit = nullptr;
        QLineEdit *mFilterEdit = nullptr;

        EntryType mCurrentType = EntryType::All;
    };
}

Q_DECLARE_METATYPE(Evernus::WalletEntryFilterWidget::EntryType)
