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
#include <QStandardItemModel>
#include <QSettings>

#include "EveDataProvider.h"

#include "RegionComboBox.h"

namespace Evernus
{
    RegionComboBox::RegionComboBox(const EveDataProvider &dataProvider, const QString &settingsKey, QWidget *parent)
        : QComboBox(parent)
    {
        setEditable(true);
        setInsertPolicy(QComboBox::NoInsert);

        std::unordered_set<uint> saved;
        QSettings settings;

        const auto savedList = settings.value(settingsKey).toList();
        for (const auto &value : savedList)
            saved.emplace(value.toUInt());

        auto model = new QStandardItemModel{this};

        const auto regions = dataProvider.getRegions();
        for (const auto &region : regions)
        {
            auto item = new QStandardItem{region.second};
            item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            item->setData(region.first);
            item->setCheckState((saved.find(region.first) != std::end(saved)) ? (Qt::Checked) : (Qt::Unchecked));

            model->appendRow(item);
        }

        setModel(model);

        connect(model, &QStandardItemModel::itemChanged, this, [=] {
            QVariantList saved;
            for (auto i = 0; i < model->rowCount(); ++i)
            {
                const auto item = model->item(i);
                if (item->checkState() == Qt::Checked)
                    saved.append(item->data().toUInt());
            }

            QSettings settings;
            settings.setValue(settingsKey, saved);

            setRegionText();
        }, Qt::QueuedConnection);

        auto item = new QStandardItem{tr("- all -")};
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState((saved.empty() || saved.find(0) != std::end(saved)) ? (Qt::Checked) : (Qt::Unchecked));

        model->insertRow(allRegionsIndex, item);
        setRegionText();
    }

    RegionComboBox::RegionList RegionComboBox::getSelectedRegionList() const
    {
        RegionList list;

        const auto srcModel = static_cast<const QStandardItemModel *>(model());
        if (srcModel->item(allRegionsIndex)->checkState() == Qt::Checked)
        {
            for (auto i = allRegionsIndex + 1; i < srcModel->rowCount(); ++i)
                list.emplace(srcModel->item(i)->data().toUInt());
        }
        else
        {
            for (auto i = allRegionsIndex + 1; i < srcModel->rowCount(); ++i)
            {
                if (srcModel->item(i)->checkState() == Qt::Checked)
                    list.emplace(srcModel->item(i)->data().toUInt());
            }
        }

        return list;
    }

    void RegionComboBox::setRegionText()
    {
        const auto srcModel = static_cast<const QStandardItemModel *>(model());
        if (srcModel->item(allRegionsIndex)->checkState() == Qt::Checked)
        {
            setCurrentText(tr("- all -"));
            return;
        }

        auto hasChecked = false;
        for (auto i = allRegionsIndex + 1; i < srcModel->rowCount(); ++i)
        {
            const auto item = srcModel->item(i);
            if (item->checkState() == Qt::Checked)
            {
                if (hasChecked)
                {
                    setCurrentText(tr("- multiple -"));
                    return;
                }

                hasChecked = true;
                setCurrentText(item->text());
            }
        }

        if (!hasChecked)
            setCurrentText(tr("- none -"));
    }
}
