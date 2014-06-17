#pragma once

#include <QDialog>

class QLineEdit;

namespace Evernus
{
    class Key;

    class KeyEditDialog
        : public QDialog
    {
    public:
        explicit KeyEditDialog(Key &key, QWidget *parent = nullptr);
        virtual ~KeyEditDialog() = default;

        virtual void accept() override;

    private:
        Key &mKey;

        QLineEdit *mIdEdit = nullptr;
        QLineEdit *mCodeEdit = nullptr;
    };
}
