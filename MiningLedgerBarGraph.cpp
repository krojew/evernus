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
#include <QVBoxLayout>
#include <QMessageBox>
#include <QStringList>
#include <QCheckBox>
#include <QComboBox>
#include <Q3DCamera>
#include <Q3DScene>
#include <Q3DTheme>
#include <QSlider>
#include <Q3DBars>
#include <QLabel>

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

        const auto graph = new Q3DBars{};

        const auto container = QWidget::createWindowContainer(graph, this);
        mainLayout->addWidget(container, 1);

        if (!graph->hasContext())
        {
            QMessageBox::warning(nullptr, tr("Mining ledger"), tr("Couldn't initialize graph!"));
            return;
        }

        mTypeAxis->setTitle(tr("Mined type"));
        mTypeAxis->setTitleVisible(true);

        mSolarSystemAxis->setTitle(tr("Solar system"));
        mSolarSystemAxis->setTitleVisible(true);

        mValueAxis->setLabelFormat(QStringLiteral("%i"));

        graph->setColumnAxis(mTypeAxis);
        graph->setRowAxis(mSolarSystemAxis);
        graph->setValueAxis(mValueAxis);

        mMinedTypesSeries->setItemLabelFormat(QStringLiteral("(@rowLabel, @colLabel): %i"));
        mMinedTypesSeries->setMesh(QAbstract3DSeries::MeshBevelBar);

        graph->addSeries(mMinedTypesSeries);

        graph->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPresetIsometricRightHigh);

        const auto infoLayout = new QVBoxLayout{};
        mainLayout->addLayout(infoLayout);
        infoLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

        const auto infoText = new QLabel{
            tr("Hold the right mouse button to rotate the camera. Use the mouse wheel to zoom, and the left button to select data."),
            this
        };
        infoText->setWordWrap(true);
        infoLayout->addWidget(infoText);

        infoLayout->addWidget(new QLabel{tr("Camera preset"), this});

        const auto cameraPresetCombo = new QComboBox{this};
        infoLayout->addWidget(cameraPresetCombo);
        cameraPresetCombo->addItem(tr("None"), Q3DCamera::CameraPresetNone);
        cameraPresetCombo->addItem(tr("Front Low"), Q3DCamera::CameraPresetFrontLow);
        cameraPresetCombo->addItem(tr("Front"), Q3DCamera::CameraPresetFront);
        cameraPresetCombo->addItem(tr("Front High"), Q3DCamera::CameraPresetFrontHigh);
        cameraPresetCombo->addItem(tr("Left Low"), Q3DCamera::CameraPresetLeftLow);
        cameraPresetCombo->addItem(tr("Left"), Q3DCamera::CameraPresetLeft);
        cameraPresetCombo->addItem(tr("Left High"), Q3DCamera::CameraPresetLeftHigh);
        cameraPresetCombo->addItem(tr("Right Low"), Q3DCamera::CameraPresetRightLow);
        cameraPresetCombo->addItem(tr("Right"), Q3DCamera::CameraPresetRight);
        cameraPresetCombo->addItem(tr("Right High"), Q3DCamera::CameraPresetRightHigh);
        cameraPresetCombo->addItem(tr("Behind Low"), Q3DCamera::CameraPresetBehindLow);
        cameraPresetCombo->addItem(tr("Behind"), Q3DCamera::CameraPresetBehind);
        cameraPresetCombo->addItem(tr("Behind High"), Q3DCamera::CameraPresetBehindHigh);
        cameraPresetCombo->addItem(tr("Isometric Left"), Q3DCamera::CameraPresetIsometricLeft);
        cameraPresetCombo->addItem(tr("Isometric Left High"), Q3DCamera::CameraPresetIsometricLeftHigh);
        cameraPresetCombo->addItem(tr("Isometric Right"), Q3DCamera::CameraPresetIsometricRight);
        cameraPresetCombo->addItem(tr("Isometric Right High"), Q3DCamera::CameraPresetIsometricRightHigh);
        cameraPresetCombo->addItem(tr("Directly Above"), Q3DCamera::CameraPresetDirectlyAbove);
        cameraPresetCombo->addItem(tr("Directly Above CW45"), Q3DCamera::CameraPresetDirectlyAboveCW45);
        cameraPresetCombo->addItem(tr("Directly Above CCW45"), Q3DCamera::CameraPresetDirectlyAboveCCW45);
        connect(cameraPresetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [=] {
            graph->scene()->activeCamera()->setCameraPreset(static_cast<Q3DCamera::CameraPreset>(cameraPresetCombo->currentData().toInt()));
        });

        infoLayout->addWidget(new QLabel{tr("Bar style"), this});

        const auto barStyleCombo = new QComboBox{this};
        infoLayout->addWidget(barStyleCombo);
        barStyleCombo->addItem(tr("Bar"), QAbstract3DSeries::MeshBar);
        barStyleCombo->addItem(tr("Pyramid"), QAbstract3DSeries::MeshPyramid);
        barStyleCombo->addItem(tr("Cone"), QAbstract3DSeries::MeshCone);
        barStyleCombo->addItem(tr("Cylinder"), QAbstract3DSeries::MeshCylinder);
        barStyleCombo->addItem(tr("Bevel Bar"), QAbstract3DSeries::MeshBevelBar);
        barStyleCombo->addItem(tr("Sphere"), QAbstract3DSeries::MeshSphere);
        barStyleCombo->setCurrentIndex(4);
        connect(barStyleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [=] {
            mMinedTypesSeries->setMesh(static_cast<QAbstract3DSeries::Mesh>(barStyleCombo->currentData().toInt()));
        });

        infoLayout->addWidget(new QLabel{tr("Font size"), this});

        const auto fontSizeSlider = new QSlider{Qt::Horizontal, this};
        infoLayout->addWidget(fontSizeSlider);
        fontSizeSlider->setRange(1, 100);
        fontSizeSlider->setValue(graph->activeTheme()->font().pointSize());
        connect(fontSizeSlider, &QSlider::valueChanged, this, [=](auto value) {
            const auto theme = graph->activeTheme();

            auto font = theme->font();
            font.setPointSize(value);

            theme->setFont(font);
        });

        const auto gridBtn = new QCheckBox{tr("Show grid"), this};
        infoLayout->addWidget(gridBtn);
        gridBtn->setChecked(graph->activeTheme()->isGridEnabled());
        connect(gridBtn, &QCheckBox::stateChanged, this, [=](auto state) {
            graph->activeTheme()->setGridEnabled(state == Qt::Checked);
        });

        const auto smoothBtn = new QCheckBox{tr("Smooth bars"), this};
        infoLayout->addWidget(smoothBtn);
        smoothBtn->setChecked(mMinedTypesSeries->isMeshSmooth());
        connect(smoothBtn, &QCheckBox::stateChanged, this, [=](auto state) {
            mMinedTypesSeries->setMeshSmooth(state == Qt::Checked);
        });
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
        dataSet->reserve(static_cast<int>(mappedData.size()));

        QStringList rowLabels;
        rowLabels.reserve(dataSet->size());

        for (auto &systemData : mappedData)
        {
            rowLabels << systemData.first;

            auto row = std::make_unique<QBarDataRow>(static_cast<int>(allTypes.size()));
            auto curCol = std::begin(*row);

            for (const auto &typeName : allTypes)
            {
                curCol->setValue(systemData.second[typeName]);
                ++curCol;
            }

            dataSet->append(row.release());
        }

        QStringList colLabels;
        colLabels.reserve(static_cast<int>(allTypes.size()));

        for (const auto &typeName : allTypes)
            colLabels << typeName;

        mMinedTypesSeries->dataProxy()->resetArray(dataSet.release(), rowLabels, colLabels);
    }
}
