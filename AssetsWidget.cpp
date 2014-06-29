#include <QVBoxLayout>
#include <QHBoxLayout>

#include "ButtonWithTimer.h"

#include "AssetsWidget.h"

namespace Evernus
{
    AssetsWidget::AssetsWidget(QWidget *parent)
        : QWidget{parent}
    {
        auto mainLayout = new QVBoxLayout{};
        setLayout(mainLayout);

        auto toolBarLayout = new QHBoxLayout{};
        mainLayout->addLayout(toolBarLayout);

        mImportBtn = new ButtonWithTimer{tr("API import"), this};
        toolBarLayout->addWidget(mImportBtn);
        connect(mImportBtn, &QPushButton::clicked, this, &AssetsWidget::requestUpdate);

        toolBarLayout->addStretch();

        mainLayout->addStretch();
    }

    void AssetsWidget::setCharacter(Character::IdType id)
    {
        mCharacterId = id;
    }

    void AssetsWidget::requestUpdate()
    {
        Q_ASSERT(mCharacterId != Character::invalidId);
        emit importAssets(mCharacterId);
    }
}
