#pragma once

#include <QWidget>

#include "Character.h"

namespace Evernus
{
    class ButtonWithTimer;

    class AssetsWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit AssetsWidget(QWidget *parent = nullptr);
        virtual ~AssetsWidget() = default;

    signals:
        void importAssets(Character::IdType id);

    public slots:
        void setCharacter(Character::IdType id);

    private slots:
        void requestUpdate();

    private:
        ButtonWithTimer *mImportBtn = nullptr;

        Character::IdType mCharacterId = Character::invalidId;
    };
}
