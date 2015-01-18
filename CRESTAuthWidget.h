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
class QWebView;
class QWebPage;
class QUrl;

namespace Evernus
{
    class CRESTAuthWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit CRESTAuthWidget(QWidget *parent = nullptr);

        QWebPage *page() const;

        void setUrl(const QUrl &url);

    private:
        QLineEdit *mUrlEdit = nullptr;
        QWebView *mView = nullptr;
    };
}
