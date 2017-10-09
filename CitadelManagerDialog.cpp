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
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>

#include "CitadelLocationWidget.h"
#include "CitadelAccessCache.h"
#include "CitadelRepository.h"

#include "CitadelManagerDialog.h"

namespace Evernus
{
    CitadelManagerDialog::CitadelManagerDialog(const EveDataProvider &dataProvider,
                                               const CitadelRepository &citadelRepo,
                                               CitadelAccessCache &citadelAccessCache,
                                               Character::IdType charId,
                                               QWidget *parent)
        : QDialog{parent, Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint}
        , mCitadelRepo{citadelRepo}
        , mCitadelAccessCache{citadelAccessCache}
    {
        const auto mainLayout = new QVBoxLayout{this};

        const auto ignoredBox = new QGroupBox{tr("Ignored citadels"), this};
        mainLayout->addWidget(ignoredBox);

        const auto ignoredBoxLayout = new QVBoxLayout{ignoredBox};

        ignoredBoxLayout->addWidget(new QLabel{tr("Ignored citadels will not have their data imported."), this});

        mIgnoredCitadelsWidget = new CitadelLocationWidget{dataProvider, mCitadelAccessCache, charId, this};
        ignoredBoxLayout->addWidget(mIgnoredCitadelsWidget);
        connect(this, &CitadelManagerDialog::citadelsChanged, mIgnoredCitadelsWidget, &CitadelLocationWidget::refresh);

        ignoredBoxLayout->addWidget(new QLabel{tr("<s>Citadel Name</s> - citadel unavailable for current character"), this});

        const auto refreshAccessCacheBtn = new QPushButton{tr("Refresh access cache"), this};
        mainLayout->addWidget(refreshAccessCacheBtn);
        refreshAccessCacheBtn->setToolTip(tr("Clear citadel access cache to check if your characters have access to various citadels."));
        connect(refreshAccessCacheBtn, &QPushButton::clicked, this, &CitadelManagerDialog::refreshAccessCache);

        const auto buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &CitadelManagerDialog::applyChanges);
        connect(buttons, &QDialogButtonBox::rejected, this, &CitadelManagerDialog::reject);

        setWindowTitle(tr("Citadel manager"));
    }

    void CitadelManagerDialog::applyChanges()
    {
        mCitadelRepo.setIgnored(mIgnoredCitadelsWidget->getSelectedCitadels());
        accept();
    }

    void CitadelManagerDialog::refreshAccessCache()
    {
        mCitadelAccessCache.clear();
        mIgnoredCitadelsWidget->refresh();
    }
}
