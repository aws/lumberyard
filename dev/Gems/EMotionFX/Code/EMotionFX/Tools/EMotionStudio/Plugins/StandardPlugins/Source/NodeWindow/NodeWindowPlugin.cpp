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
#include "NodeWindowPlugin.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/NodeGroup.h>
#include <EMotionFX/Source/Material.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/NodeAttribute.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/NodeHierarchyWidget.h"
#include <MysticQt/Source/PropertyWidget.h>
#include <MysticQt/Source/SearchButton.h>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTreeWidget>
#include <QHeaderView>
#include <QIcon>


namespace EMStudio
{
    // constructor
    NodeWindowPlugin::NodeWindowPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mSelectCallback                 = nullptr;
        mUnselectCallback               = nullptr;
        mClearSelectionCallback         = nullptr;
        mDialogStack                    = nullptr;
    }


    // destructor
    NodeWindowPlugin::~NodeWindowPlugin()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
    }


    // clone the log window
    EMStudioPlugin* NodeWindowPlugin::Clone()
    {
        return new NodeWindowPlugin();
    }


    // init after the parent dock window has been created
    bool NodeWindowPlugin::Init()
    {
        //LogInfo("Initializing nodes window.");

        // create and register the command callbacks only (only execute this code once for all plugins)
        mSelectCallback                 = new CommandSelectCallback(false);
        mUnselectCallback               = new CommandUnselectCallback(false);
        mClearSelectionCallback         = new CommandClearSelectionCallback(false);

        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);

        // create the dialog stack
        assert(mDialogStack == nullptr);
        mDialogStack = new MysticQt::DialogStack();

        // add the node hierarchy
        mHierarchyWidget = new NodeHierarchyWidget(mDock, false);
        mHierarchyWidget->GetTreeWidget()->setMinimumWidth(100);
        mDialogStack->Add(mHierarchyWidget, "Hierarchy", false, true);

        // add the node attributes widget
        mPropertyWidget = new MysticQt::PropertyWidget();
        mPropertyWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mPropertyWidget->setMinimumHeight(215);

        //mPropertyBrowser = new PropertyBrowser(mDock);
        //mPropertyBrowser->Init(true);
        //mPropertyBrowser->setMinimumHeight(150);
        mDialogStack->Add(mPropertyWidget, "Node Attributes", false, true, true, false);

        // prepare the dock window
        mDock->SetContents(mDialogStack);
        mDock->setMinimumWidth(100);
        mDock->setMinimumHeight(100);

        // add functionality to the controls
        connect(mDock, SIGNAL(visibilityChanged(bool)), this, SLOT(VisibilityChanged(bool)));
        connect(mHierarchyWidget->GetTreeWidget(), SIGNAL(itemSelectionChanged()), this, SLOT(OnNodeChanged()));

        const QObject* filterStringEdit = mHierarchyWidget->GetSearchButton()->GetSearchEdit();
        connect(filterStringEdit, SIGNAL(textChanged(const QString&)), this, SLOT(FilterStringChanged(const QString&)));

        connect(mHierarchyWidget->GetDisplayNodesButton(), SIGNAL(clicked()), this, SLOT(UpdateVisibleNodeIndices()));
        connect(mHierarchyWidget->GetDisplayBonesButton(), SIGNAL(clicked()), this, SLOT(UpdateVisibleNodeIndices()));
        connect(mHierarchyWidget->GetDisplayMeshesButton(), SIGNAL(clicked()), this, SLOT(UpdateVisibleNodeIndices()));

        // reinit the dialog
        ReInit();

        return true;
    }


    // reinitialize the window
    void NodeWindowPlugin::ReInit()
    {
        const CommandSystem::SelectionList& selection       = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance*           actorInstance   = selection.GetSingleActorInstance();

        // reset the node name filter
        mHierarchyWidget->GetSearchButton()->GetSearchEdit()->setText("");
        //GetManager()->SetNodeNameFilterString(nullptr);

        mHierarchyWidget->GetTreeWidget()->clear();
        //mPropertyBrowser->GetPropertyBrowser()->clear();
        mPropertyWidget->Clear();

        if (actorInstance)
        {
            mHierarchyWidget->Update(actorInstance->GetID());
            FillActorInfo(actorInstance);

            // resize column to contents
            const uint32 numPropertyWidgetColumns = mPropertyWidget->columnCount();
            for (uint32 i = 0; i < numPropertyWidgetColumns; ++i)
            {
                mPropertyWidget->resizeColumnToContents(i);
            }
        }
    }


    void NodeWindowPlugin::OnNodeChanged()
    {
        // get access to the current selection list and clear the node selection
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        selection.ClearNodeSelection();
        mSelectedNodeIndices.Clear();

        MCore::Array<SelectionItem>& selectedItems = mHierarchyWidget->GetSelectedItems();

        EMotionFX::ActorInstance*   selectedInstance    = nullptr;
        EMotionFX::Node*            selectedNode        = nullptr;

        for (uint32 i = 0; i < selectedItems.GetLength(); ++i)
        {
            const uint32                actorInstanceID = selectedItems[i].mActorInstanceID;
            const char*                 nodeName        = selectedItems[i].GetNodeName();
            EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);

            if (actorInstance == nullptr)
            {
                continue;
            }

            EMotionFX::Actor*           actor   = actorInstance->GetActor();
            //const uint32  actorID = actor->GetID();
            EMotionFX::Node*            node    = actor->GetSkeleton()->FindNodeByName(nodeName);

            if (node && mHierarchyWidget->CheckIfNodeVisible(actorInstance, node))
            {
                if (selectedInstance == nullptr)
                {
                    selectedInstance = actorInstance;
                }
                if (selectedNode == nullptr)
                {
                    selectedNode = node;
                }

                // add the node to the node selection
                selection.AddNode(node);
                mSelectedNodeIndices.Add(node->GetNodeIndex());
            }
            else if (node == nullptr && actorInstance)
            {
                if (selectedInstance == nullptr)
                {
                    selectedInstance = actorInstance;
                }
                if (selectedNode == nullptr)
                {
                    selectedNode = node;
                }
            }
        }

        if (selectedInstance == nullptr)
        {
            GetManager()->SetSelectedNodeIndices(mSelectedNodeIndices);
            return;
        }

        //mPropertyBrowser->GetPropertyBrowser()->clear();
        mPropertyWidget->Clear();

        if (selectedNode)
        {
            // show the node information in the lower property widget
            FillNodeInfo(selectedNode, selectedInstance);
        }
        else
        {
            FillActorInfo(selectedInstance);
        }

        // resize column to contents
        const uint32 numPropertyWidgetColumns = mPropertyWidget->columnCount();
        for (uint32 i = 0; i < numPropertyWidgetColumns; ++i)
        {
            mPropertyWidget->resizeColumnToContents(i);
        }

        // pass the selected node indices to the manager
        GetManager()->SetSelectedNodeIndices(mSelectedNodeIndices);
    }


    void NodeWindowPlugin::FillNodeInfo(EMotionFX::Node* node, EMotionFX::ActorInstance* actorInstance)
    {
        uint32 i;

        uint32                      nodeIndex       = node->GetNodeIndex();
        EMotionFX::Actor*           actor           = actorInstance->GetActor();
        EMotionFX::TransformData*   transformData   = actorInstance->GetTransformData();

        mPropertyWidget->AddReadOnlyStringProperty("", "Node Name", node->GetName());

        // transform info
        MCore::Vector3 position = transformData->GetCurrentPose()->GetLocalTransform(nodeIndex).mPosition;
        mPropertyWidget->AddReadOnlyVector3Property("", "Position", position);

        MCore::Vector3 eulerRotation = transformData->GetCurrentPose()->GetLocalTransform(nodeIndex).mRotation.ToEuler();
        eulerRotation.x = MCore::Math::RadiansToDegrees(eulerRotation.x);
        eulerRotation.y = MCore::Math::RadiansToDegrees(eulerRotation.y);
        eulerRotation.z = MCore::Math::RadiansToDegrees(eulerRotation.z);
        mPropertyWidget->AddReadOnlyVector3Property("", "Rotation Euler (deg)", eulerRotation);

        MCore::Quaternion   rotationQuat = transformData->GetCurrentPose()->GetLocalTransform(nodeIndex).mRotation;
        AZ::Vector4     rotationVec4 = AZ::Vector4(rotationQuat.x, rotationQuat.y, rotationQuat.z, rotationQuat.w);
        mPropertyWidget->AddReadOnlyVector4Property("", "Rotation Quat", rotationVec4);

        EMFX_SCALECODE
        (
            //eulerRotation = transformData->GetCurrentPose()->GetLocalTransform(nodeIndex).mScaleRotation.ToEuler();
            //eulerRotation.x = Math::RadiansToDegrees(eulerRotation.x);
            //eulerRotation.y = Math::RadiansToDegrees(eulerRotation.y);
            //eulerRotation.z = Math::RadiansToDegrees(eulerRotation.z);

            //mPropertyWidget->AddReadOnlyVector3Property( "", "ScaleRot Euler Euler (deg)", eulerRotation );
            //Quaternion    scaleRotQuat = transformData->GetCurrentPose()->GetLocalTransform(nodeIndex).mScaleRotation;
            //Vector4       scaleRotVec4 = Vector4( scaleRotQuat.x, scaleRotQuat.y, scaleRotQuat.z, scaleRotQuat.w );
            //mPropertyWidget->AddReadOnlyVector4Property( "", "ScaleRot Quat", scaleRotVec4 );

            MCore::Vector3 scale = transformData->GetCurrentPose()->GetLocalTransform(nodeIndex).mScale;
            mPropertyWidget->AddReadOnlyVector3Property("", "Scale", scale);
        )

        // parent node
        EMotionFX::Node * parent = node->GetParentNode();
        if (parent)
        {
            mPropertyWidget->AddReadOnlyStringProperty("", "Parent", parent->GetName());
        }

        // the mirrored node
        const bool hasMirrorInfo = actor->GetHasMirrorInfo();
        if (hasMirrorInfo && actor->GetNodeMirrorInfo(nodeIndex).mSourceNode != MCORE_INVALIDINDEX16 && actor->GetNodeMirrorInfo(nodeIndex).mSourceNode != nodeIndex)
        {
            mPropertyWidget->AddReadOnlyStringProperty("", "Mirror", actor->GetSkeleton()->GetNode(actor->GetNodeMirrorInfo(nodeIndex).mSourceNode)->GetName());
        }

        // children
        const uint32 numChildren = node->GetNumChildNodes();
        MCore::String childNrString;
        if (numChildren > 0)
        {
            mPropertyWidget->AddReadOnlyIntProperty("", "Child Nodes", numChildren);

            for (i = 0; i < numChildren; ++i)
            {
                EMotionFX::Node* child = actor->GetSkeleton()->GetNode(node->GetChildIndex(i));
                childNrString.Format("Child #%d", i);

                mPropertyWidget->AddReadOnlyStringProperty("Child Nodes", childNrString.AsChar(), child->GetName());
            }

            mPropertyWidget->SetIsExpanded("Child Nodes", true);
        }

        // attributes
        const uint32 numAttributes = node->GetNumAttributes();
        if (numAttributes > 0)
        {
            mPropertyWidget->AddReadOnlyIntProperty("", "Attributes", numAttributes);
            for (i = 0; i < numAttributes; ++i)
            {
                EMotionFX::NodeAttribute* nodeAttribute = node->GetAttribute(i);
                mPropertyWidget->AddReadOnlyStringProperty("Attributes", "Type", nodeAttribute->GetTypeString());
            }
        }

        // meshes
        uint32 numMeshes = 0;
        const uint32 numLODLevels = actor->GetNumLODLevels();
        for (i = 0; i < numLODLevels; ++i)
        {
            if (actor->GetMesh(i, node->GetNodeIndex()))
            {
                numMeshes++;
            }
        }

        if (numMeshes > 0)
        {
            MCore::String groupName;
            mPropertyWidget->AddReadOnlyIntProperty("", "Meshes", numMeshes);
            for (i = 0; i < numLODLevels; ++i)
            {
                EMotionFX::Mesh* mesh = actor->GetMesh(i, node->GetNodeIndex());
                groupName.Format("Meshes.LOD%d", i);
                FillMeshInfo(groupName, mesh, node, actorInstance, i, false);
            }
            mPropertyWidget->SetIsExpanded("Meshes", true);
        }
        /*
            // collision meshes
            uint32 numCollisionMeshes = 0;
            for (i=0; i<numLODLevels; ++i)
            {
                if (actor->GetCollisionMesh(i, node->GetNodeIndex()))
                    numCollisionMeshes++;
            }

            if (numCollisionMeshes > 0)
            {
                for (i=0; i<numLODLevels; ++i)
                {
                    EMotionFX::Mesh* collisionMesh = actor->GetCollisionMesh(i, node->GetNodeIndex());
                    FillMeshInfo( "Collision Meshes", collisionMesh, node, actorInstance, i, true);
                }
                mPropertyWidget->SetIsExpanded( "Collision Meshes", true );
            }*/
    }


    void NodeWindowPlugin::FillMeshInfo(const char* groupName, EMotionFX::Mesh* mesh, EMotionFX::Node* node, EMotionFX::ActorInstance* actorInstance, int lodLevel, bool colMesh)
    {
        if (mesh == nullptr)
        {
            return;
        }

        uint32 i;
        MCore::String temp, tempGroupName;
        EMotionFX::Actor* actor = actorInstance->GetActor();

        // main mesh property
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "Geom LOD Level", lodLevel);
        mPropertyWidget->SetIsExpanded(groupName, true);

        // is deformable flag
        bool isDeformable = actor->CheckIfHasDeformableMesh(lodLevel, node->GetNodeIndex());
        mPropertyWidget->AddReadOnlyBoolProperty(groupName, "Deformable", isDeformable);
        mPropertyWidget->AddReadOnlyBoolProperty(groupName, "Is Collision Mesh", colMesh);

        // lod level
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "LOD Level", lodLevel);

        // vertices, indices and polygons etc.
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "Vertices", mesh->GetNumVertices());
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "Indices", mesh->GetNumIndices());
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "Polygons", mesh->GetNumPolygons());
        mPropertyWidget->AddReadOnlyBoolProperty(groupName, "Is Triangle Mesh", mesh->CheckIfIsTriangleMesh());
        mPropertyWidget->AddReadOnlyBoolProperty(groupName, "Is Quad Mesh", mesh->CheckIfIsQuadMesh());
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "Org Vertices", mesh->GetNumOrgVertices());

        if (mesh->GetNumOrgVertices() > 0)
        {
            mPropertyWidget->AddReadOnlyFloatSpinnerProperty(groupName, "Vertex Dupe Ratio", mesh->GetNumVertices() / (float)mesh->GetNumOrgVertices());
        }
        else
        {
            mPropertyWidget->AddReadOnlyFloatSpinnerProperty(groupName, "Vertex Dupe Ratio", 0.0f);
        }

        // skinning influences
        AZStd::vector<uint32> vertexCounts;
        const uint32 maxNumInfluences = mesh->CalcMaxNumInfluences(vertexCounts);

        tempGroupName = groupName;
        tempGroupName += ".Max Influences";
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "Max Influences", maxNumInfluences);
        mPropertyWidget->SetIsExpanded(tempGroupName.AsChar(), true);
        for (i = 0; i < maxNumInfluences; ++i)
        {
            mString.Format("%d Influence%s", i, (i != 1) ? "s" : "");
            temp.Format("%d vertices", vertexCounts[i]);
            mPropertyWidget->AddReadOnlyStringProperty(tempGroupName.AsChar(), mString.AsChar(), temp.AsChar());
        }

        // sub meshes
        const uint32 numSubMeshes = mesh->GetNumSubMeshes();

        tempGroupName = groupName;
        tempGroupName += ".Sub Meshes";
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "Sub Meshes", numSubMeshes);
        mPropertyWidget->SetIsExpanded(tempGroupName.AsChar(), true);

        for (i = 0; i < numSubMeshes; ++i)
        {
            EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(i);
            const uint32 numBones = subMesh->GetNumBones();

            mString.Format("Sub Mesh #%d", i);

            tempGroupName = groupName;
            tempGroupName += ".Sub Meshes.";
            tempGroupName += mString;

            mPropertyWidget->AddReadOnlyStringProperty(tempGroupName.AsChar(), "Material", actor->GetMaterial(lodLevel, subMesh->GetMaterial())->GetName());
            mPropertyWidget->AddReadOnlyIntProperty(tempGroupName.AsChar(), "Vertices", subMesh->GetNumVertices());
            mPropertyWidget->AddReadOnlyIntProperty(tempGroupName.AsChar(), "Indices", subMesh->GetNumIndices());
            mPropertyWidget->AddReadOnlyIntProperty(tempGroupName.AsChar(), "Polygons", subMesh->GetNumPolygons());

            // bones
            mPropertyWidget->AddReadOnlyIntProperty(tempGroupName.AsChar(), "Bones", numBones);
        }

        // vertex attribute layers
        const uint32 numVertexAttributeLayers = mesh->GetNumVertexAttributeLayers();

        tempGroupName = groupName;
        tempGroupName += ".Attribute Layers";
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "Attribute Layers", numVertexAttributeLayers);
        mPropertyWidget->SetIsExpanded(tempGroupName.AsChar(), true);

        for (i = 0; i < numVertexAttributeLayers; ++i)
        {
            EMotionFX::VertexAttributeLayer* attributeLayer = mesh->GetVertexAttributeLayer(i);

            const uint32 attributeLayerType = attributeLayer->GetType();
            switch (attributeLayerType)
            {
            case EMotionFX::Mesh::ATTRIB_POSITIONS:
                mString = "Vertex positions";
                break;
            case EMotionFX::Mesh::ATTRIB_NORMALS:
                mString = "Vertex normals";
                break;
            case EMotionFX::Mesh::ATTRIB_TANGENTS:
                mString = "Vertex tangents";
                break;
            case EMotionFX::Mesh::ATTRIB_UVCOORDS:
                mString = "Vertex uv coordinates";
                break;
            case EMotionFX::Mesh::ATTRIB_COLORS32:
                mString = "Vertex colors in 32-bits";
                break;
            case EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS:
                mString = "Original vertex numbers";
                break;
            case EMotionFX::Mesh::ATTRIB_COLORS128:
                mString = "Vertex colors in 128-bits";
                break;
            case EMotionFX::Mesh::ATTRIB_BITANGENTS:
                mString = "Vertex bitangents";
                break;
            default:
                mString.Format("Unknown data (TypeID=%d)", attributeLayerType);
            }

            if (attributeLayer->GetNameString().GetLength() > 0)
            {
                mString.FormatAdd(" [%s]", attributeLayer->GetName());
            }

            mPropertyWidget->AddReadOnlyStringProperty(tempGroupName.AsChar(), attributeLayer->GetTypeString(), mString.AsChar());
        }


        // shared vertex attribute layers
        const uint32 numSharedVertexAttributeLayers = mesh->GetNumSharedVertexAttributeLayers();

        tempGroupName = groupName;
        tempGroupName += ".Shared Attribute Layers";
        mPropertyWidget->AddReadOnlyIntProperty(groupName, "Shared Attribute Layers", numSharedVertexAttributeLayers);
        mPropertyWidget->SetIsExpanded(tempGroupName.AsChar(), true);

        for (i = 0; i < numSharedVertexAttributeLayers; ++i)
        {
            EMotionFX::VertexAttributeLayer* attributeLayer = mesh->GetSharedVertexAttributeLayer(i);

            const uint32 attributeLayerType = attributeLayer->GetType();
            switch (attributeLayerType)
            {
            case EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID:
                mString = "Skinning info";
                break;
            default:
                mString.Format("Unknown data (TypeID=%d)", attributeLayerType);
            }

            if (attributeLayer->GetNameString().GetLength() > 0)
            {
                mString.FormatAdd(" [%s]", attributeLayer->GetName());
            }

            mPropertyWidget->AddReadOnlyStringProperty(tempGroupName.AsChar(), attributeLayer->GetTypeString(), mString.AsChar());
        }
    }


    void NodeWindowPlugin::FillActorInfo(EMotionFX::ActorInstance* actorInstance)
    {
        //uint32 i;
        EMotionFX::Actor* actor = actorInstance->GetActor();

        // actor name
        /*MysticQt::PropertyWidget::Property* headerProperty = */ mPropertyWidget->AddReadOnlyStringProperty("", "Name", actor->GetName());

        // exporter information
        MCore::String sourceApplication         = actor->GetAttributeSet()->GetStringAttribute("sourceApplication");
        MCore::String originalFileName          = actor->GetAttributeSet()->GetStringAttribute("originalFileName");
        MCore::String exporterCompilationDate   = actor->GetAttributeSet()->GetStringAttribute("exporterCompilationDate");

        mPropertyWidget->AddReadOnlyStringProperty("", "Source Application", sourceApplication);
        mPropertyWidget->AddReadOnlyStringProperty("", "Original FileName", originalFileName);
        mPropertyWidget->AddReadOnlyStringProperty("", "Exporter Compilation Date", exporterCompilationDate);

        // unit type
        MCore::String fileUnitType = MCore::Distance::UnitTypeToString(actor->GetFileUnitType());
        mPropertyWidget->AddReadOnlyStringProperty("", "File Unit Type", fileUnitType.AsChar());

        // nodes
        const uint32 numNodes = actor->GetNumNodes();
        mPropertyWidget->AddReadOnlyIntProperty("", "Nodes", numNodes);

        // node groups
        const uint32 numNodeGroups = actor->GetNumNodeGroups();
        mPropertyWidget->AddReadOnlyIntProperty("", "Node Groups", numNodeGroups);
        for (uint32 i = 0; i < numNodeGroups; ++i)
        {
            EMotionFX::NodeGroup* nodeGroup = actor->GetNodeGroup(i);
            mString.Format("Node Group #%d", i);
            mPropertyWidget->AddReadOnlyStringProperty("Node Groups", mString.AsChar(), nodeGroup->GetName());

            // construct the full group name
            mTempGroupName = "Node Groups.";
            mTempGroupName += mString;

            // iterate over the nodes inside the node group
            const uint32 numGroupNodes = nodeGroup->GetNumNodes();
            for (uint32 j = 0; j < numGroupNodes; ++j)
            {
                uint16              nodeIndex   = nodeGroup->GetNode(j);
                EMotionFX::Node*    node        = actor->GetSkeleton()->GetNode(nodeIndex);
                mPropertyWidget->AddReadOnlyIntProperty(mTempGroupName.AsChar(), node->GetName(), j);
            }

            mPropertyWidget->SetIsExpanded("Node Groups", true);
        }

        mPropertyWidget->SetIsExpanded("Node Groups", true);

        // global mesh information
        const uint32 lodLevel = actorInstance->GetLODLevel();
        uint32 numPolygons, numVertices, numIndices;
        actor->CalcMeshTotals(lodLevel, &numPolygons, &numVertices, &numIndices);

        mPropertyWidget->AddReadOnlyIntProperty("", "Total Vertices", numVertices);
        mPropertyWidget->AddReadOnlyIntProperty("", "Total Indices", numIndices);
    }


    // reinit the window when it gets activated
    void NodeWindowPlugin::VisibilityChanged(bool isVisible)
    {
        if (isVisible)
        {
            ReInit();
        }
    }


    void NodeWindowPlugin::FilterStringChanged(const QString& text)
    {
        MCORE_UNUSED(text);
        //GetManager()->SetNodeNameFilterString( FromQtString(text).AsChar() );
        UpdateVisibleNodeIndices();
    }


    // update the array containing the visible node indices
    void NodeWindowPlugin::UpdateVisibleNodeIndices()
    {
        // reset the visible nodes array
        mVisibleNodeIndices.Clear();

        // get the currently selected actor instance
        const CommandSystem::SelectionList& selection       = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance*           actorInstance   = selection.GetSingleActorInstance();
        if (actorInstance == nullptr)
        {
            // make sure the empty visible nodes array gets passed to the manager, an empty array means all nodes are shown
            GetManager()->SetVisibleNodeIndices(mVisibleNodeIndices);
            return;
        }

        MCore::String filterString  = mHierarchyWidget->GetFilterString().ToLower();
        const bool showNodes        = mHierarchyWidget->GetDisplayNodes();
        const bool showBones        = mHierarchyWidget->GetDisplayBones();
        const bool showMeshes       = mHierarchyWidget->GetDisplayMeshes();

        // get access to the actor and the number of nodes
        EMotionFX::Actor* actor = actorInstance->GetActor();
        const uint32 numNodes = actor->GetNumNodes();

        // reserve memory for the visible node indices
        mVisibleNodeIndices.Reserve(numNodes);

        // extract the bones from the actor
        MCore::Array<uint32> boneList;
        actor->ExtractBoneList(actorInstance->GetLODLevel(), &boneList);

        // iterate through all nodes and check if the node is visible
        MCore::String nodeName;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);

            // get the node name and lower case it
            nodeName = node->GetNameString();
            nodeName.ToLower();

            const uint32        nodeIndex   = node->GetNodeIndex();
            EMotionFX::Mesh*    mesh        = actor->GetMesh(actorInstance->GetLODLevel(), nodeIndex);
            const bool          isMeshNode  = (mesh);
            const bool          isBone      = (boneList.Find(nodeIndex) != MCORE_INVALIDINDEX32);
            const bool          isNode      = (isMeshNode == false && isBone == false);

            if (((showMeshes && isMeshNode) ||
                 (showBones && isBone) ||
                 (showNodes && isNode)) &&
                (filterString.GetIsEmpty() || nodeName.Contains(filterString.AsChar())))
            {
                // this node is visible!
                mVisibleNodeIndices.Add(nodeIndex);
            }
        }

        // pass it over to the manager
        GetManager()->SetVisibleNodeIndices(mVisibleNodeIndices);
    }

    //-----------------------------------------------------------------------------------------
    // command callbacks
    //-----------------------------------------------------------------------------------------

    bool ReInitNodeWindowPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(NodeWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        NodeWindowPlugin* nodeWindow = (NodeWindowPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (nodeWindow->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            nodeWindow->ReInit();
        }

        return true;
    }

    bool NodeWindowPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)           { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeWindowPlugin(); }
    bool NodeWindowPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeWindowPlugin(); }
    bool NodeWindowPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)         { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeWindowPlugin(); }
    bool NodeWindowPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)            { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeWindowPlugin(); }
    bool NodeWindowPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeWindowPlugin(); }
    bool NodeWindowPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)      { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitNodeWindowPlugin(); }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/NodeWindow/NodeWindowPlugin.moc>