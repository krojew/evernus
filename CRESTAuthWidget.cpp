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
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QWebFrame>
#include <QWebView>
#include <QWebPage>

#include "CRESTAuthWidget.h"

namespace Evernus
{
    CRESTAuthWidget::CRESTAuthWidget(QWidget *parent)
        : QWidget(parent)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto urlLayout = new QHBoxLayout{};
        mainLayout->addLayout(urlLayout);

        mUrlEdit = new QLineEdit{this};
        urlLayout->addWidget(mUrlEdit);
        mUrlEdit->setReadOnly(true);

        mView = new QWebView{this};
        mainLayout->addWidget(mView);
        connect(mView->page()->mainFrame(), &QWebFrame::urlChanged, [=](const QUrl &url) {
            mUrlEdit->setText(url.toString());
            mUrlEdit->setCursorPosition(0);
        });
    }

    QWebPage *CRESTAuthWidget::page() const
    {
        return mView->page();
    }

    void CRESTAuthWidget::setUrl(const QUrl &url)
    {
        mView->setUrl(url);
    }
}
