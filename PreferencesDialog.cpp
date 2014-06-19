#include <memory>
#include <vector>

#include <QDialogButtonBox>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTreeWidget>

#include "NetworkPreferencesWidget.h"

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

        QList<QTreeWidgetItem *> mainCategories;
        try
        {
            std::vector<std::pair<QString, std::unique_ptr<QWidget>>> categories;
            categories.emplace_back(std::make_pair(QString{tr("Network")}, std::unique_ptr<QWidget>{new NetworkPreferencesWidget{}}));

            for (auto i = 0; i < categories.size(); ++i)
            {
                auto item = new QTreeWidgetItem{QStringList{categories[i].first}};
                item->setData(0, Qt::UserRole, i);

                mainCategories << item;
            }

            for (auto &category : categories)
            {
                connect(this, SIGNAL(settingsInvalidated()), category.second.get(), SLOT(applySettings()));
                mPreferencesStack->addWidget(category.second.release());
            }
        }
        catch (...)
        {
            qDeleteAll(mainCategories);
            throw;
        }

        categoryTree->addTopLevelItems(mainCategories);
        categoryTree->setCurrentItem(mainCategories.first());

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
