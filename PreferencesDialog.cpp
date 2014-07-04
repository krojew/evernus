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
#include <memory>
#include <vector>

#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidget>

#include "CharacterImportPreferencesWidget.h"
#include "AssetsImportPreferencesWidget.h"
#include "NetworkPreferencesWidget.h"
#include "PricePreferencesWidget.h"
#include "PathPreferencesWidget.h"

#include "PreferencesDialog.h"

namespace Evernus
{
    PreferencesDialog::PreferencesDialog(QWidget *parent)
        : QDialog{parent}
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

        std::vector<std::pair<QString, std::unique_ptr<QWidget>>> categories;
        categories.emplace_back(std::make_pair(QString{tr("Paths")}, std::unique_ptr<QWidget>{new PathPreferencesWidget{}}));
        categories.emplace_back(std::make_pair(QString{tr("Price")}, std::unique_ptr<QWidget>{new PricePreferencesWidget{}}));
        categories.emplace_back(std::make_pair(QString{tr("Network")}, std::unique_ptr<QWidget>{new NetworkPreferencesWidget{}}));

        for (auto i = 0; i < categories.size(); ++i)
        {
            auto item = new QTreeWidgetItem{categoryTree, QStringList{categories[i].first}};
            item->setData(0, Qt::UserRole, i);

            if (i == 0)
                categoryTree->setCurrentItem(item);
        }

        for (auto &category : categories)
        {
            connect(this, SIGNAL(settingsInvalidated()), category.second.get(), SLOT(applySettings()));
            mPreferencesStack->addWidget(category.second.release());
        }

        auto importItem = new QTreeWidgetItem{categoryTree, QStringList{tr("Import")}};
        importItem->setData(0, Qt::UserRole, -1);
        importItem->setExpanded(true);

        std::vector<std::pair<QString, std::unique_ptr<QWidget>>> importCategories;
        importCategories.emplace_back(std::make_pair(QString{tr("Character")}, std::unique_ptr<QWidget>{new CharacterImportPreferencesWidget{}}));
        importCategories.emplace_back(std::make_pair(QString{tr("Assets")}, std::unique_ptr<QWidget>{new AssetsImportPreferencesWidget{}}));

        for (auto i = 0; i < importCategories.size(); ++i)
        {
            auto item = new QTreeWidgetItem{importItem, QStringList{importCategories[i].first}};
            item->setData(0, Qt::UserRole, i + static_cast<int>(categories.size()));
        }

        for (auto &category : importCategories)
        {
            connect(this, SIGNAL(settingsInvalidated()), category.second.get(), SLOT(applySettings()));
            mPreferencesStack->addWidget(category.second.release());
        }

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
        {
            mPreferencesStack->setCurrentIndex(-1);
        }
        else
        {
            auto index = current->data(0, Qt::UserRole).toInt();
            if (index == -1)
            {
                Q_ASSERT(current->childCount() > 0);
                index = current->child(0)->data(0, Qt::UserRole).toInt();
            }

            mPreferencesStack->setCurrentIndex(index);
        }
    }
}
