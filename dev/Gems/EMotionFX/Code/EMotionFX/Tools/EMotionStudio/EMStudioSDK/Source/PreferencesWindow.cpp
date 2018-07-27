/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// include the required headers
#include "PreferencesWindow.h"

#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <MCore/Source/LogManager.h>
#include <MysticQt/Source/MysticQtManager.h>

#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QListWidget>


namespace EMStudio
{
    // constructor
    PreferencesWindow::PreferencesWindow(QWidget* parent)
        : QDialog(parent)
    {
        mCategories.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK);
    }


    // destructor
    PreferencesWindow::~PreferencesWindow()
    {
        // get rid of the categories
        const uint32 numCategories = mCategories.GetLength();
        for (uint32 i = 0; i < numCategories; ++i)
        {
            delete mCategories[i];
        }
        mCategories.Clear();
    }


    // initialize the preferences window
    void PreferencesWindow::Init()
    {
        setWindowTitle("Preferences");
        setSizeGripEnabled(false);

        const int32 iconSize = 64;
        const int32 spacing = 6;

        // the icons on the left which list the categories
        mCategoriesWidget = new QListWidget();
        mCategoriesWidget->setViewMode(QListView::IconMode);
        mCategoriesWidget->setIconSize(QSize(iconSize, iconSize));
        mCategoriesWidget->setMovement(QListView::Static);
        mCategoriesWidget->setMaximumWidth(iconSize + 20);
        mCategoriesWidget->setMinimumHeight(5 * iconSize + 5 * spacing);
        mCategoriesWidget->setSpacing(spacing);
        mCategoriesWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        connect(mCategoriesWidget, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),   this, SLOT(ChangePage(QListWidgetItem*, QListWidgetItem*)));

        // create the stacked widget, this one will store all the different property browsers for the categories
        mStackedWidget = new QStackedWidget();
        mStackedWidget->setMinimumSize(700, 480);

        // horizontal layout, icons left, property browsers right
        QHBoxLayout* horizontalLayout = new QHBoxLayout;
        horizontalLayout->addWidget(mCategoriesWidget);
        horizontalLayout->addWidget(mStackedWidget);
        setLayout(horizontalLayout);

        mCategoriesWidget->setCurrentRow(0);
    }


    // add a new category
    void PreferencesWindow::AddCategory(QWidget* widget, const char* categoryName, const char* relativeFileName, bool readOnly)
    {
        MCORE_UNUSED(readOnly);

        // create the category button
        QListWidgetItem* categoryButton = new QListWidgetItem();

        // load the category image and pass it to the category buttom
        AZStd::string imageFileName = MysticQt::GetDataDir() + relativeFileName;
        categoryButton->setIcon(QIcon(imageFileName.c_str()));

        // set the category button name and style it
        categoryButton->setText(categoryName);
        categoryButton->setTextAlignment(Qt::AlignHCenter);
        categoryButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        // add the button to the categories list and the property widget to the stacked widget on the right
        mCategoriesWidget->addItem(categoryButton);
        mStackedWidget->addWidget(widget);

        // create a category object so that we can get access to the data later on again
        Category* category          = new Category();
        category->mName             = categoryName;
        category->mWidget           = widget;
        category->mPropertyWidget   = nullptr;
        category->mListWidgetItem   = categoryButton;

        // add the new category object to the categories array
        mCategories.Add(category);
    }


    // add a new category
    AzToolsFramework::ReflectedPropertyEditor* PreferencesWindow::AddCategory(const char* categoryName, const char* relativeFileName, bool readOnly)
    {
        MCORE_UNUSED(readOnly);

        // create the category button
        QListWidgetItem* categoryButton = new QListWidgetItem();

        // load the category image and pass it to the category button
        AZStd::string imageFileName = MysticQt::GetDataDir() + relativeFileName;
        categoryButton->setIcon(QIcon(imageFileName.c_str()));

        // set the category button name and style it
        categoryButton->setText(categoryName);
        categoryButton->setTextAlignment(Qt::AlignHCenter);
        categoryButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

        // create the new property widget
        AzToolsFramework::ReflectedPropertyEditor* propertyWidget = aznew AzToolsFramework::ReflectedPropertyEditor(this);
        propertyWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        // add the button to the categories list and the property widget to the stacked widget on the right
        mCategoriesWidget->addItem(categoryButton);
        mStackedWidget->addWidget(propertyWidget);

        // create a category object so that we can get access to the data later on again
        Category* category          = new Category();
        category->mName             = categoryName;
        category->mPropertyWidget   = propertyWidget;
        category->mWidget           = propertyWidget;
        category->mListWidgetItem   = categoryButton;

        // add the new category object to the categories array and return the property widget
        mCategories.Add(category);
        return propertyWidget;
    }


    // find category by name
    PreferencesWindow::Category* PreferencesWindow::FindCategoryByName(const char* categoryName) const
    {
        // get the number of categories and iterate through them
        const uint32 numCategories = mCategories.GetLength();
        for (uint32 i = 0; i < numCategories; ++i)
        {
            Category* category = mCategories[i];

            // compare the passed name with the current category and return if they are the same
            if (category->mName == categoryName)
            {
                return category;
            }
        }

        // in case we haven't found the category with the given name return nullptr
        return nullptr;
    }


    // new category icon pressed, change to the corresponding property browser
    void PreferencesWindow::ChangePage(QListWidgetItem* current, QListWidgetItem* previous)
    {
        if (current == nullptr)
        {
            current = previous;
        }

        mStackedWidget->setCurrentIndex(mCategoriesWidget->row(current));
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/PreferencesWindow.moc>
