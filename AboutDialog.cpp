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
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>

#include "AboutDialog.h"

namespace Evernus
{
    AboutDialog::AboutDialog(QWidget *parent)
        : QDialog(parent)
    {
        auto mainLayout = new QHBoxLayout{};
        setLayout(mainLayout);

        auto icon = parent->windowIcon();

        auto iconLabel = new QLabel{this};
        mainLayout->addWidget(iconLabel);
        iconLabel->setPixmap(icon.pixmap(icon.actualSize(QSize{64, 64})));

        const auto link = "http://evernus.com";

        auto aboutLabel = new QLabel{tr(
            "<strong>%1</strong><br />%2<br /><br />"
            "Created by <strong><a href='http://evewho.com/pilot/Pete+Butcher'>Pete Butcher</a></strong><br />"
            "All donations are welcome :)<br /><a href='%3'>%3</a>")
                .arg(QCoreApplication::applicationName())
                .arg(QCoreApplication::applicationVersion())
                .arg(link),
            this};
        mainLayout->addWidget(aboutLabel);
        aboutLabel->setTextFormat(Qt::RichText);
        aboutLabel->setOpenExternalLinks(true);

        setWindowTitle(tr("About"));
    }
}
