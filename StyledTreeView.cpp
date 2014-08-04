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
#include <QApplication>
#include <QHeaderView>
#include <QClipboard>
#include <QAction>

#include "StyledTreeViewItemDelegate.h"

#include "StyledTreeView.h"

namespace Evernus
{
    StyledTreeView::StyledTreeView(QWidget *parent)
        : QTreeView(parent)
    {
        setSortingEnabled(true);
        setItemDelegate(new StyledTreeViewItemDelegate{this});
        setSelectionMode(QAbstractItemView::ExtendedSelection);
        setContextMenuPolicy(Qt::ActionsContextMenu);

        header()->resizeSections(QHeaderView::Stretch);

        auto action = new QAction{QIcon{":/images/page_copy.png"}, tr("&Copy"), this};
        action->setShortcuts(QKeySequence::Copy);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        connect(action, &QAction::triggered, this, &StyledTreeView::copy);
        addAction(action);

        action = new QAction{tr("Copy &rows"), this};
        connect(action, &QAction::triggered, this, &StyledTreeView::copyRows);
        addAction(action);

        action = new QAction{tr("Copy raw &data"), this};
        connect(action, &QAction::triggered, this, &StyledTreeView::copyRawData);
        addAction(action);
    }

    void StyledTreeView::copy()
    {
        const auto curModel = model();
        if (curModel != nullptr)
        {
            const auto data = curModel->data(currentIndex(), Qt::DisplayRole);
            if (static_cast<QMetaType::Type>(data.type()) == QMetaType::QString)
                QApplication::clipboard()->setText(data.toString());
        }
    }

    void StyledTreeView::copyRows()
    {
        copyRowsWithRole(Qt::DisplayRole);
    }

    void StyledTreeView::copyRawData()
    {
        copyRowsWithRole(Qt::UserRole);
    }

    void StyledTreeView::copyRowsWithRole(int role) const
    {
        const auto curModel = model();
        if (curModel != nullptr)
        {
            const auto indexes = selectionModel()->selectedIndexes();
            if (indexes.isEmpty())
                return;

            QString result;

            auto prevRow = indexes.first().row();
            for (const auto &index : indexes)
            {
                if (prevRow != index.row())
                {
                    prevRow = index.row();
                    result.append('\n');
                }

                const auto data = curModel->data(index, role);
                if (static_cast<QMetaType::Type>(data.type()) == QMetaType::QVariantList)
                    result.append(data.toList().first().toString());
                else
                    result.append(data.toString());

                result.append('\t');
            }

            QApplication::clipboard()->setText(result);
        }
    }
}
