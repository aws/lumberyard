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

// include required headers
#include "ConditionSelectDialog.h"
#include "AnimGraphPlugin.h"
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>


namespace EMStudio
{
    // constructor
    ConditionSelectDialog::ConditionSelectDialog(QWidget* parent)
        : QDialog(parent)
    {
        // set the selected condition type
        mSelectedConditionType = MCORE_INVALIDINDEX32;

        // set the window title
        setWindowTitle("Select A Condition");

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // create the list widget
        mListBox = new QListWidget();
        mListBox->setAlternatingRowColors(true);

        // for all registered objects, add it to the list if it is a condition
        const size_t numObjects = EMotionFX::GetAnimGraphManager().GetObjectFactory()->GetNumRegisteredObjects();
        for (size_t i = 0; i < numObjects; ++i)
        {
            // get the object
            EMotionFX::AnimGraphObject* object = EMotionFX::GetAnimGraphManager().GetObjectFactory()->GetRegisteredObject(i);

            // only include transition conditions
            if (object->GetBaseType() != EMotionFX::AnimGraphTransitionCondition::BASETYPE_ID)
            {
                continue;
            }

            // add it to the listbox
            QListWidgetItem* item = new QListWidgetItem(object->GetPaletteName(), mListBox, static_cast<int>(i));
            mListBox->addItem(item);
        }

        // set the current item of the list widget
        mListBox->setCurrentRow(0);

        // connect the list widget
        connect(mListBox, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(OnItemDoubleClicked(QListWidgetItem*)));

        // add the list widget in the layout
        layout->addWidget(mListBox);

        // create the buttons layout
        QHBoxLayout* buttonsLayout = new QHBoxLayout();
        QPushButton* addButton = new QPushButton("Add Condition");
        QPushButton* cancelButton = new QPushButton("Cancel");
        buttonsLayout->addWidget(addButton);
        buttonsLayout->addWidget(cancelButton);
        layout->addLayout(buttonsLayout);
        connect(addButton, SIGNAL(clicked()), this, SLOT(OnCreateButton()));
        connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

        // set the layout
        setLayout(layout);

        setMinimumWidth(200);
        setMinimumHeight(300);
    }


    // destructor
    ConditionSelectDialog::~ConditionSelectDialog()
    {
    }


    // when double click on an item
    void ConditionSelectDialog::OnItemDoubleClicked(QListWidgetItem* item)
    {
        mSelectedConditionType = item->type();
        emit accept();
    }


    // create button has been pressed
    void ConditionSelectDialog::OnCreateButton()
    {
        // get the currently selected item
        QListWidgetItem* item = mListBox->currentItem();
        if (item == nullptr)
        {
            mSelectedConditionType = MCORE_INVALIDINDEX32;
            emit accept();
            return;
        }

        // get the condition type
        mSelectedConditionType = item->type();
        emit accept();
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ConditionSelectDialog.moc>
