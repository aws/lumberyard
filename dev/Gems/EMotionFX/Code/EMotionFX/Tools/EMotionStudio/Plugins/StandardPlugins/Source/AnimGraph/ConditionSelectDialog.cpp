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
        m_selectedTypeId = AZ::TypeId::CreateNull();

        // set the window title
        setWindowTitle("Select A Condition");

        // create the layout
        QVBoxLayout* layout = new QVBoxLayout();

        // create the list widget
        m_listBox = new QListWidget();
        m_listBox->setAlternatingRowColors(true);

        // for all registered objects, add it to the list if it is a condition
        const AZStd::unordered_set<AZ::TypeId>& nodeObjectTypes = EMotionFX::AnimGraphObjectFactory::GetUITypes();
        m_typeIds.reserve(nodeObjectTypes.size());

        for (const AZ::TypeId& nodeObjectType : nodeObjectTypes)
        {
            EMotionFX::AnimGraphObject* object = EMotionFX::AnimGraphObjectFactory::Create(nodeObjectType);
            if (azrtti_istypeof<EMotionFX::AnimGraphTransitionCondition>(object))
            {
                // add it to the listbox
                QListWidgetItem* item = new QListWidgetItem(object->GetPaletteName(), m_listBox, static_cast<int>(m_typeIds.size()));
                m_listBox->addItem(item);
                m_typeIds.push_back(azrtti_typeid(object));
            }
            
            delete object;
        }

        // set the current item of the list widget
        m_listBox->setCurrentRow(0);

        // connect the list widget
        connect(m_listBox, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(OnItemDoubleClicked(QListWidgetItem*)));

        // add the list widget in the layout
        layout->addWidget(m_listBox);

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
        m_selectedTypeId = m_typeIds[item->type()];
        emit accept();
    }


    // create button has been pressed
    void ConditionSelectDialog::OnCreateButton()
    {
        // get the currently selected item
        QListWidgetItem* item = m_listBox->currentItem();
        if (item == nullptr)
        {
            m_selectedTypeId = AZ::TypeId::CreateNull();
            emit accept();
            return;
        }

        // get the condition type
        m_selectedTypeId = m_typeIds[item->type()];
        emit accept();
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ConditionSelectDialog.moc>
