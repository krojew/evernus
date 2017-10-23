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
#include <map>
#include <set>

#include <QCategory3DAxis>
#include <QBarDataProxy>
#include <QValue3DAxis>
#include <QBar3DSeries>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStringList>
#include <Q3DCamera>
#include <Q3DScene>
#include <Q3DBars>

#include "MiningLedgerRepository.h"
#include "EveDataProvider.h"

#include "MiningLedgerBarGraph.h"

using namespace QtDataVisualization;

namespace Evernus
{
    MiningLedgerBarGraph::MiningLedgerBarGraph(const MiningLedgerRepository &ledgerRepo,
                                               const EveDataProvider &dataProvider,
                                               QWidget *parent)
        : QWidget{parent}
        , mLedgerRepo{ledgerRepo}
        , mDataProvider{dataProvider}
        , mTypeAxis{new QCategory3DAxis{this}}
        , mSolarSystemAxis{new QCategory3DAxis{this}}
        , mValueAxis{new QValue3DAxis{this}}
        , mMinedTypesSeries{new QBar3DSeries{this}}
    {
        const auto mainLayout = new QHBoxLayout{this};
        mainLayout->setContentsMargins(0, 0, 0, 0);     // remove if adding something else to the layout

        const auto graph = new Q3DBars{};

        const auto container = QWidget::createWindowContainer(graph, this);
        mainLayout->addWidget(container);

        if (!graph->hasContext())
        {
            QMessageBox::warning(nullptr, tr("Mining ledger"), tr("Couldn't initialize graph!"));
            return;
        }

        mTypeAxis->setTitle(tr("Mined type"));
        mTypeAxis->setTitleVisible(true);

        mSolarSystemAxis->setTitle(tr("Solar system"));
        mSolarSystemAxis->setTitleVisible(true);

        graph->setColumnAxis(mTypeAxis);
        graph->setRowAxis(mSolarSystemAxis);
        graph->setValueAxis(mValueAxis);

        mMinedTypesSeries->setItemLabelFormat(QStringLiteral("(@rowLabel, @colLabel): %i"));
        mMinedTypesSeries->setMesh(QAbstract3DSeries::MeshBevelBar);

        graph->addSeries(mMinedTypesSeries);

        graph->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetIsometricRightHigh);
    }

    void MiningLedgerBarGraph::refresh(Character::IdType charId, const QDate &from, const QDate &to)
    {
        const auto data = mLedgerRepo.fetchAggregatedDataForCharacter(charId, from, to);

        std::map<QString, std::map<QString, quint64>, std::greater<QString>> mappedData;
        std::set<QString> allTypes;

        for (const auto &datum : data)
        {
            const auto typeName = mDataProvider.getTypeName(datum.mTypeId);
            mappedData[mDataProvider.getSolarSystemName(datum.mSolarSystemId)][typeName] += datum.mQuantity;

            allTypes.insert(typeName);
        }

        auto dataSet = std::make_unique<QBarDataArray>();
        dataSet->reserve(mappedData.size());

        QStringList rowLabels;
        rowLabels.reserve(dataSet->size());

        for (auto &systemData : mappedData)
        {
            rowLabels << systemData.first;

            auto row = std::make_unique<QBarDataRow>(allTypes.size());
            auto curCol = std::begin(*row);

            for (const auto &typeName : allTypes)
            {
                curCol->setValue(systemData.second[typeName]);
                ++curCol;
            }

            dataSet->append(row.release());
        }

        QStringList colLabels;
        colLabels.reserve(allTypes.size());

        for (const auto &typeName : allTypes)
            colLabels << typeName;

        mMinedTypesSeries->dataProxy()->resetArray(dataSet.release(), rowLabels, colLabels);
    }
}
