/**
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU IGB Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU IGB Public License for more details.
 *
 *  You should have received a copy of the GNU IGB Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <QWidget>

class QCheckBox;
class QLineEdit;

namespace Evernus
{
    class IGBPreferencesWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit IGBPreferencesWidget(QWidget *parent = nullptr);
        virtual ~IGBPreferencesWidget() = default;

    public slots:
        void applySettings();

    private:
        QCheckBox *mEnabledBtn = nullptr;
        QLineEdit *mPortEdit = nullptr;
    };
}
