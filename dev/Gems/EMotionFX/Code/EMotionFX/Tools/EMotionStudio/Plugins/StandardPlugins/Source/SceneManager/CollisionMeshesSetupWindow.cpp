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
#include "CollisionMeshesSetupWindow.h"
#include "EMotionFX/Source/ActorManager.h"
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/StringConversions.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/Mesh.h>
#include <QLabel>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QGraphicsDropShadowEffect>


namespace EMStudio
{
    // constructor
    CollisionMeshesSetupWindow::CollisionMeshesSetupWindow(QWidget* parent)
        : QDialog(parent)
    {
        // set the window title
        setWindowTitle("Collision Mesh Setup");

        // create the hierarchy widget
        mHierarchyWidget = new CollisionMeshesNodeHierarchyWidget(this);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout   = new QHBoxLayout();
        QPushButton* mOKButton      = new QPushButton("OK");
        QPushButton* mCancelButton  = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        // create and set the layout
        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget(mHierarchyWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        // connect the buttons
        connect(mOKButton, &QPushButton::clicked, this, &CollisionMeshesSetupWindow::accept);
        connect(mCancelButton, &QPushButton::clicked, this, &CollisionMeshesSetupWindow::reject);
        connect(this, &CollisionMeshesSetupWindow::accepted, this, &CollisionMeshesSetupWindow::OnAccept);
    }


    // destructor
    CollisionMeshesSetupWindow::~CollisionMeshesSetupWindow()
    {
    }


    // accepted signal
    void CollisionMeshesSetupWindow::OnAccept()
    {
        // get the actor instance ID and check if it's invalid
        const uint32 actorInstanceID = mHierarchyWidget->GetActorInstanceID();
        if (actorInstanceID == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // get the actor ID
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        const uint32 actorID = actorInstance->GetActor()->GetID();

        // create the command group
        MCore::CommandGroup commandGroup("Actor set collison meshes");
        AZStd::string tempString;

        // add each command to update the collision meshes of each LOD
        const MCore::Array<MCore::Array<AZStd::string> >& LODNodeList = mHierarchyWidget->GetLODNodeList();
        const uint32 numLOD = LODNodeList.GetLength();
        for (uint32 i = 0; i < numLOD; ++i)
        {
            // get the number of nodes
            const uint32 numNodes = LODNodeList[i].GetLength();

            // generate the node list
            AZStd::string nodeList;
            for (uint32 j = 0; j < numNodes; ++j)
            {
                nodeList += AZStd::string::format("%s;", LODNodeList[i][j].c_str());
            }

            // remove the semicolon at the end
            AzFramework::StringFunc::Strip(nodeList, MCore::CharacterConstants::semiColon, true /* case sensitive */, false /* beginning */, true /* ending */);

            // generate the command
            tempString = AZStd::string::format("ActorSetCollisionMeshes -actorID %i -lod %i -nodeList \"", actorID, i);
            tempString += nodeList;
            tempString += "\"";

            // add the command
            commandGroup.AddCommandString(tempString.c_str());
        }

        // execute the command group
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, tempString) == false)
        {
            MCore::LogError(tempString.c_str());
        }
    }
} // namespace EMStudio


#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/CollisionMeshesSetupWindow.moc>