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
#include "MorphTargetSelectionWindow.h"
#include "EMStudioManager.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/Mesh.h>
#include <QLabel>
#include <QSizePolicy>
#include <QPushButton>
#include <QVBoxLayout>


namespace EMStudio
{
    // constructor
    MorphTargetSelectionWindow::MorphTargetSelectionWindow(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Morph Target Selection Window");

        QVBoxLayout* layout = new QVBoxLayout();

        mListWidget = new QListWidget();
        mListWidget->setAlternatingRowColors(true);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mCancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        layout->addWidget(mListWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(mOKButton, SIGNAL(clicked()), this, SLOT(accept()));
        connect(mCancelButton, SIGNAL(clicked()), this, SLOT(reject()));
        connect(mListWidget, SIGNAL(itemSelectionChanged()), this, SLOT(OnSelectionChanged()));
    }


    // destructor
    MorphTargetSelectionWindow::~MorphTargetSelectionWindow()
    {
    }


    // called whenever the selection changes
    void MorphTargetSelectionWindow::OnSelectionChanged()
    {
        mSelection.Clear(false);

        // get the number of items and iterate through them
        const uint32 numItems = mListWidget->count();
        for (uint32 i = 0; i < numItems; ++i)
        {
            QListWidgetItem* item = mListWidget->item(i);

            // add the morph target id to the selection array in case the item is selected
            if (item->isSelected())
            {
                mSelection.Add(item->data(Qt::UserRole).toInt());
            }
        }
    }


    // refill the table with the current morph setup
    void MorphTargetSelectionWindow::Update(EMotionFX::MorphSetup* morphSetup, const MCore::Array<uint32>& selection)
    {
        if (morphSetup == nullptr)
        {
            return;
        }

        mListWidget->setSelectionMode(QListWidget::ExtendedSelection);

        mListWidget->blockSignals(true);

        // remove all old items first
        mListWidget->clear();

        // get the number of morph targets, iterate through and add them to the table
        const uint32 numMorphTargets = morphSetup->GetNumMorphTargets();
        for (uint32 i = 0; i < numMorphTargets; ++i)
        {
            EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(i);
            const uint32 morphTargetID = morphTarget->GetID();

            // create the list item and fill it
            QListWidgetItem* item = new QListWidgetItem();
            item->setText(morphTarget->GetName());
            item->setData(Qt::UserRole, morphTargetID);

            // add the item to the list widget
            mListWidget->addItem(item);

            // check if we need to select the item
            if (selection.Find(morphTargetID) != MCORE_INVALIDINDEX32)
            {
                item->setSelected(true);
            }
        }

        mListWidget->blockSignals(false);

        mSelection = selection;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MorphTargetSelectionWindow.moc>