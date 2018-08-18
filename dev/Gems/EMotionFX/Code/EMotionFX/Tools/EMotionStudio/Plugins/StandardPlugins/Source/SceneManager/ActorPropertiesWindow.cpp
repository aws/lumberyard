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
#include "ActorPropertiesWindow.h"
#include "SceneManagerPlugin.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QWindow>
#include <MysticQt/Source/ButtonGroup.h>
#include <MCore/Source/Compare.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include "MirrorSetupWindow.h"


namespace EMStudio
{
    // constructor
    ActorPropertiesWindow::ActorPropertiesWindow(QWidget* parent, SceneManagerPlugin* plugin)
        : QWidget(parent)
    {
        mExcludedNodesSelectionWindow       = nullptr;
        mPlugin                             = plugin;
        mActor                              = nullptr;
        mActorInstance                      = nullptr;
        mSelectionList                      = nullptr;
        mMirrorSetupLink                    = nullptr;
        mMirrorSetupWindow                  = nullptr;
        mCollisionMeshesSetupLink           = nullptr;
    }


    // destructor
    ActorPropertiesWindow::~ActorPropertiesWindow()
    {
        delete mSelectionList;
    }


    // init after the parent dock window has been created
    void ActorPropertiesWindow::Init()
    {
        QVBoxLayout* mainVerticalLayout = new QVBoxLayout();
        mainVerticalLayout->setMargin(0);
        setLayout(mainVerticalLayout);

        QGridLayout* layout = new QGridLayout();
        uint32 rowNr = 0;
        layout->setMargin(0);
        layout->setVerticalSpacing(0);
        layout->setAlignment(Qt::AlignLeft);
        mainVerticalLayout->addLayout(layout);

        // actor name
        rowNr = 0;
        layout->addWidget(new QLabel("Actor Name:"), rowNr, 0);
        mNameEdit = new QLineEdit();
        connect(mNameEdit, SIGNAL(editingFinished()), this, SLOT(NameEditChanged()));
        layout->addWidget(mNameEdit, rowNr, 1);

        // add the motion extraction node selection
        rowNr++;
        QHBoxLayout* extractNodeLayout = new QHBoxLayout();
        extractNodeLayout->setDirection(QBoxLayout::LeftToRight);
        extractNodeLayout->setMargin(0);
        mMotionExtractionNode = new MysticQt::LinkWidget();
        mMotionExtractionNode->setMinimumHeight(20);
        mMotionExtractionNode->setMaximumHeight(20);
        layout->addWidget(new QLabel("Motion Extraction Node:"), rowNr, 0);
        mResetMotionExtractionNodeButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mResetMotionExtractionNodeButton, "/Images/Icons/Remove.png",   "Reset selection");
        extractNodeLayout->addWidget(mMotionExtractionNode);
        extractNodeLayout->addWidget(mResetMotionExtractionNodeButton);

        // add the find best match button
        mFindBestMatchButton = new QPushButton("Find Best Match");
        mFindBestMatchButton->setMinimumHeight(20);
        mFindBestMatchButton->setMaximumHeight(20);
        mFindBestMatchButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        extractNodeLayout->addWidget(mFindBestMatchButton);
        connect(mFindBestMatchButton, SIGNAL(clicked()), this, SLOT(OnFindBestMatchingNode()));

        QWidget* dummyWidget = new QWidget();
        dummyWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
        extractNodeLayout->addWidget(dummyWidget);

        layout->addLayout(extractNodeLayout, rowNr, 1);

        // motion extraction node
        connect(mResetMotionExtractionNodeButton, SIGNAL(clicked()), this, SLOT(ResetMotionExtractionNode()));
        connect(mMotionExtractionNode, SIGNAL(clicked()), this, SLOT(OnSelectMotionExtractionNode()));


        // add the retarget root selection
        rowNr++;
        QHBoxLayout* retargetRootNodeLayout = new QHBoxLayout();
        retargetRootNodeLayout->setDirection(QBoxLayout::LeftToRight);
        retargetRootNodeLayout->setMargin(0);
        mRetargetRootNode = new MysticQt::LinkWidget();
        mRetargetRootNode->setMinimumHeight(20);
        mRetargetRootNode->setMaximumHeight(20);
        layout->addWidget(new QLabel("Retarget Root Node:"), rowNr, 0);
        mResetRetargetRootNodeButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mResetRetargetRootNodeButton, "/Images/Icons/Remove.png",   "Reset selection");
        retargetRootNodeLayout->addWidget(mRetargetRootNode);
        retargetRootNodeLayout->addWidget(mResetRetargetRootNodeButton);

        dummyWidget = new QWidget();
        dummyWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
        retargetRootNodeLayout->addWidget(dummyWidget);

        layout->addLayout(retargetRootNodeLayout, rowNr, 1);
        connect(mResetRetargetRootNodeButton, SIGNAL(clicked()), this, SLOT(ResetRetargetRootNode()));
        connect(mRetargetRootNode, SIGNAL(clicked()), this, SLOT(OnSelectRetargetRootNode()));


        // bounding box nodes
        rowNr++;
        QHBoxLayout* excludedNodesLayout = new QHBoxLayout();
        excludedNodesLayout->setDirection(QBoxLayout::LeftToRight);
        excludedNodesLayout->setMargin(0);
        mExcludedNodesLink = new MysticQt::LinkWidget();
        layout->addWidget(new QLabel("Excluded From Bounds:"), rowNr, 0);
        mResetExcludedNodesButton = new QPushButton();
        EMStudioManager::MakeTransparentButton(mResetExcludedNodesButton, "/Images/Icons/Remove.png",  "Reset selection");
        excludedNodesLayout->addWidget(mExcludedNodesLink);
        excludedNodesLayout->addWidget(mResetExcludedNodesButton);

        dummyWidget = new QWidget();
        dummyWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
        excludedNodesLayout->addWidget(dummyWidget);

        layout->addLayout(excludedNodesLayout, rowNr, 1);

        connect(mResetExcludedNodesButton,                                                 SIGNAL(clicked()),                                      this, SLOT(ResetExcludedNodes()));
        connect(mExcludedNodesLink,                                                        SIGNAL(clicked()),                                      this, SLOT(OnSelectExcludedNodes()));

        // collision meshes nodes
        rowNr++;
        mCollisionMeshesSetupWindow = new CollisionMeshesSetupWindow(this);
        mCollisionMeshesSetupLink = new MysticQt::LinkWidget();
        mCollisionMeshesSetupLink->setMinimumHeight(20);
        mCollisionMeshesSetupLink->setMaximumHeight(20);
        mCollisionMeshesSetupLink->setText("Click to setup");
        layout->addWidget(new QLabel("Collision Mesh Setup:"), rowNr, 0);
        layout->addWidget(mCollisionMeshesSetupLink, rowNr, 1);
        connect(mCollisionMeshesSetupLink, SIGNAL(clicked()), this, SLOT(OnCollisionMeshesSetup()));

        // mirror setup
        rowNr++;
        mMirrorSetupWindow = new MirrorSetupWindow(mPlugin->GetDockWidget(), mPlugin);
        mMirrorSetupLink = new MysticQt::LinkWidget();
        mMirrorSetupLink->setMinimumHeight(20);
        mMirrorSetupLink->setMaximumHeight(20);
        mMirrorSetupLink->setText("Click to setup");
        layout->addWidget(new QLabel("Mirror Setup:"), rowNr, 0);
        layout->addWidget(mMirrorSetupLink, rowNr, 1);
        connect(mMirrorSetupLink, SIGNAL(clicked()), this, SLOT(OnMirrorSetup()));

        UpdateInterface();
    }


    void ActorPropertiesWindow::UpdateInterface()
    {
        mActor         = nullptr;
        mActorInstance = nullptr;

        EMotionFX::ActorInstance*   actorInstance   = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        EMotionFX::Actor*           actor           = GetCommandManager()->GetCurrentSelection().GetSingleActor();

        // in case we have selected a single actor instance
        if (actorInstance)
        {
            mActorInstance  = actorInstance;
            mActor          = actorInstance->GetActor();
        }
        // in case we have selected a single actor
        else if (actor)
        {
            mActor = actor;

            const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
            for (uint32 i = 0; i < numActorInstances; ++i)
            {
                EMotionFX::ActorInstance* currentInstance = EMotionFX::GetActorManager().GetActorInstance(i);
                if (currentInstance->GetActor() == actor)
                {
                    mActorInstance = currentInstance;
                    break;
                }
            }
        }

        mMirrorSetupWindow->Reinit();

        if (mActorInstance == nullptr || mActor == nullptr)
        {
            // reset data and disable interface elements
            mMotionExtractionNode->setText(GetDefaultNodeSelectionLabelText());
            mMotionExtractionNode->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            mMotionExtractionNode->setEnabled(false);
            mResetMotionExtractionNodeButton->setVisible(false);
            mFindBestMatchButton->setVisible(false);

            mRetargetRootNode->setText(GetDefaultNodeSelectionLabelText());
            mRetargetRootNode->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            mRetargetRootNode->setEnabled(false);
            mResetRetargetRootNodeButton->setVisible(false);

            // nodes excluded from bounding volume calculations
            mExcludedNodesLink->setText(GetDefaultExcludeBoundNodesLabelText());
            mExcludedNodesLink->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            mExcludedNodesLink->setEnabled(false);
            mResetExcludedNodesButton->setVisible(false);

            // actor name
            mNameEdit->setEnabled(false);
            mNameEdit->setText("");

            mCollisionMeshesSetupLink->setEnabled(false);
            mMirrorSetupLink->setEnabled(false);

            return;
        }

        mCollisionMeshesSetupLink->setEnabled(true);
        mMirrorSetupLink->setEnabled(true);

        // get the motion extraction node
        EMotionFX::Node* extractionNode = mActor->GetMotionExtractionNode();
        if (extractionNode == nullptr)
        {
            mMotionExtractionNode->setEnabled(true);
            mMotionExtractionNode->setText(GetDefaultNodeSelectionLabelText());
            mResetMotionExtractionNodeButton->setVisible(false);
        }
        else
        {
            mMotionExtractionNode->setEnabled(true);
            mMotionExtractionNode->setText(extractionNode->GetName());
            mResetMotionExtractionNodeButton->setVisible(true);
        }

        // find best matching motion extraction node button, show it only when it differs from the currently selected node
        EMotionFX::Node* bestMatchingNode = mActor->FindBestMotionExtractionNode();
        if (bestMatchingNode && mActor->GetMotionExtractionNode() != bestMatchingNode)
        {
            mFindBestMatchButton->setVisible(true);
        }
        else
        {
            mFindBestMatchButton->setVisible(false);
        }

        mMotionExtractionNode->setToolTip("The node used to drive the character's movement and rotation");
        mMotionExtractionNode->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);


        // get the retarget root node
        EMotionFX::Node* retargetRootNode = mActor->GetRetargetRootNode();
        if (!retargetRootNode)
        {
            mRetargetRootNode->setEnabled(true);
            mRetargetRootNode->setText(GetDefaultNodeSelectionLabelText());
            mResetRetargetRootNodeButton->setVisible(false);
        }
        else
        {
            mRetargetRootNode->setEnabled(true);
            mRetargetRootNode->setText(retargetRootNode->GetName());
            mResetRetargetRootNodeButton->setVisible(true);
        }

        mRetargetRootNode->setToolTip("The root node that will use special handling when retargeting. Z must point up.");
        mRetargetRootNode->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);


        // nodes excluded from bounding volume calculations
        const uint32 calcNumExcludedNodes = CalcNumExcludedNodesFromBounds(mActor);
        if (calcNumExcludedNodes == 0)
        {
            mExcludedNodesLink->setText(GetDefaultExcludeBoundNodesLabelText());
            mResetExcludedNodesButton->setVisible(false);
        }
        else
        {
            const QString textLink = QString("%1 nodes").arg(calcNumExcludedNodes);
            mExcludedNodesLink->setText(textLink);
            mResetExcludedNodesButton->setVisible(true);
        }
        mExcludedNodesLink->setToolTip("Nodes that are excluded from bounding volume calculations");
        mExcludedNodesLink->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        mExcludedNodesLink->setEnabled(true);

        // actor name
        mNameEdit->setEnabled(true);
        mNameEdit->setText(mActor->GetName());
    }


    void ActorPropertiesWindow::ResetNode(const char* parameterNodeType)
    {
        if (mActorInstance == nullptr)
        {
            AZ_Warning("EMotionFX", false, "Cannot reset '%s'. Please select an actor instance first.", parameterNodeType);
            return;
        }

        EMotionFX::Actor* actor = mActorInstance->GetActor();
        const uint32 actorID = actor->GetID();

        // execute the command
        const AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -%s \"$NULL$\"", actorID, parameterNodeType);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void ActorPropertiesWindow::ResetMotionExtractionNode()
    {
        ResetNode("motionExtractionNodeName");
    }


    void ActorPropertiesWindow::ResetRetargetRootNode()
    {
        ResetNode("retargetRootNodeName");
    }


    void ActorPropertiesWindow::OnSelectMotionExtractionNode()
    {
        if (mActorInstance == nullptr)
        {
            AZ_Warning("EMotionFX", false, "Cannot open node selection window. Please select an actor instance first.");
            return;
        }

        // get the actor from the actor instance
        mActor = mActorInstance->GetActor();

        // get the current node used
        EMotionFX::Node* node = nullptr;
        node = mActor->GetMotionExtractionNode();

        // show the node selection window
        if (mSelectionList == nullptr)
        {
            mSelectionList = new CommandSystem::SelectionList();
        }
        mSelectionList->Clear();
        if (node)
        {
            mSelectionList->AddNode(node);
        }

        NodeSelectionWindow motionExtractionNodeSelectionWindow(this, true);

        connect(motionExtractionNodeSelectionWindow.GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(OnMotionExtractionNodeSelected(MCore::Array<SelectionItem>)));

        motionExtractionNodeSelectionWindow.Update(mActorInstance->GetID(), mSelectionList);
        motionExtractionNodeSelectionWindow.exec();

        disconnect(motionExtractionNodeSelectionWindow.GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(OnMotionExtractionNodeSelected(MCore::Array<SelectionItem>)));
    }


    void ActorPropertiesWindow::OnSelectRetargetRootNode()
    {
        if (!mActorInstance)
        {
            AZ_Warning("EMotionFX", false, "Cannot open node selection window. Please select an actor instance first.");
            return;
        }

        mActor = mActorInstance->GetActor();
        EMotionFX::Node* node = nullptr;
        node = mActor->GetMotionExtractionNode();

        // show the node selection window
        if (!mSelectionList)
        {
            mSelectionList = new CommandSystem::SelectionList();
        }

        mSelectionList->Clear();
        if (node)
        {
            mSelectionList->AddNode(node);
        }

        NodeSelectionWindow nodeSelectionWindow(this, true);
        connect(nodeSelectionWindow.GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(OnRetargetRootNodeSelected(MCore::Array<SelectionItem>)));
        nodeSelectionWindow.Update(mActorInstance->GetID(), mSelectionList);
        nodeSelectionWindow.exec();

        disconnect(nodeSelectionWindow.GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(OnRetargetRootNodeSelected(MCore::Array<SelectionItem>)));
    }

    void ActorPropertiesWindow::GetNodeName(const MCore::Array<SelectionItem>& selection, AZStd::string* outNodeName, uint32* outActorID)
    {
        // check if selection is valid
        if (selection.GetLength() != 1 || selection[0].GetNodeNameString().empty())
        {
            AZ_Warning("EMotionFX", false, "Cannot adjust motion extraction node. No valid node selected.");
            return;
        }

        const uint32                actorInstanceID = selection[0].mActorInstanceID;
        const char*                 nodeName        = selection[0].GetNodeName();
        EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            return;
        }

        EMotionFX::Actor* actor = actorInstance->GetActor();
        *outActorID     = actor->GetID();
        *outNodeName    = nodeName;
    }


    void ActorPropertiesWindow::OnMotionExtractionNodeSelected(MCore::Array<SelectionItem> selection)
    {
        // get the selected node name
        uint32 actorID;
        AZStd::string nodeName;
        GetNodeName(selection, &nodeName, &actorID);

        // create the command group
        MCore::CommandGroup commandGroup("Adjust motion extraction node");

        // adjust the actor
        AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -motionExtractionNodeName \"%s\"", actorID, nodeName.c_str());
        commandGroup.AddCommandString(command);

        // execute the command group
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    void ActorPropertiesWindow::OnRetargetRootNodeSelected(MCore::Array<SelectionItem> selection)
    {
        // get the selected node name
        uint32 actorID;
        AZStd::string nodeName;
        GetNodeName(selection, &nodeName, &actorID);

        MCore::CommandGroup commandGroup("Adjust retarget root node");
        AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -retargetRootNodeName \"%s\"", actorID, nodeName.c_str());
        commandGroup.AddCommandString(command);

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // automatically find the best matching motion extraction node and set it
    void ActorPropertiesWindow::OnFindBestMatchingNode()
    {
        // check if the actor is invalid
        if (mActor == nullptr)
        {
            return;
        }

        // find the best motion extraction node
        EMotionFX::Node* bestMatchingNode = mActor->FindBestMotionExtractionNode();
        if (bestMatchingNode == nullptr)
        {
            return;
        }

        // create the command group
        MCore::CommandGroup commandGroup("Adjust motion extraction node");

        // adjust the actor
        const AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -motionExtractionNodeName \"%s\"", mActor->GetID(), bestMatchingNode->GetName());
        commandGroup.AddCommandString(command);

        // execute the command group
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    EMotionFX::Node* ActorPropertiesWindow::GetNode(NodeHierarchyWidget* hierarchyWidget)
    {
        AZStd::vector<SelectionItem>& selectedItems = hierarchyWidget->GetSelectedItems();
        if (selectedItems.size() != 1)
        {
            return nullptr;
        }

        const uint32                actorInstanceID = selectedItems[0].mActorInstanceID;
        const char*                 nodeName        = selectedItems[0].GetNodeName();
        EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            return nullptr;
        }

        EMotionFX::Actor* actor = actorInstance->GetActor();
        return actor->GetSkeleton()->FindNodeByName(nodeName);
    }


    // check if we have any node that is excluded from the bounding volume calculation
    uint32 ActorPropertiesWindow::CalcNumExcludedNodesFromBounds(EMotionFX::Actor* actor)
    {
        // check if the actor is valid
        if (actor == nullptr)
        {
            return 0;
        }

        uint32 result = 0;

        // get the number of nodes and iterate through them
        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // check if the current node is excluded from the bounds calculations
            if (actor->GetSkeleton()->GetNode(i)->GetIncludeInBoundsCalc() == false)
            {
                result++;
            }
        }

        // return the number of nodes that are excluded in the bounding volume calculations
        return result;
    }


    // actor name changed
    void ActorPropertiesWindow::NameEditChanged()
    {
        if (mActor == nullptr)
        {
            return;
        }

        // execute the command
        const AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -name \"%s\"", mActor->GetID(), mNameEdit->text().toUtf8().data());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // show the node selection window
    void ActorPropertiesWindow::OnSelectExcludedNodes()
    {
        // check if the actor is valid
        if (mActorInstance == nullptr)
        {
            return;
        }

        // get the number of nodes and iterate through them
        const uint32 numNodes = mActor->GetNumNodes();
        mOldExcludedNodeSelection.Clear();
        mOldExcludedNodeSelection.Reserve(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = mActor->GetSkeleton()->GetNode(i);

            if (node->GetIncludeInBoundsCalc() == false)
            {
                mOldExcludedNodeSelection.Add(node->GetID());
            }
        }

        // fill in a selection list with the excluded nodes so that the node selection window can preselect the current ones
        PrepareExcludedNodeSelectionList(mActor, &mExcludedNodeSelectionList);

        // show the node selection window
        NodeSelectionWindow excludedNodesSelectionWindow(this, false);
        mExcludedNodesSelectionWindow = &excludedNodesSelectionWindow;

        connect(excludedNodesSelectionWindow.GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(OnSelectExcludedNodes(MCore::Array<SelectionItem>)));
        connect(&excludedNodesSelectionWindow, SIGNAL(rejected()), this, SLOT(OnCancelExcludedNodes()));
        connect(excludedNodesSelectionWindow.GetNodeHierarchyWidget()->GetTreeWidget(), SIGNAL(itemSelectionChanged()), this, SLOT(OnExcludeNodeSelectionChanged()));

        excludedNodesSelectionWindow.Update(mActorInstance->GetID(), &mExcludedNodeSelectionList);
        excludedNodesSelectionWindow.exec();

        disconnect(excludedNodesSelectionWindow.GetNodeHierarchyWidget(), SIGNAL(OnSelectionDone(MCore::Array<SelectionItem>)), this, SLOT(OnSelectExcludedNodes(MCore::Array<SelectionItem>)));
        disconnect(&excludedNodesSelectionWindow, SIGNAL(rejected()), this, SLOT(OnCancelExcludedNodes()));
        disconnect(excludedNodesSelectionWindow.GetNodeHierarchyWidget()->GetTreeWidget(), SIGNAL(itemSelectionChanged()), this, SLOT(OnExcludeNodeSelectionChanged()));
        mExcludedNodesSelectionWindow = nullptr;
    }


    // walk over the actor nodes and check which of them we want to exclude from the bounding volume calculations
    void ActorPropertiesWindow::PrepareExcludedNodeSelectionList(EMotionFX::Actor* actor, CommandSystem::SelectionList* outSelectionList)
    {
        // reset the resulting selection list
        outSelectionList->Clear();

        // check if the actor is valid
        if (actor == nullptr)
        {
            return;
        }

        // get the number of nodes and iterate through them
        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            if (node->GetIncludeInBoundsCalc() == false)
            {
                outSelectionList->AddNode(node);
            }
        }
    }


    // select the nodes that should be excluded from the bounding volume calculations
    void ActorPropertiesWindow::OnSelectExcludedNodes(MCore::Array<SelectionItem> selection)
    {
        if (mActor == nullptr)
        {
            return;
        }

        AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -nodesExcludedFromBounds \"", mActor->GetID());

        // prepare the nodes excluded from bounds string
        uint32 addedItems = 0;
        const uint32 numSelectedItems = selection.GetLength();
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            // find the node based on the selection item and skip in case it can't be found
            EMotionFX::Node* node = mActor->GetSkeleton()->FindNodeByName(selection[i].GetNodeName());
            if (node == nullptr)
            {
                continue;
            }

            if (addedItems > 0)
            {
                command += ';';
            }

            command += node->GetName();
            addedItems++;
        }

        // construct the command
        command += "\" -nodeAction \"select\"";

        // reset the changes we did so that the undo data can be stored correctly
        SetToOldExcludeNodeSelection();

        // execute the command
        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // called when the selection got changed while the window is still opened
    void ActorPropertiesWindow::OnExcludeNodeSelectionChanged()
    {
        // check if the actor is valid
        if (mActor == nullptr)
        {
            return;
        }

        // get some infos from the selection window
        NodeHierarchyWidget* hierarchyWidget = mExcludedNodesSelectionWindow->GetNodeHierarchyWidget();
        hierarchyWidget->UpdateSelection();
        AZStd::vector<SelectionItem>& selectedItems = hierarchyWidget->GetSelectedItems();

        // get the number of nodes in the actor and the current lod level
        const uint32 numNodes = mActor->GetNumNodes();

        // disable all nodes first
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mActor->GetSkeleton()->GetNode(i)->SetIncludeInBoundsCalc(true);
        }

        // iterate through the selected items and enable these nodes
        for (const SelectionItem& selectedItem : selectedItems)
        {
            EMotionFX::Node* node = mActor->GetSkeleton()->FindNodeByName(selectedItem.GetNodeName());
            if (node)
            {
                node->SetIncludeInBoundsCalc(false);
            }
        }
    }


    // set the actor exclude nodes back to what it was before opening the node selection window
    void ActorPropertiesWindow::SetToOldExcludeNodeSelection()
    {
        // check if the actor is valid
        if (mActor == nullptr)
        {
            return;
        }

        // get the number of nodes in the actor and the current lod level
        const uint32 numNodes = mActor->GetNumNodes();

        // disable all nodes first
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mActor->GetSkeleton()->GetNode(i)->SetIncludeInBoundsCalc(true);
        }

        // iterate through the selected items and enable these nodes
        const uint32 numExcludedNodes = mOldExcludedNodeSelection.GetLength();
        for (uint32 i = 0; i < numExcludedNodes; ++i)
        {
            EMotionFX::Node* node = mActor->GetSkeleton()->FindNodeByID(mOldExcludedNodeSelection[i]);
            if (node)
            {
                node->SetIncludeInBoundsCalc(false);
            }
        }
    }


    // selecting excluded nodes canceled
    void ActorPropertiesWindow::OnCancelExcludedNodes()
    {
        // set it back to what it was before opening the node selection window
        SetToOldExcludeNodeSelection();
    }


    // reset the nodes that should be excluded from the bounding volume calculations
    void ActorPropertiesWindow::ResetExcludedNodes()
    {
        if (mActor == nullptr)
        {
            return;
        }

        const AZStd::string command = AZStd::string::format("AdjustActor -actorID %i -nodesExcludedFromBounds \"\" -nodeAction \"select\"", mActor->GetID());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // open the mirror setup
    void ActorPropertiesWindow::OnMirrorSetup()
    {
        if (mMirrorSetupWindow)
        {
            mMirrorSetupWindow->exec();
        }
    }


    void ActorPropertiesWindow::OnCollisionMeshesSetup()
    {
        mCollisionMeshesSetupWindow->Update(mActorInstance->GetID());
        mCollisionMeshesSetupWindow->exec();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/ActorPropertiesWindow.moc>