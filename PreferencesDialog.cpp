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
#include <vector>

#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidget>

#include "CharacterImportPreferencesWidget.h"
#include "AssetsImportPreferencesWidget.h"
#include "NetworkPreferencesWidget.h"
#include "GeneralPreferencesWidget.h"
#include "WalletPreferencesWidget.h"
#include "ImportPreferencesWidget.h"
#include "PricePreferencesWidget.h"
#include "PathPreferencesWidget.h"

#include "PreferencesDialog.h"

namespace Evernus
{
    PreferencesDialog::PreferencesDialog(QWidget *parent)
        : QDialog(parent)
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto preferencesLayout = new QHBoxLayout{};
        mainLayout->addLayout(preferencesLayout);

        auto categoryTree = new QTreeWidget{this};
        preferencesLayout->addWidget(categoryTree);
        categoryTree->setHeaderHidden(true);
        categoryTree->setMaximumWidth(150);
        connect(categoryTree, &QTreeWidget::currentItemChanged, this, &PreferencesDialog::setCurrentPage);

        mPreferencesStack = new QStackedWidget{this};
        preferencesLayout->addWidget(mPreferencesStack, 1);

        std::vector<std::pair<QString, QWidget *>> categories;
        categories.emplace_back(std::make_pair(tr("General"), new GeneralPreferencesWidget{this}));
        categories.emplace_back(std::make_pair(tr("Paths"), new PathPreferencesWidget{this}));
        categories.emplace_back(std::make_pair(tr("Prices"), new PricePreferencesWidget{this}));
        categories.emplace_back(std::make_pair(tr("Network"), new NetworkPreferencesWidget{this}));
        categories.emplace_back(std::make_pair(tr("Wallet"), new WalletPreferencesWidget{this}));

        for (auto i = 0; i < categories.size(); ++i)
        {
            auto item = new QTreeWidgetItem{categoryTree, QStringList{categories[i].first}};
            item->setData(0, Qt::UserRole, i);

            if (i == 0)
                categoryTree->setCurrentItem(item);
        }

        for (auto &category : categories)
        {
            connect(this, SIGNAL(settingsInvalidated()), category.second, SLOT(applySettings()));
            mPreferencesStack->addWidget(category.second);
        }

        auto importItem = new QTreeWidgetItem{categoryTree, QStringList{tr("Import")}};
        importItem->setExpanded(true);

        std::vector<std::pair<QString, QWidget *>> importCategories;
        importCategories.emplace_back(std::make_pair(tr("Character"), new CharacterImportPreferencesWidget{this}));
        importCategories.emplace_back(std::make_pair(tr("Assets"), new AssetsImportPreferencesWidget{this}));

        for (auto i = 0; i < importCategories.size(); ++i)
        {
            auto item = new QTreeWidgetItem{importItem, QStringList{importCategories[i].first}};
            item->setData(0, Qt::UserRole, i + static_cast<int>(categories.size()));
        }

        for (auto &category : importCategories)
        {
            connect(this, SIGNAL(settingsInvalidated()), category.second, SLOT(applySettings()));
            mPreferencesStack->addWidget(category.second);
        }

        auto importPreferencesWidget = new ImportPreferencesWidget{this};
        connect(this, &PreferencesDialog::settingsInvalidated, importPreferencesWidget, &ImportPreferencesWidget::applySettings);

        mPreferencesStack->addWidget(importPreferencesWidget);
        importItem->setData(0, Qt::UserRole, static_cast<int>(categories.size() + importCategories.size()));

        auto buttons = new QDialogButtonBox{QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this};
        mainLayout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        setWindowTitle(tr("Preferences"));
    }

    void PreferencesDialog::accept()
    {
        emit settingsInvalidated();

        QDialog::accept();
    }

    void PreferencesDialog::setCurrentPage(QTreeWidgetItem *current, QTreeWidgetItem *previous)
    {
        if (current == nullptr)
            mPreferencesStack->setCurrentIndex(-1);
        else
            mPreferencesStack->setCurrentIndex(current->data(0, Qt::UserRole).toInt());
    }
}
