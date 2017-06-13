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
#include <QStackedWidget>
#include <QWebEnginePage>
#include <QWebEngineView>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>

#include "SSOAuthWidget.h"

namespace Evernus
{
    SSOAuthWidget::SSOAuthWidget(const QUrl &url, QWidget *parent)
        : QWidget(parent)
        , mAuthUrl(url)
    {
        auto mainLayout = new QVBoxLayout{this};

        auto urlLayout = new QHBoxLayout{};
        mainLayout->addLayout(urlLayout);

        mUrlEdit = new QLineEdit{this};
        urlLayout->addWidget(mUrlEdit);
        mUrlEdit->setReadOnly(true);

        auto useExternalBtn = new QPushButton{tr("Toggle external browser"), this};
        urlLayout->addWidget(useExternalBtn);
        connect(useExternalBtn, &QPushButton::clicked, this, [=] {
            mAuthWidgetStack->setCurrentIndex(mAuthWidgetStack->currentIndex() ^ 1);
            adjustSize();
        });

        mAuthWidgetStack = new QStackedWidget{this};
        mainLayout->addWidget(mAuthWidgetStack);

        mView = new QWebEngineView{this};
        mAuthWidgetStack->addWidget(mView);
        connect(mView, &QWebEngineView::urlChanged, this, [=](const QUrl &url) {
            mUrlEdit->setText(url.toString());
            mUrlEdit->setCursorPosition(0);
        });
        mView->setUrl(mAuthUrl);

        auto externalAuthWidget = new QWidget{this};
        mAuthWidgetStack->addWidget(externalAuthWidget);

        auto externalAuthLayout = new QVBoxLayout{externalAuthWidget};

        auto infoLabel = new QLabel{
            tr("To authorize inside the browser, use the following link and paste the resulting code below: <a href='%1'>%1</a>").arg(url.toString()),
            this};
        externalAuthLayout->addWidget(infoLabel);
        infoLabel->setWordWrap(true);
        infoLabel->setOpenExternalLinks(true);
        infoLabel->setTextFormat(Qt::RichText);

        auto codeLayout = new QHBoxLayout{};
        externalAuthLayout->addLayout(codeLayout);

        mCodeEdit = new QLineEdit{this};
        codeLayout->addWidget(mCodeEdit);
        mCodeEdit->setPlaceholderText(tr("paste the resulting code here"));

        auto authBtn = new QPushButton{tr("Authorize"), this};
        codeLayout->addWidget(authBtn);
        connect(authBtn, &QPushButton::clicked, this, &SSOAuthWidget::applyCode);
    }

    QWebEnginePage *SSOAuthWidget::page() const
    {
        return mView->page();
    }

    void SSOAuthWidget::applyCode()
    {
        const auto code = mCodeEdit->text().toLatin1();
        if (code.isEmpty())
        {
            QMessageBox::warning(this,
                                 tr("SSO Authentication"),
                                 tr("The supplied code is invalid. Please make sure all characters were copied or use internal browser authorization."));
            return;
        }

        emit acquiredCode(code);
    }
}
