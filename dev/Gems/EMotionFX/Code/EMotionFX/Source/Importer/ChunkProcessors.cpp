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

#include "../EMotionFXConfig.h"
#include <EMotionFX/Source/Parameter/GroupParameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/CDF97Wavelet.h>
#include <MCore/Source/D4Wavelet.h>
#include <MCore/Source/Distance.h>
#include <MCore/Source/HaarWavelet.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <MCore/Source/StringConversions.h>
#include "../KeyTrackLinear.h"
#include "ChunkProcessors.h"
#include "Importer.h"
#include "../EventManager.h"
#include "../Node.h"
#include "../NodeGroup.h"
#include "../Motion.h"
#include "../MotionEventTrack.h"
#include "../MotionSet.h"
#include "../MotionManager.h"
#include "../SkeletalMotion.h"
#include "../WaveletSkeletalMotion.h"
#include "../SkeletalSubMotion.h"
#include "../Actor.h"
#include "../Mesh.h"
#include "../MeshDeformerStack.h"
#include "../SoftSkinDeformer.h"
#include "../SubMesh.h"
#include "../Material.h"
#include "../StandardMaterial.h"
#include "../SkinningInfoVertexAttributeLayer.h"
#include "../SoftSkinManager.h"
#include "../DualQuatSkinDeformer.h"
#include "../VertexAttributeLayerAbstractData.h"
#include "../MorphTarget.h"
#include "../MorphSetup.h"
#include "../MorphMeshDeformer.h"
#include "../MorphSubMotion.h"
#include "../MorphTargetStandard.h"
#include "../MotionEventTable.h"
#include "../Skeleton.h"
#include "../AnimGraph.h"
#include "../AnimGraphGameControllerSettings.h"
#include "../AnimGraphManager.h"
#include "../AnimGraphObjectFactory.h"
#include "../AnimGraphNode.h"
#include "../AnimGraphNodeGroup.h"
#include "../BlendTree.h"
#include "../AnimGraphStateMachine.h"
#include "../AnimGraphStateTransition.h"
#include "../AnimGraphTransitionCondition.h"
#include "../MCore/Source/Endian.h"
#include <AzCore/Math/PackedVector3.h>
#include <MCore/Source/MCoreSystem.h>
#include "../NodeMap.h"
#include "LegacyAnimGraphNodeParser.h"
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(SharedData, ImporterAllocator, 0)
    AZ_CLASS_ALLOCATOR_IMPL(ChunkProcessor, ImporterAllocator, 0)

    bool ForwardFullAttribute(MCore::File* file, MCore::Endian::EEndianType endianType)
    {
        // read the attribute type
        uint32 attributeType;
        if (file->Read(&attributeType, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&attributeType, endianType);

        // read the attribute size
        uint32 attributeSize;
        if (file->Read(&attributeSize, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&attributeSize, endianType);

        uint32 numCharacters;
        if (file->Read(&numCharacters, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        if (numCharacters && !file->Forward(numCharacters))
        {
            return false;
        }
        if (attributeSize && !file->Forward(attributeSize))
        {
            return false;
        }

        return true;
    }

    bool ForwardAttributeSettings(MCore::File* file, MCore::Endian::EEndianType endianType)
    {
        // read the version of the attribute settings format
        uint8 version;
        if (file->Read(&version, sizeof(uint8)) != sizeof(uint8))
        {
            return false;
        }

        if (version == 2)
        {
            // read the flags (new in version 2)
            if (!file->Forward(sizeof(uint16)))
            {
                return false;
            }
        }
        if (version == 1 || version == 2)
        {
            // read the internal name
            uint32 numChars;
            if (file->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
            if (numChars && !file->Forward(numChars))
            {
                return false;
            }
            // read name
            if (file->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
            if (numChars && !file->Forward(numChars)) // name
            {
                return false;
            }
            // read the description
            if (file->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
            if (numChars && !file->Forward(numChars))
            {
                return false;
            }
            // interface type
            if (!file->Forward(sizeof(uint32)))
            {
                return false;
            }
            // read the number of combobox values
            uint32 numComboValues;
            if (file->Read(&numComboValues, sizeof(uint32)) != sizeof(uint32))
            {
                return false;
            }
            MCore::Endian::ConvertUnsignedInt32(&numComboValues, endianType);
            // read the combo strings
            for (uint32 i = 0; i < numComboValues; ++i)
            {
                if (file->Read(&numChars, sizeof(uint32)) != sizeof(uint32))
                {
                    return false;
                }
                MCore::Endian::ConvertUnsignedInt32(&numChars, endianType);
                if (numChars && !file->Forward(numChars))
                {
                    return false;
                }
            }
            // full attributes means that it saves the type, size, version and its data
            // the default value
            if (!ForwardFullAttribute(file, endianType))
            {
                return false;
            }
            // the minimum value
            if (!ForwardFullAttribute(file, endianType))
            {
                return false;
            }
            // the maximum value
            if (!ForwardFullAttribute(file, endianType))
            {
                return false;
            }
        }
        else
        {
            AZ_Assert(false, "Unknown attribute version");
            return false;
        }

        return true;
    }

    bool ForwardAttributes(MCore::File* file, MCore::Endian::EEndianType endianType, uint32 numAttributes, bool hasAttributeSettings = false)
    {
        for (uint32 i = 0; i < numAttributes; ++i)
        {
            if (hasAttributeSettings && !ForwardAttributeSettings(file, endianType))
            {
                return false;
            }
            if (!ForwardFullAttribute(file, endianType))
            {
                return false;
            }
        }
        return true;
    }

    bool ForwardAttributeSet(MCore::File* file, MCore::Endian::EEndianType endianType)
    {
        // version
        if (!file->Forward(sizeof(uint8)))
        {
            return false;
        }
        uint32 numAttributes;
        if (file->Read(&numAttributes, sizeof(uint32)) != sizeof(uint32))
        {
            return false;
        }
        MCore::Endian::ConvertUnsignedInt32(&numAttributes, endianType);
        return ForwardAttributes(file, endianType, numAttributes, true);
    }


    // constructor
    SharedHelperData::SharedHelperData()
    {
        mFileHighVersion    = 1;
        mFileLowVersion     = 0;
        mIsUnicodeFile      = true;

        // allocate the string buffer used for reading in variable sized strings
        mStringStorageSize  = 256;
        mStringStorage      = (char*)MCore::Allocate(mStringStorageSize, EMFX_MEMCATEGORY_IMPORTER);
        mBlendNodes.SetMemoryCategory(EMFX_MEMCATEGORY_IMPORTER);
        mBlendNodes.Reserve(1024);
        //mConvertString.Reserve( 256 );
    }


    // destructor
    SharedHelperData::~SharedHelperData()
    {
        Reset();
    }


    // creation
    SharedHelperData* SharedHelperData::Create()
    {
        return new SharedHelperData();
    }


    // reset the shared data
    void SharedHelperData::Reset()
    {
        // free the string buffer
        if (mStringStorage)
        {
            MCore::Free(mStringStorage);
        }

        mStringStorage = nullptr;
        mStringStorageSize = 0;
        //mConvertString.Clear();

        // get rid of the blend nodes array
        mBlendNodes.Clear();

        m_entryNodeIndexToStateMachineIdLookupTable.clear();
    }


    // check if the strings in the file are encoded using unicode or multi-byte
    bool SharedHelperData::GetIsUnicodeFile(const char* dateString, MCore::Array<SharedData*>* sharedData)
    {
        // find the helper data
        SharedData* data = Importer::FindSharedData(sharedData, SharedHelperData::TYPE_ID);
        SharedHelperData* helperData = static_cast<SharedHelperData*>(data);

        AZStd::vector<AZStd::string> dateParts;
        AzFramework::StringFunc::Tokenize(dateString, dateParts, MCore::CharacterConstants::space, false /* keep empty strings */, true /* keep space strings */);

        // decode the month
        int32 month = 0;
        const AZStd::string& monthString = dateParts[0];
        const char* monthStrings[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
        for (int32 i = 0; i < 12; ++i)
        {
            if (monthString == monthStrings[i])
            {
                month = i + 1;
                break;
            }
        }

        //int32 day = dateParts[1].ToInt();
        int32 year;
        if (!AzFramework::StringFunc::LooksLikeInt(dateParts[2].c_str(), &year))
        {
            return false;
        }

        // set if the file contains unicode strings or not based on the compilcation date
        if (year < 2012 || (year == 2012 && month < 11))
        {
            helperData->mIsUnicodeFile = false;
        }

        //LogInfo( "String: '%s', Decoded: %i.%i.%i - isUnicode=%i", dateString, day, month, year, helperData->mIsUnicodeFile );

        return helperData->mIsUnicodeFile;
    }


    const char* SharedHelperData::ReadString(MCore::Stream* file, MCore::Array<SharedData*>* sharedData, MCore::Endian::EEndianType endianType)
    {
        MCORE_ASSERT(file);
        MCORE_ASSERT(sharedData);

        // find the helper data
        SharedData* data                = Importer::FindSharedData(sharedData, SharedHelperData::TYPE_ID);
        SharedHelperData* helperData    = static_cast<SharedHelperData*>(data);

        // get the size of the string (number of characters)
        uint32 numCharacters;
        file->Read(&numCharacters, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numCharacters, endianType);

        // if we need to enlarge the buffer
        if (helperData->mStringStorageSize < numCharacters + 1)
        {
            helperData->mStringStorageSize = numCharacters + 1;
            helperData->mStringStorage = (char*)MCore::Realloc(helperData->mStringStorage, helperData->mStringStorageSize, EMFX_MEMCATEGORY_IMPORTER);
        }

        // receive the actual string
        file->Read(helperData->mStringStorage, numCharacters * sizeof(uint8));
        helperData->mStringStorage[numCharacters] = '\0';

        //if (helperData->mIsUnicodeFile)
        //helperData->mConvertString = helperData->mStringStorage;
        //else
        //          helperData->mConvertString = helperData->mStringStorage;

        //result = helperData->mStringStorage;
        return helperData->mStringStorage;
    }


    // get the array of anim graph nodes
    MCore::Array<AnimGraphNode*>& SharedHelperData::GetBlendNodes(MCore::Array<SharedData*>* sharedData)
    {
        // find the helper data
        SharedData* data                = Importer::FindSharedData(sharedData, SharedHelperData::TYPE_ID);
        SharedHelperData* helperData    = static_cast<SharedHelperData*>(data);
        return helperData->mBlendNodes;
    }

    // Get the table of entry state indices to state machines IDs
    AZStd::map<AZ::u64, uint32>& SharedHelperData::GetEntryStateToStateMachineTable(MCore::Array<SharedData*>* sharedData)
    {
        // Find the helper data
        SharedData* data = Importer::FindSharedData(sharedData, SharedHelperData::TYPE_ID);
        SharedHelperData* helperData = static_cast<SharedHelperData*>(data);
        return helperData->m_entryNodeIndexToStateMachineIdLookupTable;
    }

    //-----------------------------------------------------------------------------

    // constructor
    ChunkProcessor::ChunkProcessor(uint32 chunkID, uint32 version)
        : BaseObject()
    {
        mChunkID        = chunkID;
        mVersion        = version;
        mLoggingActive  = false;
    }


    // destructor
    ChunkProcessor::~ChunkProcessor()
    {
    }


    uint32 ChunkProcessor::GetChunkID() const
    {
        return mChunkID;
    }


    uint32 ChunkProcessor::GetVersion() const
    {
        return mVersion;
    }


    void ChunkProcessor::SetLogging(bool loggingActive)
    {
        mLoggingActive = loggingActive;
    }


    bool ChunkProcessor::GetLogging() const
    {
        return mLoggingActive;
    }


    //=================================================================================================


    // a chunk that contains all nodes in one chunk
    bool ChunkProcessorActorNodes::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;
        Importer::ActorSettings* actorSettings = importParams.mActorSettings;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        FileFormat::Actor_Nodes nodesHeader;
        file->Read(&nodesHeader, sizeof(FileFormat::Actor_Nodes));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodesHeader.mNumNodes, endianType);
        MCore::Endian::ConvertUnsignedInt32(&nodesHeader.mNumRootNodes, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMin.mX, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMin.mY, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMin.mZ, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMax.mX, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMax.mY, endianType);
        MCore::Endian::ConvertFloat(&nodesHeader.mStaticBoxMax.mZ, endianType);

        // convert endian and coord system of the static box
        AZ::Vector3 boxMin(nodesHeader.mStaticBoxMin.mX, nodesHeader.mStaticBoxMin.mY, nodesHeader.mStaticBoxMin.mZ);
        AZ::Vector3 boxMax(nodesHeader.mStaticBoxMax.mX, nodesHeader.mStaticBoxMax.mY, nodesHeader.mStaticBoxMax.mZ);

        // build the box and set it
        MCore::AABB staticBox;
        staticBox.SetMin(boxMin);
        staticBox.SetMax(boxMax);
        actor->SetStaticAABB(staticBox);

        // pre-allocate space for the nodes
        actor->SetNumNodes(nodesHeader.mNumNodes);

        // pre-allocate space for the root nodes
        skeleton->ReserveRootNodes(nodesHeader.mNumRootNodes);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Nodes: %d (%d root nodes)", nodesHeader.mNumNodes, nodesHeader.mNumRootNodes);
        }

        // add the transform
        actor->ResizeTransformData();

        // read all nodes
        for (uint32 n = 0; n < nodesHeader.mNumNodes; ++n)
        {
            // read the node header
            FileFormat::Actor_Node nodeChunk;
            file->Read(&nodeChunk, sizeof(FileFormat::Actor_Node));

            // read the node name
            const char* nodeName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&nodeChunk.mParentIndex, endianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeChunk.mSkeletalLODs, endianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeChunk.mNumChilds, endianType);
            MCore::Endian::ConvertFloat(&nodeChunk.mOBB[0], endianType, 16);

            // show the name of the node, the parent and the number of children
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Node name = '%s'", nodeName);
                MCore::LogDetailedInfo("     - Parent = '%s'", (nodeChunk.mParentIndex != MCORE_INVALIDINDEX32) ? skeleton->GetNode(nodeChunk.mParentIndex)->GetName() : "");
                MCore::LogDetailedInfo("     - NumChild Nodes = %d", nodeChunk.mNumChilds);
            }

            // create the new node
            Node* node = Node::Create(nodeName, skeleton);

            // set the node index
            const uint32 nodeIndex = n;
            node->SetNodeIndex(nodeIndex);

            // pre-allocate space for the number of child nodes
            node->PreAllocNumChildNodes(nodeChunk.mNumChilds);

            // add it to the actor
            skeleton->SetNode(n, node);

            // create Core objects from the data
            AZ::PackedVector3f pos(nodeChunk.mLocalPos.mX, nodeChunk.mLocalPos.mY, nodeChunk.mLocalPos.mZ);
            AZ::PackedVector3f scale(nodeChunk.mLocalScale.mX, nodeChunk.mLocalScale.mY, nodeChunk.mLocalScale.mZ);
            MCore::Quaternion rot(nodeChunk.mLocalQuat.mX, nodeChunk.mLocalQuat.mY, nodeChunk.mLocalQuat.mZ, nodeChunk.mLocalQuat.mW);

            // convert endian and coordinate system
            ConvertVector3(&pos, endianType);
            ConvertScale(&scale, endianType);
            ConvertQuaternion(&rot, endianType);

            // make sure the input data is normalized
            // TODO: this isn't really needed as we already normalized?
            //rot.FastNormalize();
            //scaleRot.FastNormalize();

            // set the local transform
            Transform bindTransform;
            bindTransform.mPosition     = AZ::Vector3(pos);
            bindTransform.mRotation     = rot;

            EMFX_SCALECODE
            (
                bindTransform.mScale            = AZ::Vector3(scale);
            )

            actor->GetBindPose()->SetLocalTransform(nodeIndex, bindTransform);

            // set the skeletal LOD levels
            if (actorSettings->mLoadSkeletalLODs)
            {
                node->SetSkeletalLODLevelBits(nodeChunk.mSkeletalLODs);
            }

            // set if this node has to be taken into the bounding volume calculation
            const bool includeInBoundsCalc = (nodeChunk.mNodeFlags & (1 << 0)); // first bit
            node->SetIncludeInBoundsCalc(includeInBoundsCalc);

            // set the parent, and add this node as child inside the parent
            if (nodeChunk.mParentIndex != MCORE_INVALIDINDEX32) // if this node has a parent and the parent node is valid
            {
                if (nodeChunk.mParentIndex < n)
                {
                    node->SetParentIndex(nodeChunk.mParentIndex);
                    Node* parentNode = skeleton->GetNode(nodeChunk.mParentIndex);
                    parentNode->AddChild(nodeIndex);
                }
                else
                {
                    MCore::LogError("Cannot assign parent node index (%d) for node '%s' as the parent node is not yet loaded. Making '%s' a root node.", nodeChunk.mParentIndex, node->GetName(), node->GetName());
                    skeleton->AddRootNode(nodeIndex);
                }
            }
            else // if this node has no parent, so when it is a root node
            {
                skeleton->AddRootNode(nodeIndex);
            }

            // OBB
            MCore::Matrix obbMatrix;
            MCore::MemCopy(&obbMatrix, nodeChunk.mOBB, sizeof(MCore::Matrix));

            AZ::Vector3 obbCenter(obbMatrix.m44[3][0], obbMatrix.m44[3][1], obbMatrix.m44[3][2]);
            AZ::Vector3 obbExtents(obbMatrix.m44[0][3], obbMatrix.m44[1][3], obbMatrix.m44[2][3]);

        #ifndef MCORE_MATRIX_ROWMAJOR
            obbMatrix.Transpose();
        #endif

            obbMatrix.SetRow(3, AZ::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
            obbMatrix.SetColumn(3, AZ::Vector3::CreateZero());

            // initialize the OBB
            MCore::OBB obb;
            obb.SetCenter(obbCenter);
            obb.SetExtents(obbExtents);
            obb.SetTransformation(obbMatrix);
            actor->SetNodeOBB(nodeIndex, obb);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("      - Position:      x=%f, y=%f, z=%f",
                    static_cast<float>(pos.GetX()),
                    static_cast<float>(pos.GetY()),
                    static_cast<float>(pos.GetZ()));
                MCore::LogDetailedInfo("      - Rotation:      x=%f, y=%f, z=%f, w=%f", rot.x, rot.y, rot.z, rot.w);
                MCore::LogDetailedInfo("      - Scale:         x=%f, y=%f, z=%f",
                    static_cast<float>(scale.GetX()),
                    static_cast<float>(scale.GetY()),
                    static_cast<float>(scale.GetZ()));
                MCore::LogDetailedInfo("      - IncludeInBoundsCalc: %d", includeInBoundsCalc);
            }
        } // for all nodes

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorMesh::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;
        Importer::ActorSettings* actorSettings = importParams.mActorSettings;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the mesh header
        FileFormat::Actor_Mesh meshHeader;
        file->Read(&meshHeader, sizeof(FileFormat::Actor_Mesh));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&meshHeader.mNodeIndex,    endianType);
        MCore::Endian::ConvertUnsignedInt32(&meshHeader.mNumOrgVerts,  endianType);
        MCore::Endian::ConvertUnsignedInt32(&meshHeader.mNumSubMeshes, endianType);
        MCore::Endian::ConvertUnsignedInt32(&meshHeader.mNumPolygons,  endianType);
        MCore::Endian::ConvertUnsignedInt32(&meshHeader.mNumLayers,    endianType);
        MCore::Endian::ConvertUnsignedInt32(&meshHeader.mTotalIndices, endianType);
        MCore::Endian::ConvertUnsignedInt32(&meshHeader.mTotalVerts,   endianType);
        MCore::Endian::ConvertUnsignedInt32(&meshHeader.mLOD,          endianType);

        bool importMesh = true;
        if (meshHeader.mLOD > 0 && importParams.mActorSettings->mLoadGeometryLODs == false)
        {
            importMesh = false;
        }

        // if we are dealing with a collision mesh, but don't want to load it, seek past this chunk
        if ((meshHeader.mIsCollisionMesh && actorSettings->mLoadCollisionMeshes == false) || (meshHeader.mIsCollisionMesh == false && actorSettings->mLoadMeshes == false) || importMesh == false)
        {
            // first read back to the position where we can read the chunk header
            size_t pos = file->GetPos();
            file->Seek(pos - sizeof(FileFormat::Actor_Mesh) - sizeof(FileFormat::FileChunk));

            // read the chunk header
            FileFormat::FileChunk chunk;
            file->Read(&chunk, sizeof(FileFormat::FileChunk));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&chunk.mChunkID, endianType);
            MCore::Endian::ConvertUnsignedInt32(&chunk.mVersion, endianType);
            MCore::Endian::ConvertUnsignedInt32(&chunk.mSizeInBytes, endianType);

            // make sure we're dealing with a mesh chunk to verify that this worked
            MCORE_ASSERT(chunk.mChunkID == FileFormat::ACTOR_CHUNK_MESH);

            // seek past the end of the mesh data chunk
            file->Forward(chunk.mSizeInBytes);

            // return success
            return true;
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Mesh");
            MCore::LogDetailedInfo("    + NodeIndex         = %d (%s)", meshHeader.mNodeIndex, skeleton->GetNode(meshHeader.mNodeIndex)->GetName());
            MCore::LogDetailedInfo("    + LOD               = %d", meshHeader.mLOD);
            MCore::LogDetailedInfo("    + NumOrgVerts       = %d", meshHeader.mNumOrgVerts);
            MCore::LogDetailedInfo("    + NumVerts          = %d", meshHeader.mTotalVerts);
            MCore::LogDetailedInfo("    + NumPolygons       = %d", meshHeader.mNumPolygons);
            MCore::LogDetailedInfo("    + NumIndices        = %d (%d polygons)", meshHeader.mTotalIndices, meshHeader.mNumPolygons);
            MCore::LogDetailedInfo("    + NumSubMeshes      = %d", meshHeader.mNumSubMeshes);
            MCore::LogDetailedInfo("    + Num attrib layers = %d", meshHeader.mNumLayers);
            MCore::LogDetailedInfo("    + IsCollisionMesh   = %s", meshHeader.mIsCollisionMesh ? "Yes" : "No");
            MCore::LogDetailedInfo("    + IsTriangleMesh    = %d", meshHeader.mIsTriangleMesh ? "Yes" : "No");
        }

        // create the smartpointer to the mesh object
        // we use a smartpointer here, because of the reference counting system needed for shared meshes.
        Mesh* mesh = Mesh::Create(meshHeader.mTotalVerts, meshHeader.mTotalIndices, meshHeader.mNumPolygons, meshHeader.mNumOrgVerts, meshHeader.mIsCollisionMesh ? true : false);

        // link the mesh to the correct node
        Node* meshNode = skeleton->GetNode(meshHeader.mNodeIndex);
        actor->SetMesh(meshHeader.mLOD, meshNode->GetNodeIndex(), mesh);

        // read the layers
        const uint32 numLayers = meshHeader.mNumLayers;
        mesh->ReserveVertexAttributeLayerSpace(numLayers);
        for (uint32 layerNr = 0; layerNr < numLayers; ++layerNr)
        {
            // read the layer header
            FileFormat::Actor_VertexAttributeLayer fileLayer;
            file->Read(&fileLayer, sizeof(FileFormat::Actor_VertexAttributeLayer));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&fileLayer.mLayerTypeID, endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileLayer.mAttribSizeInBytes, endianType);

            // read the layer name
            const char* layerName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("   - Layer #%d:", layerNr);
                MCore::LogDetailedInfo("     + Type ID:         %d (%s)", fileLayer.mLayerTypeID, Importer::ActorVertexAttributeLayerTypeToString(fileLayer.mLayerTypeID));
                MCore::LogDetailedInfo("     + Attribute size:  %d bytes", fileLayer.mAttribSizeInBytes);
                MCore::LogDetailedInfo("     + Keep originals:  %s", fileLayer.mEnableDeformations ? "Yes" : "No");
                MCore::LogDetailedInfo("     + Is scale factor: %s", fileLayer.mIsScale ? "Yes" : "No");
                MCore::LogDetailedInfo("     + Name:            %s", (layerName) ? layerName : "");
            }

            // check if we need to skip this layer or not
            const uint32 numAttribs = meshHeader.mTotalVerts;
            if (actorSettings->mLayerIDsToIgnore.Contains(fileLayer.mLayerTypeID))
            {
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     + Skipping layer data as this was specified in the actor import settings...");
                }
                file->Forward(fileLayer.mAttribSizeInBytes * numAttribs);
                continue; // process the next layer
            }

            // create the layer
            VertexAttributeLayerAbstractData* layer = VertexAttributeLayerAbstractData::Create(numAttribs, fileLayer.mLayerTypeID, fileLayer.mAttribSizeInBytes, (fileLayer.mEnableDeformations) != 0);
            layer->SetName(layerName);
            mesh->AddVertexAttributeLayer(layer);

            // read the data from disk into the layer
            MCORE_ASSERT(layer->CalcTotalDataSizeInBytes(false) == (fileLayer.mAttribSizeInBytes * numAttribs));
            file->Read(layer->GetOriginalData(), layer->CalcTotalDataSizeInBytes(false));

            // convert endian and coordinate systems of all data
            if (actorSettings->mLayerConvertFunction(layer, endianType) == false)
            {
                MCore::LogWarning("   + Don't know how to endian and/or coordinate system convert layer with type %d (%s)", fileLayer.mLayerTypeID, Importer::ActorVertexAttributeLayerTypeToString(fileLayer.mLayerTypeID));
            }

            // copy the original data over the current data values
            layer->ResetToOriginalData();
        }

        // submesh offsets
        uint32 indexOffset  = 0;
        uint32 vertexOffset = 0;
        uint32 polyOffset   = 0;

        // get a pointer to the index buffer
        uint32* indices = mesh->GetIndices();
        uint8* polyVertexCounts = mesh->GetPolygonVertexCounts();

        // read the submeshes
        const uint32 numSubMeshes = meshHeader.mNumSubMeshes;
        mesh->SetNumSubMeshes(numSubMeshes);
        for (uint32 s = 0; s < numSubMeshes; ++s)
        {
            // read the layer header
            FileFormat::Actor_SubMesh fileSubMesh;
            file->Read(&fileSubMesh, sizeof(FileFormat::Actor_SubMesh));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&fileSubMesh.mMaterialIndex,   endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMesh.mNumVerts,        endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMesh.mNumIndices,      endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMesh.mNumPolygons,     endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMesh.mNumBones,        endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("   - SubMesh #%d:", s);
                MCore::LogDetailedInfo("     + Material:       %d (%s)", fileSubMesh.mMaterialIndex, actor->GetMaterial(0, fileSubMesh.mMaterialIndex)->GetName());
                MCore::LogDetailedInfo("     + Num vertices:   %d", fileSubMesh.mNumVerts);
                MCore::LogDetailedInfo("     + Num indices:    %d (%d polygons)", fileSubMesh.mNumIndices, fileSubMesh.mNumPolygons);
                MCore::LogDetailedInfo("     + Num bones:      %d", fileSubMesh.mNumBones);
            }

            // create and add the submesh inside GetEMotionFX()
            const uint32 numPolys = fileSubMesh.mNumPolygons;
            SubMesh* subMesh = SubMesh::Create(mesh, vertexOffset, indexOffset, polyOffset, fileSubMesh.mNumVerts, fileSubMesh.mNumIndices, numPolys, fileSubMesh.mMaterialIndex, fileSubMesh.mNumBones);
            mesh->SetSubMesh(s, subMesh);

            // read the indices
            uint32* fileIndices = (uint32*)MCore::Allocate(fileSubMesh.mNumIndices * sizeof(uint32), EMFX_MEMCATEGORY_IMPORTER);
            file->Read(fileIndices, fileSubMesh.mNumIndices * sizeof(uint32));
            MCore::Endian::ConvertUnsignedInt32(fileIndices, endianType, fileSubMesh.mNumIndices);
            uint32 i;
            for (i = 0; i < fileSubMesh.mNumIndices; ++i)
            {
                indices[indexOffset + i] = fileIndices[i] + vertexOffset;
            }
            MCore::Free(fileIndices);


            // read the polygon vertex counts
            uint8* filePolyVertexCounts = (uint8*)MCore::Allocate(numPolys * sizeof(uint8), EMFX_MEMCATEGORY_IMPORTER);
            file->Read(filePolyVertexCounts, numPolys * sizeof(uint8));
            for (i = 0; i < numPolys; ++i)
            {
                polyVertexCounts[polyOffset + i] = filePolyVertexCounts[i];
            }
            MCore::Free(filePolyVertexCounts);


            // read the bone/node numbers
            for (i = 0; i < fileSubMesh.mNumBones; ++i)
            {
                uint32 value;
                file->Read(&value, sizeof(uint32));
                MCore::Endian::ConvertUnsignedInt32(&value, endianType);
                //LogDebug("bone #%d = %s", i, actor->GetNode(value)->GetName());
                subMesh->SetBone(i, value);
            }

            // increase the offsets
            indexOffset  += fileSubMesh.mNumIndices;
            vertexOffset += fileSubMesh.mNumVerts;
            polyOffset   += numPolys;
        }

        // calculate the tangents when we want tangents, but they aren't there
        if (mesh->FindOriginalVertexData(Mesh::ATTRIB_TANGENTS) == nullptr && actorSettings->mAutoGenTangents)
        {
            if (GetLogging())
            {
                MCore::LogDetailedInfo("    + Generating tangents...");
            }
            mesh->CalcTangents();
        }

        return true;
    }

    //=================================================================================================

    // the skinning information v4
    bool ChunkProcessorActorSkinningInfo::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Skinning Information (chunk v4)");
        }

        // read the node number this info belongs to
        FileFormat::Actor_SkinningInfo fileInfo;
        file->Read(&fileInfo, sizeof(FileFormat::Actor_SkinningInfo));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInfo.mNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInfo.mLOD, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInfo.mNumTotalInfluences, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInfo.mNumLocalBones, endianType);

        const uint32 lodLevel = fileInfo.mLOD;

        Mesh* mesh = nullptr;

        bool skip = false;
        if (lodLevel > 0 && importParams.mActorSettings->mLoadGeometryLODs == false)
        {
            skip = true;
        }

        FileFormat::Actor_SkinInfluence* fileInfluences = nullptr;
        if (skip == false)
        {
            mesh = actor->GetMesh(lodLevel, fileInfo.mNodeIndex);
        }

        if (mesh == nullptr)
        {
            skip = true;
        }

        if (skip)
        {
            // first read back to the position where we can read the chunk header
            size_t pos = file->GetPos();
            file->Seek(pos - sizeof(FileFormat::Actor_SkinningInfo) - sizeof(FileFormat::FileChunk));

            // read the chunk header
            FileFormat::FileChunk chunk;
            file->Read(&chunk, sizeof(FileFormat::FileChunk));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&chunk.mChunkID, endianType);
            MCore::Endian::ConvertUnsignedInt32(&chunk.mVersion, endianType);
            MCore::Endian::ConvertUnsignedInt32(&chunk.mSizeInBytes, endianType);

            // make sure we're dealing with a mesh chunk to verify that this worked
            MCORE_ASSERT(chunk.mChunkID == FileFormat::ACTOR_CHUNK_SKINNINGINFO);

            // seek past the end of the mesh data chunk
            file->Forward(chunk.mSizeInBytes);

            // return success
            return true;
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Node number      = %d", fileInfo.mNodeIndex);
            MCore::LogDetailedInfo("   + LOD              = %d", fileInfo.mLOD);
            MCore::LogDetailedInfo("   + Node name        = %s", skeleton->GetNode(fileInfo.mNodeIndex)->GetName());
            MCore::LogDetailedInfo("   + Num local bones  = %d", fileInfo.mNumLocalBones);
            MCore::LogDetailedInfo("   + Total influences = %d", fileInfo.mNumTotalInfluences);
            MCore::LogDetailedInfo("   + For Col. Mesh    = %d", fileInfo.mIsForCollisionMesh);
        }

        // add the skinning info to the mesh
        SkinningInfoVertexAttributeLayer* skinningLayer = SkinningInfoVertexAttributeLayer::Create(mesh->GetNumOrgVertices(), false);
        mesh->AddSharedVertexAttributeLayer(skinningLayer);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Num org vertices = %d", mesh->GetNumOrgVertices());
        }

        // get the Array2D from the skinning layer
        MCore::Array2D<SkinInfluence>& skinningArray = skinningLayer->GetArray2D();
        MCore::Array<SkinInfluence>& skinningDataArray = skinningArray.GetData();

        // make the skinning data array as big as we need
        skinningDataArray.Resize(fileInfo.mNumTotalInfluences);

        // allocate space for all influences
        fileInfluences = (FileFormat::Actor_SkinInfluence*)MCore::Allocate(fileInfo.mNumTotalInfluences * sizeof(FileFormat::Actor_SkinInfluence), EMFX_MEMCATEGORY_IMPORTER);

        // now read in all influences
        file->Read(fileInfluences, fileInfo.mNumTotalInfluences * sizeof(FileFormat::Actor_SkinInfluence));

        // init all influences
        uint32 w;
        const uint32 totalInfluences = fileInfo.mNumTotalInfluences;
        for (w = 0; w < totalInfluences; ++w)
        {
            MCore::Endian::ConvertUnsignedInt16(&fileInfluences[w].mNodeNr, endianType);
            MCore::Endian::ConvertFloat(&fileInfluences[w].mWeight, endianType);
            skinningDataArray[w] = SkinInfluence(fileInfluences[w].mNodeNr, fileInfluences[w].mWeight);
        }

        // release temp influence data from memory
        MCore::Free(fileInfluences);

        // now read in the table entries
        MCore::Array< MCore::Array2D<SkinInfluence>::TableEntry >& tableEntryArray = skinningArray.GetIndexTable();
        tableEntryArray.Resize(mesh->GetNumOrgVertices());
        MCORE_ASSERT(sizeof(FileFormat::Actor_SkinningInfoTableEntry) == sizeof(MCore::Array2D<SkinInfluence>::TableEntry));
        file->Read(tableEntryArray.GetPtr(), sizeof(FileFormat::Actor_SkinningInfoTableEntry) * mesh->GetNumOrgVertices());

        // convert all endians
        // TODO: check if endian conversion is needed at all?
        const uint32 totalEntries = mesh->GetNumOrgVertices();
        for (w = 0; w < totalEntries; ++w)
        {
            MCore::Endian::ConvertUnsignedInt32(&tableEntryArray[w].mStartIndex, endianType);
            MCore::Endian::ConvertUnsignedInt32(&tableEntryArray[w].mNumElements, endianType);
            //MCORE_ASSERT( tableEntryArray[w].mStartIndex < fileInfo.mNumTotalInfluences );
            //MCORE_ASSERT( tableEntryArray[w].mNumElements < 5 );
        }

        // get the stack
        Node* node = skeleton->GetNode(fileInfo.mNodeIndex);
        /*  if (fileInfo.mIsForCollisionMesh)
            {
                MeshDeformerStack* stack = actor->GetCollisionMeshDeformerStack(lodLevel, node->GetNodeIndex());

                // create the stack if it doesn't yet exist
                if (stack == nullptr)
                {
                    stack = MeshDeformerStack::Create( actor->GetCollisionMesh(lodLevel, node->GetNodeIndex()) );
                    actor->SetCollisionMeshDeformerStack(lodLevel, node->GetNodeIndex(), stack);
                }

                // add the skinning deformer to the stack
                if (importParams.mActorSettings->mDualQuatSkinning)
                {
                    DualQuatSkinDeformer* skinDeformer = DualQuatSkinDeformer::Create( actor->GetCollisionMesh(lodLevel, node->GetNodeIndex()) );
                    stack->AddDeformer( skinDeformer );
                    skinDeformer->ReserveLocalBones( fileInfo.mNumLocalBones ); // pre-alloc data to prevent reallocs
                }
                else
                {
                    SoftSkinDeformer* skinDeformer = GetSoftSkinManager().CreateDeformer( actor->GetCollisionMesh(lodLevel, node->GetNodeIndex()) );
                    stack->AddDeformer( skinDeformer );
                    skinDeformer->ReserveLocalBones( fileInfo.mNumLocalBones ); // pre-alloc data to prevent reallocs
                }
            }
            else*/
        {
            MeshDeformerStack* stack = actor->GetMeshDeformerStack(lodLevel, node->GetNodeIndex());

            // create the stack if it doesn't yet exist
            if (stack == nullptr)
            {
                stack = MeshDeformerStack::Create(actor->GetMesh(lodLevel, node->GetNodeIndex()));
                actor->SetMeshDeformerStack(lodLevel, node->GetNodeIndex(), stack);
            }

            // add the skinning deformer to the stack
            if (importParams.mActorSettings->mDualQuatSkinning)
            {
                DualQuatSkinDeformer* skinDeformer = DualQuatSkinDeformer::Create(actor->GetMesh(lodLevel, node->GetNodeIndex()));
                stack->AddDeformer(skinDeformer);
                skinDeformer->ReserveLocalBones(fileInfo.mNumLocalBones); // pre-alloc data to prevent reallocs
            }
            else
            {
                SoftSkinDeformer* skinDeformer = GetSoftSkinManager().CreateDeformer(actor->GetMesh(lodLevel, node->GetNodeIndex()));
                stack->AddDeformer(skinDeformer);
                skinDeformer->ReserveLocalBones(fileInfo.mNumLocalBones); // pre-alloc data to prevent reallocs
            }
        }

        return true;
    }

    //=================================================================================================

    // read all submotions in one chunk
    bool ChunkProcessorMotionSubMotions::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;
        //Importer::SkeletalMotionSettings* skelMotionSettings = importParams.mSkeletalMotionSettings;

        MCORE_ASSERT(motion);
        MCORE_ASSERT(motion->GetType() == SkeletalMotion::TYPE_ID); // make sure we're dealing with a skeletal motion
        SkeletalMotion* skelMotion = (SkeletalMotion*)motion;

        // read the header
        FileFormat::Motion_SubMotions subMotionsHeader;
        file->Read(&subMotionsHeader, sizeof(FileFormat::Motion_SubMotions));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&subMotionsHeader.mNumSubMotions, endianType);

        // pre-allocate space for the submotions
        if (!skelMotion->SetNumSubMotions(subMotionsHeader.mNumSubMotions))
        {
            if (GetLogging())
            {
                MCore::LogError("Could not create %d sub motions", subMotionsHeader.mNumSubMotions);
                return false;
            }
        }

        // for all submotions
        for (uint32 s = 0; s < subMotionsHeader.mNumSubMotions; ++s)
        {
            FileFormat::Motion_SkeletalSubMotion fileSubMotion;
            file->Read(&fileSubMotion, sizeof(FileFormat::Motion_SkeletalSubMotion));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&fileSubMotion.mNumPosKeys, endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMotion.mNumRotKeys, endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileSubMotion.mNumScaleKeys, endianType);

            // read the motion part name
            const char* motionPartName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // convert into Core objects
            AZ::PackedVector3f posePos(fileSubMotion.mPosePos.mX, fileSubMotion.mPosePos.mY, fileSubMotion.mPosePos.mZ);
            AZ::PackedVector3f poseScale(fileSubMotion.mPoseScale.mX, fileSubMotion.mPoseScale.mY, fileSubMotion.mPoseScale.mZ);
            MCore::Compressed16BitQuaternion poseRot(fileSubMotion.mPoseRot.mX, fileSubMotion.mPoseRot.mY, fileSubMotion.mPoseRot.mZ, fileSubMotion.mPoseRot.mW);

            AZ::PackedVector3f bindPosePos(fileSubMotion.mBindPosePos.mX, fileSubMotion.mBindPosePos.mY, fileSubMotion.mBindPosePos.mZ);
            AZ::PackedVector3f bindPoseScale(fileSubMotion.mBindPoseScale.mX, fileSubMotion.mBindPoseScale.mY, fileSubMotion.mBindPoseScale.mZ);
            MCore::Compressed16BitQuaternion bindPoseRot(fileSubMotion.mBindPoseRot.mX, fileSubMotion.mBindPoseRot.mY, fileSubMotion.mBindPoseRot.mZ, fileSubMotion.mBindPoseRot.mW);

            // convert endian and coordinate system
            ConvertVector3(&posePos, endianType);
            ConvertVector3(&bindPosePos, endianType);
            ConvertScale(&poseScale, endianType);
            ConvertScale(&bindPoseScale, endianType);
            Convert16BitQuaternion(&poseRot, endianType);
            Convert16BitQuaternion(&bindPoseRot, endianType);

            if (GetLogging())
            {
                MCore::Quaternion uncompressedPoseRot           = poseRot.ToQuaternion();
                MCore::Quaternion uncompressedBindPoseRot       = bindPoseRot.ToQuaternion();

                MCore::LogDetailedInfo("- Skeletal SubMotion = '%s'", motionPartName);
                MCore::LogDetailedInfo("    + Pose Position:         x=%f, y=%f, z=%f",
                    static_cast<float>(posePos.GetX()),
                    static_cast<float>(posePos.GetY()),
                    static_cast<float>(posePos.GetZ()));
                MCore::LogDetailedInfo("    + Pose Rotation:         x=%f, y=%f, z=%f, w=%f", uncompressedPoseRot.x, uncompressedPoseRot.y, uncompressedPoseRot.z, uncompressedPoseRot.w);
                MCore::LogDetailedInfo("    + Pose Scale:            x=%f, y=%f, z=%f",
                    static_cast<float>(poseScale.GetX()),
                    static_cast<float>(poseScale.GetY()),
                    static_cast<float>(poseScale.GetZ()));
                MCore::LogDetailedInfo("    + Bind Pose Position:    x=%f, y=%f, z=%f",
                    static_cast<float>(bindPosePos.GetX()),
                    static_cast<float>(bindPosePos.GetY()),
                    static_cast<float>(bindPosePos.GetZ()));
                MCore::LogDetailedInfo("    + Bind Pose Rotation:    x=%f, y=%f, z=%f, w=%f", uncompressedBindPoseRot.x, uncompressedBindPoseRot.y, uncompressedBindPoseRot.z, uncompressedBindPoseRot.w);
                MCore::LogDetailedInfo("    + Bind Pose Scale:       x=%f, y=%f, z=%f",
                    static_cast<float>(bindPoseScale.GetX()),
                    static_cast<float>(bindPoseScale.GetY()),
                    static_cast<float>(bindPoseScale.GetZ()));
                MCore::LogDetailedInfo("    + Num Pos Keys:          %d", fileSubMotion.mNumPosKeys);
                MCore::LogDetailedInfo("    + Num Rot Keys:          %d", fileSubMotion.mNumRotKeys);
                MCore::LogDetailedInfo("    + Num Scale Keys:        %d", fileSubMotion.mNumScaleKeys);
            }

            // create the part, and add it to the motion
            SkeletalSubMotion* subMotion = SkeletalSubMotion::Create(motionPartName);

            subMotion->SetPosePos(AZ::Vector3(posePos));
            subMotion->SetCompressedPoseRot(poseRot);
            subMotion->SetBindPosePos(AZ::Vector3(bindPosePos));
            subMotion->SetCompressedBindPoseRot(bindPoseRot);

            EMFX_SCALECODE
            (
                subMotion->SetPoseScale(AZ::Vector3(poseScale));
                subMotion->SetBindPoseScale(AZ::Vector3(bindPoseScale));
            )

            //if (skelMotionSettings->mLoadMotionLODErrors)
            //subMotion->SetMotionLODMaxError( fileSubMotion.mMaxError );

            skelMotion->SetSubMotion(s, subMotion);

            // now read the animation data
            // read the position keys first
            uint32 i;

            if (fileSubMotion.mNumPosKeys > 0)
            {
                subMotion->CreatePosTrack();
                KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>* posTrack = subMotion->GetPosTrack();
                if (!posTrack->SetNumKeys(fileSubMotion.mNumPosKeys))
                {
                    if (GetLogging())
                    {
                        MCore::LogError("Could not create %d position keys", fileSubMotion.mNumPosKeys);
                        subMotion->Destroy();
                        return false;
                    }
                }

                for (i = 0; i < fileSubMotion.mNumPosKeys; ++i)
                {
                    FileFormat::Motion_Vector3Key key;
                    file->Read(&key, sizeof(FileFormat::Motion_Vector3Key));

                    // convert the endian of the time
                    MCore::Endian::ConvertFloat(&key.mTime, endianType);

                    // convert endian and coordinate system of the position
                    AZ::PackedVector3f pos(key.mValue.mX, key.mValue.mY, key.mValue.mZ);
                    ConvertVector3(&pos, endianType);

                    //LogInfo("       + Key#%i: Time=%f, Pos(%f, %f, %f)", i, key.mTime, pos.x, pos.y, pos.z);

                    // add the key
                    posTrack->SetKey(i, key.mTime, pos);
                }
            }


            // now the rotation keys
            if (fileSubMotion.mNumRotKeys > 0)
            {
                subMotion->CreateRotTrack();
                KeyTrackLinear<MCore::Quaternion, MCore::Compressed16BitQuaternion>* rotTrack = subMotion->GetRotTrack();
                if (!rotTrack->SetNumKeys(fileSubMotion.mNumRotKeys))
                {
                    if (GetLogging())
                    {
                        MCore::LogError("Could not create %d rotation keys", fileSubMotion.mNumRotKeys);
                        subMotion->Destroy();
                        return false;
                    }
                }

                for (i = 0; i < fileSubMotion.mNumRotKeys; ++i)
                {
                    FileFormat::Motion_16BitQuaternionKey key;
                    file->Read(&key, sizeof(FileFormat::Motion_16BitQuaternionKey));

                    // convert the endian of the time
                    MCore::Endian::ConvertFloat(&key.mTime, endianType);

                    // convert endian and coordinate system
                    MCore::Compressed16BitQuaternion rot(key.mValue.mX, key.mValue.mY, key.mValue.mZ, key.mValue.mW);
                    Convert16BitQuaternion(&rot, endianType);

                    //LogInfo("       + Key#%i: Time=%f, Rot(%i, %i, %i, %i)", i, key.mTime, key.mValue.mX, key.mValue.mY, key.mValue.mZ, key.mValue.mW);

                    // add the key
                    rotTrack->SetStorageTypeKey(i, key.mTime, rot);
                }
            }


    #ifndef EMFX_SCALE_DISABLED
            // and the scale keys
            if (fileSubMotion.mNumScaleKeys > 0)
            {
                subMotion->CreateScaleTrack();
                KeyTrackLinear<AZ::PackedVector3f, AZ::PackedVector3f>* scaleTrack = subMotion->GetScaleTrack();
                if (!scaleTrack->SetNumKeys(fileSubMotion.mNumScaleKeys))
                {
                    if (GetLogging())
                    {
                        MCore::LogError("Could not create %d scale keys", fileSubMotion.mNumScaleKeys);
                        subMotion->Destroy();
                        return false;
                    }
                }

                for (i = 0; i < fileSubMotion.mNumScaleKeys; ++i)
                {
                    FileFormat::Motion_Vector3Key key;
                    file->Read(&key, sizeof(FileFormat::Motion_Vector3Key));

                    // convert the endian of the time
                    MCore::Endian::ConvertFloat(&key.mTime, endianType);

                    // convert endian and coordinate system
                    AZ::PackedVector3f scale(key.mValue.mX, key.mValue.mY, key.mValue.mZ);
                    ConvertScale(&scale, endianType);

                    //LogInfo("       + Key#%i: Time=%f, Scale(%f, %f, %f)", i, key.mTime, scale.x, scale.y, scale.z);

                    // add the key
                    scaleTrack->SetKey(i, key.mTime, AZ::PackedVector3f(scale));
                }
            }
    #else   // no scaling
            // and the scale keys
            if (fileSubMotion.mNumScaleKeys > 0)
            {
                for (i = 0; i < fileSubMotion.mNumScaleKeys; ++i)
                {
                    FileFormat::Motion_Vector3Key key;
                    file->Read(&key, sizeof(FileFormat::Motion_Vector3Key));
                }
            }
    #endif

            // init the keytracks
            if (subMotion->GetPosTrack())
            {
                //          subMotion->GetPosTrack()->SetInterpolator( new LinearInterpolator<Vector3, Vector3>() );
                subMotion->GetPosTrack()->Init();
            }

            if (subMotion->GetRotTrack())
            {
                //          subMotion->GetRotTrack()->SetInterpolator( new LinearQuaternionInterpolator<Compressed16BitQuaternion>() );
                subMotion->GetRotTrack()->Init();
            }

    #ifndef EMFX_SCALE_DISABLED
            if (subMotion->GetScaleTrack())
            {
                //          subMotion->GetScaleTrack()->SetInterpolator( new LinearInterpolator<Vector3, Vector3>() );
                subMotion->GetScaleTrack()->Init();
            }
    #endif
        } // for all submotions

        // calculate the maximum motion LOD error
        /////skelMotion->UpdateMotionLODMaxError(); // TODO: could do this inside the loop already

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionInfo::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;

        MCORE_ASSERT(motion);

        // read the chunk
        FileFormat::Motion_Info fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Motion_Info));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionMask, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Unit Type                     = %d", fileInformation.mUnitType);
        }

        motion->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        motion->SetFileUnitType(motion->GetUnitType());

        // Try to remain backward compatible by still capturing height when this was enabled in the old mask system.
        if (fileInformation.mMotionExtractionMask & (1 << 2))   // The 1<<2 was the mask used for position Z in the old motion extraction mask settings
        {
            motion->SetMotionExtractionFlags(MOTIONEXTRACT_CAPTURE_Z);
        }

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionInfo2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;
        MCORE_ASSERT(motion);

        // read the chunk
        FileFormat::Motion_Info2 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Motion_Info2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionFlags, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Unit Type                     = %d", fileInformation.mUnitType);
            MCore::LogDetailedInfo("   + Motion Extraction Flags       = 0x%x [capZ=%d]", fileInformation.mMotionExtractionFlags, (fileInformation.mMotionExtractionFlags & EMotionFX::MOTIONEXTRACT_CAPTURE_Z) ? 1 : 0);
        }

        motion->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        motion->SetFileUnitType(motion->GetUnitType());
        motion->SetMotionExtractionFlags(static_cast<EMotionFX::EMotionExtractionFlags>(fileInformation.mMotionExtractionFlags));

        return true;
    }

    //=================================================================================================

    // Actor Standard Material 3, with layers already inside it
    bool ChunkProcessorActorStdMaterial::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;
        Importer::ActorSettings* actorSettings = importParams.mActorSettings;

        MCORE_ASSERT(actor);

        // read the material header
        FileFormat::Actor_StandardMaterial material;
        file->Read(&material, sizeof(FileFormat::Actor_StandardMaterial));

        // convert the data into Core objects
        MCore::RGBAColor ambient(material.mAmbient.mR, material.mAmbient.mG, material.mAmbient.mB);
        MCore::RGBAColor diffuse(material.mDiffuse.mR, material.mDiffuse.mG, material.mDiffuse.mB);
        MCore::RGBAColor specular(material.mSpecular.mR, material.mSpecular.mG, material.mSpecular.mB);
        MCore::RGBAColor emissive(material.mEmissive.mR, material.mEmissive.mG, material.mEmissive.mB);

        // convert endian
        MCore::Endian::ConvertRGBAColor(&ambient, endianType);
        MCore::Endian::ConvertRGBAColor(&diffuse, endianType);
        MCore::Endian::ConvertRGBAColor(&specular, endianType);
        MCore::Endian::ConvertRGBAColor(&emissive, endianType);
        MCore::Endian::ConvertFloat(&material.mShine, endianType);
        MCore::Endian::ConvertFloat(&material.mShineStrength, endianType);
        MCore::Endian::ConvertFloat(&material.mOpacity, endianType);
        MCore::Endian::ConvertFloat(&material.mIOR, endianType);
        MCore::Endian::ConvertUnsignedInt32(&material.mLOD, endianType);

        // check if we want to import this material
        if (material.mLOD > 0 && importParams.mActorSettings->mLoadGeometryLODs == false)
        {
            // first read back to the position where we can read the chunk header
            size_t pos = file->GetPos();
            file->Seek(pos - sizeof(FileFormat::Actor_StandardMaterial) - sizeof(FileFormat::FileChunk));

            // read the chunk header
            FileFormat::FileChunk chunk;
            file->Read(&chunk, sizeof(FileFormat::FileChunk));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&chunk.mChunkID, endianType);
            MCore::Endian::ConvertUnsignedInt32(&chunk.mVersion, endianType);
            MCore::Endian::ConvertUnsignedInt32(&chunk.mSizeInBytes, endianType);

            // make sure we're dealing with the right chunk type to verify that this worked
            MCORE_ASSERT(chunk.mChunkID == FileFormat::ACTOR_CHUNK_STDMATERIAL);

            // seek past the end of the mesh data chunk
            file->Forward(chunk.mSizeInBytes);
            return true;
        }

        // read the material name
        const char* materialName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

        // print material information
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Material: Name = '%s'", materialName);
            MCore::LogDetailedInfo("    + LOD           : %d", material.mLOD);
            MCore::LogDetailedInfo("    + Ambient       : (%f, %f, %f)", ambient.r, ambient.g, ambient.b);
            MCore::LogDetailedInfo("    + Diffuse       : (%f, %f, %f)", diffuse.r, diffuse.g, diffuse.b);
            MCore::LogDetailedInfo("    + Specular      : (%f, %f, %f)", specular.r, specular.g, specular.b);
            MCore::LogDetailedInfo("    + Emissive      : (%f, %f, %f)", emissive.r, emissive.g, emissive.b);
            MCore::LogDetailedInfo("    + Shine         : %f", material.mShine);
            MCore::LogDetailedInfo("    + Shine strength: %f", material.mShineStrength);
            MCore::LogDetailedInfo("    + Opacity       : %f", material.mOpacity);
            MCore::LogDetailedInfo("    + IOR           : %f", material.mIOR);
            MCore::LogDetailedInfo("    + Double sided  : %d", material.mDoubleSided);
            MCore::LogDetailedInfo("    + WireFrame     : %d", material.mWireFrame);
            MCore::LogDetailedInfo("    + Num Layers    : %d", material.mNumLayers);
        }

        // create the material
        Material* stdMat = StandardMaterial::Create(materialName);
        StandardMaterial* mat = (StandardMaterial*)stdMat;

        // setup its properties
        mat->SetAmbient     (ambient);
        mat->SetDiffuse     (diffuse);
        mat->SetSpecular    (specular);
        mat->SetEmissive    (emissive);
        mat->SetDoubleSided (material.mDoubleSided != 0);
        mat->SetIOR         (material.mIOR);
        mat->SetOpacity     (material.mOpacity);
        mat->SetShine       (material.mShine);
        mat->SetWireFrame   (material.mWireFrame != 0);
        mat->SetShineStrength(material.mShineStrength);

        // add the material to the actor
        actor->AddMaterial(material.mLOD,  stdMat);

        // pre-alloc the number of layers
        if (actorSettings->mLoadStandardMaterialLayers)
        {
            mat->ReserveLayers(material.mNumLayers);
        }

        // read all layers
        for (uint32 i = 0; i < material.mNumLayers; ++i)
        {
            // read the layer from disk
            FileFormat::Actor_StandardMaterialLayer matLayer;
            file->Read(&matLayer, sizeof(FileFormat::Actor_StandardMaterialLayer));

            // read the texture file name
            const char* textureFileName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // if we should load the layers
            if (actorSettings->mLoadStandardMaterialLayers)
            {
                // convert endian
                MCore::Endian::ConvertUnsignedInt16(&matLayer.mMaterialNumber, endianType);
                MCore::Endian::ConvertFloat(&matLayer.mAmount, endianType);
                MCore::Endian::ConvertFloat(&matLayer.mRotationRadians, endianType);
                MCore::Endian::ConvertFloat(&matLayer.mUOffset, endianType);
                MCore::Endian::ConvertFloat(&matLayer.mVOffset, endianType);
                MCore::Endian::ConvertFloat(&matLayer.mUTiling, endianType);
                MCore::Endian::ConvertFloat(&matLayer.mVTiling, endianType);

                // add the layer to the material
                Material* baseMat = actor->GetMaterial(material.mLOD, matLayer.mMaterialNumber);
                MCORE_ASSERT(baseMat->GetType() == StandardMaterial::TYPE_ID);
                StandardMaterial* bmat = (StandardMaterial*)baseMat;
                StandardMaterialLayer* layer = bmat->AddLayer(StandardMaterialLayer::Create(matLayer.mMapType, textureFileName, matLayer.mAmount));

                // set the layer blend mode
                layer->SetBlendMode(matLayer.mBlendMode);
                layer->SetRotationRadians(matLayer.mRotationRadians);
                layer->SetUOffset(matLayer.mUOffset);
                layer->SetVOffset(matLayer.mVOffset);
                layer->SetUTiling(matLayer.mUTiling);
                layer->SetVTiling(matLayer.mVTiling);

                if (GetLogging())
                {
                    MCore::LogDetailedInfo("    - Material Layer");
                    MCore::LogDetailedInfo("       + Texture  = %s", textureFileName);
                    MCore::LogDetailedInfo("       + Material = %s (number %d)", mat->GetName(), matLayer.mMaterialNumber);
                    MCore::LogDetailedInfo("       + Amount   = %f", matLayer.mAmount);
                    MCore::LogDetailedInfo("       + MapType  = %d", matLayer.mMapType);
                    MCore::LogDetailedInfo("       + UOffset  = %f", matLayer.mUOffset);
                    MCore::LogDetailedInfo("       + VOffset  = %f", matLayer.mVOffset);
                    MCore::LogDetailedInfo("       + UTiling  = %f", matLayer.mUTiling);
                    MCore::LogDetailedInfo("       + VTiling  = %f", matLayer.mVTiling);
                    MCore::LogDetailedInfo("       + Rotation = %f (radians)", matLayer.mRotationRadians);
                }
            } // if we should load the layers
        } // for all layers

        return true;
    }


    //=================================================================================================

    // Actor Generic Material
    bool ChunkProcessorActorGenericMaterial::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;
        //Importer::ActorSettings* actorSettings = importParams.mActorSettings;

        MCORE_ASSERT(actor);

        // read the material header
        FileFormat::Actor_GenericMaterial material;
        file->Read(&material, sizeof(FileFormat::Actor_GenericMaterial));

        MCore::Endian::ConvertUnsignedInt32(&material.mLOD, endianType);

        // check if we want to import this material
        if (material.mLOD > 0 && importParams.mActorSettings->mLoadGeometryLODs == false)
        {
            // first read back to the position where we can read the chunk header
            size_t pos = file->GetPos();
            file->Seek(pos - sizeof(FileFormat::Actor_GenericMaterial) - sizeof(FileFormat::FileChunk));

            // read the chunk header
            FileFormat::FileChunk chunk;
            file->Read(&chunk, sizeof(FileFormat::FileChunk));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&chunk.mChunkID, endianType);
            MCore::Endian::ConvertUnsignedInt32(&chunk.mVersion, endianType);
            MCore::Endian::ConvertUnsignedInt32(&chunk.mSizeInBytes, endianType);

            // make sure we're dealing with the right chunk type to verify that this worked
            MCORE_ASSERT(chunk.mChunkID == FileFormat::ACTOR_CHUNK_GENERICMATERIAL);

            // seek past the end of the mesh data chunk
            file->Forward(chunk.mSizeInBytes);
            return true;
        }

        // read the material name
        const char* materialName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

        // print material information
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Generic Material: Name = '%s'", materialName);
            MCore::LogDetailedInfo("    + LOD           : %d", material.mLOD);
        }

        // create the material
        Material* genericMaterial = Material::Create(materialName);

        // add the material to the actor
        actor->AddMaterial(material.mLOD, genericMaterial);

        return true;
    }


    //=================================================================================================


    // Actor material attribute set
    bool ChunkProcessorActorMaterialAttributeSet::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;
        Importer::ActorSettings* actorSettings = importParams.mActorSettings;

        MCORE_ASSERT(actor);

        // read the header so we know what material to apply the set to
        FileFormat::Actor_MaterialAttributeSet setHeader;
        file->Read(&setHeader, sizeof(FileFormat::Actor_MaterialAttributeSet));
        MCore::Endian::ConvertUnsignedInt32(&setHeader.mMaterialIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&setHeader.mLODLevel, endianType);

        // check if we really want to load this set
        bool load = true;
        if (actorSettings->mLoadGeometryLODs == false && setHeader.mLODLevel > 0)
        {
            load = false;
        }

        // There used to be an AttributeSet here, read to move the stream
        ForwardAttributeSet(file, endianType);

        if (actor->GetNumLODLevels() > setHeader.mLODLevel && load)
        {
            if (GetLogging())
            {
                MCore::LogInfo("- Material Attribute Set:");
                MCore::LogInfo("   + Material Index: %d (%s)", setHeader.mMaterialIndex, actor->GetMaterial(setHeader.mLODLevel, setHeader.mMaterialIndex)->GetName());
                MCore::LogInfo("   + Material LOD:   %d", setHeader.mLODLevel);
            }
        }
        
        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorStdMaterialLayer::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);

        // read the layer from disk
        FileFormat::Actor_StandardMaterialLayer matLayer;
        file->Read(&matLayer, sizeof(FileFormat::Actor_StandardMaterialLayer));

        // convert endian
        MCore::Endian::ConvertUnsignedInt16(&matLayer.mMaterialNumber, endianType);
        MCore::Endian::ConvertFloat(&matLayer.mAmount, endianType);
        MCore::Endian::ConvertFloat(&matLayer.mRotationRadians, endianType);
        MCore::Endian::ConvertFloat(&matLayer.mUOffset, endianType);
        MCore::Endian::ConvertFloat(&matLayer.mVOffset, endianType);
        MCore::Endian::ConvertFloat(&matLayer.mUTiling, endianType);
        MCore::Endian::ConvertFloat(&matLayer.mVTiling, endianType);

        // read the texture file name
        const char* textureFileName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

        // add the layer to the material
        Material* baseMat = actor->GetMaterial(0, matLayer.mMaterialNumber);
        MCORE_ASSERT(baseMat->GetType() == StandardMaterial::TYPE_ID);
        StandardMaterial* mat = (StandardMaterial*)baseMat;
        StandardMaterialLayer* layer = mat->AddLayer(StandardMaterialLayer::Create(matLayer.mMapType, textureFileName, matLayer.mAmount));

        // set the layer blend mode
        layer->SetBlendMode(matLayer.mBlendMode);
        layer->SetRotationRadians(matLayer.mRotationRadians);
        layer->SetUOffset(matLayer.mUOffset);
        layer->SetVOffset(matLayer.mVOffset);
        layer->SetUTiling(matLayer.mUTiling);
        layer->SetVTiling(matLayer.mVTiling);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("    - Material Layer");
            MCore::LogDetailedInfo("       + Texture  = %s", textureFileName);
            MCore::LogDetailedInfo("       + Material = %s (number %d)", mat->GetName(), matLayer.mMaterialNumber);
            MCore::LogDetailedInfo("       + Amount   = %f", matLayer.mAmount);
            MCore::LogDetailedInfo("       + MapType  = %d", matLayer.mMapType);
            MCore::LogDetailedInfo("       + UOffset  = %f", matLayer.mUOffset);
            MCore::LogDetailedInfo("       + VOffset  = %f", matLayer.mVOffset);
            MCore::LogDetailedInfo("       + UTiling  = %f", matLayer.mUTiling);
            MCore::LogDetailedInfo("       + VTiling  = %f", matLayer.mVTiling);
            MCore::LogDetailedInfo("       + Rotation = %f (radians)", matLayer.mRotationRadians);
        }

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorMotionEventTrackTable::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;
        Importer::SkeletalMotionSettings* skelMotionSettings = importParams.mSkeletalMotionSettings;

        MCORE_ASSERT(motion);

        // read the motion event table header
        FileFormat::FileMotionEventTable fileEventTable;
        file->Read(&fileEventTable, sizeof(FileFormat::FileMotionEventTable));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileEventTable.mNumTracks, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Motion Event Table:");
            MCore::LogDetailedInfo("  + Num Tracks = %d", fileEventTable.mNumTracks);
        }

        // get the motion event table the reserve the event tracks
        MotionEventTable* motionEventTable = motion->GetEventTable();
        motionEventTable->ReserveNumTracks(fileEventTable.mNumTracks);

        // read all tracks
        AZStd::string trackName;
        MCore::Array<AZStd::string> typeStrings;
        MCore::Array<AZStd::string> paramStrings;
        MCore::Array<AZStd::string> mirrorTypeStrings;
        for (uint32 t = 0; t < fileEventTable.mNumTracks; ++t)
        {
            // read the motion event table header
            FileFormat::FileMotionEventTrack fileTrack;
            file->Read(&fileTrack, sizeof(FileFormat::FileMotionEventTrack));

            // read the track name
            trackName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.mNumEvents,             endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.mNumTypeStrings,        endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.mNumParamStrings,       endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileTrack.mNumMirrorTypeStrings,  endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Motion Event Track:");
                MCore::LogDetailedInfo("   + Name       = %s", trackName.c_str());
                MCore::LogDetailedInfo("   + Num events = %d", fileTrack.mNumEvents);
                MCore::LogDetailedInfo("   + Num types  = %d", fileTrack.mNumTypeStrings);
                MCore::LogDetailedInfo("   + Num params = %d", fileTrack.mNumParamStrings);
                MCore::LogDetailedInfo("   + Num mirror = %d", fileTrack.mNumMirrorTypeStrings);
                MCore::LogDetailedInfo("   + Enabled    = %d", fileTrack.mIsEnabled);
            }

            // the even type and parameter strings
            typeStrings.Resize(fileTrack.mNumTypeStrings);
            paramStrings.Resize(fileTrack.mNumParamStrings);
            mirrorTypeStrings.Resize(fileTrack.mNumMirrorTypeStrings);

            // read all type strings
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Event types:");
            }
            uint32 i;
            for (i = 0; i < fileTrack.mNumTypeStrings; ++i)
            {
                typeStrings[i] = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] = '%s'", i, typeStrings[i].c_str());
                }
            }

            // read all param strings
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Parameters:");
            }
            for (i = 0; i < fileTrack.mNumParamStrings; ++i)
            {
                paramStrings[i] = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] = '%s'", i, paramStrings[i].c_str());
                }
            }

            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Mirror Type Strings:");
            }
            for (i = 0; i < fileTrack.mNumMirrorTypeStrings; ++i)
            {
                mirrorTypeStrings[i] = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] = '%s'", i, mirrorTypeStrings[i].c_str());
                }
            }

            // check if we want to automatically register motion events or not
            bool autoRegister = false;
            if ((skelMotionSettings && skelMotionSettings->mAutoRegisterEvents) /* || (pmMotionSettings && pmMotionSettings->mAutoRegisterEvents)*/)
            {
                autoRegister = true;
            }

            // create the default event track
            MotionEventTrack* track = MotionEventTrack::Create(trackName.c_str(), motion);
            track->SetIsEnabled(fileTrack.mIsEnabled != 0);
            track->ReserveNumEvents(fileTrack.mNumEvents);
            track->ReserveNumParameters(fileTrack.mNumParamStrings);
            motionEventTable->AddTrack(track);

            // register all mirror events
            if (autoRegister)
            {
                GetEventManager().Lock();
                for (i = 0; i < fileTrack.mNumMirrorTypeStrings; ++i)
                {
                    if (GetEventManager().CheckIfHasEventType(mirrorTypeStrings[i].c_str()) == false)
                    {
                        const uint32 mirrorID = GetEventManager().RegisterEventType(mirrorTypeStrings[i].c_str());
                        if (GetLogging())
                        {
                            MCore::LogInfo("     Event '%s' has been automatically registered with ID %d", mirrorTypeStrings[i].c_str(), mirrorID);
                        }
                    }
                }
                GetEventManager().Unlock();
            }

            // read all motion events
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Motion Events:");
            }
            for (i = 0; i < fileTrack.mNumEvents; ++i)
            {
                // read the event header
                FileFormat::FileMotionEvent fileEvent;
                file->Read(&fileEvent, sizeof(FileFormat::FileMotionEvent));

                // convert endian
                MCore::Endian::ConvertUnsignedInt32(&fileEvent.mEventTypeIndex, endianType);
                MCore::Endian::ConvertUnsignedInt16(&fileEvent.mParamIndex, endianType);
                MCore::Endian::ConvertUnsignedInt32(&fileEvent.mMirrorTypeIndex, endianType);
                MCore::Endian::ConvertFloat(&fileEvent.mStartTime, endianType);
                MCore::Endian::ConvertFloat(&fileEvent.mEndTime, endianType);

                // print motion event information
                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     [%d] StartTime = %f  -  EndTime = %f  -  Type = '%s'  -  Param = '%s'  -  Mirror = '%s'", i, fileEvent.mStartTime, fileEvent.mEndTime, typeStrings[fileEvent.mEventTypeIndex].c_str(), paramStrings[fileEvent.mParamIndex].c_str(), mirrorTypeStrings[fileEvent.mMirrorTypeIndex].c_str());
                }

                // if we want to automatically register motion events
                if (autoRegister)
                {
                    GetEventManager().Lock();

                    // if the event type hasn't been registered yet
                    if (fileEvent.mEventTypeIndex != MCORE_INVALIDINDEX32)
                    {
                        if (GetEventManager().CheckIfHasEventType(typeStrings[fileEvent.mEventTypeIndex].c_str()) == false)
                        {
                            // register/link the event type with the free ID
                            const uint32 id = GetEventManager().RegisterEventType(typeStrings[fileEvent.mEventTypeIndex].c_str());
                            if (GetLogging())
                            {
                                MCore::LogInfo("     Event '%s' has been automatically registered with ID %d", typeStrings[fileEvent.mEventTypeIndex].c_str(), id);
                            }
                        }
                    }
                    else
                    {
                        // check if the empty string is registered, if not register it
                        if (GetEventManager().CheckIfHasEventType("") == false)
                        {
                            GetEventManager().RegisterEventType("");
                        }
                    }

                    GetEventManager().Unlock();
                }

                // add the event
                if (fileEvent.mMirrorTypeIndex != MCORE_INVALIDINDEX32)
                {
                    if (fileEvent.mEventTypeIndex != MCORE_INVALIDINDEX32)
                    {
                        track->AddEvent(fileEvent.mStartTime, fileEvent.mEndTime, typeStrings[fileEvent.mEventTypeIndex].c_str(), paramStrings[fileEvent.mParamIndex].c_str(), mirrorTypeStrings[fileEvent.mMirrorTypeIndex].c_str());
                    }
                    else
                    {
                        track->AddEvent(fileEvent.mStartTime, fileEvent.mEndTime, "", paramStrings[fileEvent.mParamIndex].c_str(), mirrorTypeStrings[fileEvent.mMirrorTypeIndex].c_str());
                    }
                }
                else
                {
                    if (fileEvent.mEventTypeIndex != MCORE_INVALIDINDEX32)
                    {
                        track->AddEvent(fileEvent.mStartTime, fileEvent.mEndTime, typeStrings[fileEvent.mEventTypeIndex].c_str(), paramStrings[fileEvent.mParamIndex].c_str(), "");
                    }
                    else
                    {
                        track->AddEvent(fileEvent.mStartTime, fileEvent.mEndTime, "", paramStrings[fileEvent.mParamIndex].c_str(), "");
                    }
                }
            }
        } // for all tracks

        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorInfo::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);

        // read the chunk
        FileFormat::Actor_Info fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Actor_Info));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mTrajectoryNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mNumLODs, endianType);
        MCore::Endian::ConvertFloat(&fileInformation.mRetargetRootOffset, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        // read the source application, original filename and the compilation date of the exporter string
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        const char* exporterCompilationDate = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        
        const char* name = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        actor->SetName(name);
        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Actor name             = '%s'", name);
        }

        // print motion event information
        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Exporter version       = v%d.%d", fileInformation.mExporterHighVersion, fileInformation.mExporterLowVersion);
            MCore::LogDetailedInfo("   + Num LODs               = %d", fileInformation.mNumLODs);
            MCore::LogDetailedInfo("   + Motion Extraction node = %d", fileInformation.mMotionExtractionNodeIndex);
            MCore::LogDetailedInfo("   + Retarget root offset   = %f", fileInformation.mRetargetRootOffset);
            MCore::LogDetailedInfo("   + UnitType               = %d", fileInformation.mUnitType);
        }

        actor->SetMotionExtractionNodeIndex(fileInformation.mMotionExtractionNodeIndex);
        //  actor->SetRetargetOffset( fileInformation.mRetargetRootOffset );
        actor->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        actor->SetFileUnitType(actor->GetUnitType());

        // preallocate memory for the lod level information that will follow
        if (importParams.mActorSettings->mLoadGeometryLODs)
        {
            actor->SetNumLODLevels(fileInformation.mNumLODs);
        }
        else
        {
            actor->SetNumLODLevels(1);
        }
        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorInfo2::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        // read the chunk
        FileFormat::Actor_Info2 fileInformation;
        file->Read(&fileInformation, sizeof(FileFormat::Actor_Info2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mMotionExtractionNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mRetargetRootNodeIndex, endianType);
        MCore::Endian::ConvertUnsignedInt32(&fileInformation.mNumLODs, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- File Information");
        }

        // read the source application, original filename and the compilation date of the exporter string
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        const char* exporterCompilationDate = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        
        const char* name = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        actor->SetName(name);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("   + Actor name             = '%s'", name);
            MCore::LogDetailedInfo("   + Exporter version       = v%d.%d", fileInformation.mExporterHighVersion, fileInformation.mExporterLowVersion);
            MCore::LogDetailedInfo("   + Num LODs               = %d", fileInformation.mNumLODs);
            MCore::LogDetailedInfo("   + Motion Extraction node = %d", fileInformation.mMotionExtractionNodeIndex);
            MCore::LogDetailedInfo("   + Retarget root node     = %d", fileInformation.mRetargetRootNodeIndex);
            MCore::LogDetailedInfo("   + UnitType               = %d", fileInformation.mUnitType);
        }

        actor->SetMotionExtractionNodeIndex(fileInformation.mMotionExtractionNodeIndex);
        actor->SetRetargetRootNodeIndex(fileInformation.mRetargetRootNodeIndex);
        actor->SetUnitType(static_cast<MCore::Distance::EUnitType>(fileInformation.mUnitType));
        actor->SetFileUnitType(actor->GetUnitType());

        // preallocate memory for the lod level information that will follow
        if (importParams.mActorSettings->mLoadGeometryLODs)
        {
            actor->SetNumLODLevels(fileInformation.mNumLODs);
        }
        else
        {
            actor->SetNumLODLevels(1);
        }
        return true;
    }

    //=================================================================================================

    bool ChunkProcessorActorMeshLOD::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;
        Importer::ActorSettings actorSettings = *importParams.mActorSettings;
        //actorSettings.mAutoRegisterMasterEnabled = false;

        MCORE_ASSERT(actor);

        // read the chunk
        FileFormat::Actor_MeshLODLevel meshLOD;
        file->Read(&meshLOD, sizeof(FileFormat::Actor_MeshLODLevel));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&meshLOD.mLODLevel, endianType);
        MCore::Endian::ConvertUnsignedInt32(&meshLOD.mSizeInBytes, endianType);

        // print motion LOD level information
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Mesh LOD Level %d", meshLOD.mLODLevel);
            MCore::LogDetailedInfo("   + SizeInBytes: %d", meshLOD.mSizeInBytes);
        }

        // create and fill the buffer with the mesh LOD model
        uint8* buffer = (uint8*)MCore::Allocate(meshLOD.mSizeInBytes, EMFX_MEMCATEGORY_IMPORTER);
        file->Read(buffer, meshLOD.mSizeInBytes);

        // load the lod model from memory and add it to the main actor
        Actor* lodModel = GetImporter().LoadActor(buffer, meshLOD.mSizeInBytes, &actorSettings);
        if (lodModel)
        {
            actor->AddLODLevel(false);
            actor->CopyLODLevel(lodModel, 0, actor->GetNumLODLevels() - 1, false);
        }
        else
        {
            MCore::LogError("LOD level loading failed!");
        }

        // get rid of the temporary buffer
        MCore::Free(buffer);

        return true;
    }


    //=================================================================================================


    // morph targets
    bool ChunkProcessorActorProgMorphTarget::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the expression part from disk
        FileFormat::Actor_MorphTarget morphTargetChunk;
        file->Read(&morphTargetChunk, sizeof(FileFormat::Actor_MorphTarget));

        // convert endian
        MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMin, endianType);
        MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMax, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mLOD, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mNumMeshDeformDeltas, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mNumTransformations, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mPhonemeSets, endianType);

        // get the expression name
        const char* morphTargetName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

        // get the level of detail of the expression part
        const uint32 morphTargetLOD = morphTargetChunk.mLOD;

        if (GetLogging())
        {
            MCore::LogDetailedInfo(" - Morph Target:");
            MCore::LogDetailedInfo("    + Name               = '%s'", morphTargetName);
            MCore::LogDetailedInfo("    + LOD Level          = %d", morphTargetChunk.mLOD);
            MCore::LogDetailedInfo("    + RangeMin           = %f", morphTargetChunk.mRangeMin);
            MCore::LogDetailedInfo("    + RangeMax           = %f", morphTargetChunk.mRangeMax);
            MCore::LogDetailedInfo("    + NumDeformDatas     = %d", morphTargetChunk.mNumMeshDeformDeltas);
            MCore::LogDetailedInfo("    + NumTransformations = %d", morphTargetChunk.mNumTransformations);
            MCore::LogDetailedInfo("    + PhonemeSets: %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets).c_str());
        }

        // check if the morph setup has already been created, if not create it
        if (actor->GetMorphSetup(morphTargetLOD) == nullptr)
        {
            // create the morph setup
            MorphSetup* morphSetup = MorphSetup::Create();

            // set the morph setup
            actor->SetMorphSetup(morphTargetLOD, morphSetup);
        }

        // create the mesh morph target
        MorphTargetStandard* morphTarget = MorphTargetStandard::Create(morphTargetName);

        // set the slider range
        morphTarget->SetRangeMin(morphTargetChunk.mRangeMin);
        morphTarget->SetRangeMax(morphTargetChunk.mRangeMax);

        // set the phoneme sets
        morphTarget->SetPhonemeSets((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets);

        // add the morph target
        actor->GetMorphSetup(morphTargetLOD)->AddMorphTarget(morphTarget);

        // pre-allocate space in the array of deform datas inside the morph target
        morphTarget->ReserveDeformDatas(morphTargetChunk.mNumMeshDeformDeltas);

        // read all deform datas
        // each node that gets deformed by the mesh morph results in a deform data object that we read here
        // so if a given morph target affects 3 meshes, there are 3 deform datas
        uint32 i;
        for (i = 0; i < morphTargetChunk.mNumMeshDeformDeltas; ++i)
        {
            FileFormat::Actor_MorphTargetMeshDeltas meshDeformDataChunk;
            file->Read(&meshDeformDataChunk, sizeof(FileFormat::Actor_MorphTargetMeshDeltas));

            // convert endian
            MCore::Endian::ConvertFloat(&meshDeformDataChunk.mMinValue, endianType);
            MCore::Endian::ConvertFloat(&meshDeformDataChunk.mMaxValue, endianType);
            MCore::Endian::ConvertUnsignedInt32(&meshDeformDataChunk.mNumVertices, endianType);
            MCore::Endian::ConvertUnsignedInt32(&meshDeformDataChunk.mNodeIndex, endianType);

            // get the deformation node
            MCORE_ASSERT(meshDeformDataChunk.mNodeIndex < actor->GetNumNodes());
            Node* deformNode = skeleton->GetNode(meshDeformDataChunk.mNodeIndex);
            MCORE_ASSERT(deformNode);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("    + Deform data #%d: Node='%s' (index=%d)", i, deformNode->GetName(), deformNode->GetNodeIndex());
                MCore::LogDetailedInfo("       - NumVertices = %d", meshDeformDataChunk.mNumVertices);
                MCore::LogDetailedInfo("       - MinValue    = %f", meshDeformDataChunk.mMinValue);
                MCore::LogDetailedInfo("       - MaxValue    = %f", meshDeformDataChunk.mMaxValue);
            }

            // if the node has no mesh, we can skip it
            Mesh* mesh = actor->GetMesh(morphTargetLOD, deformNode->GetNodeIndex());
            if (mesh) // if there is a mesh
            {
                // create and add the deform data object
                MorphTargetStandard::DeformData* deformData = aznew MorphTargetStandard::DeformData(deformNode->GetNodeIndex(), meshDeformDataChunk.mNumVertices);
                morphTarget->AddDeformData(deformData);

                // set the min and max values, used to define the compression/quantitization range for the positions
                deformData->mMinValue = meshDeformDataChunk.mMinValue;
                deformData->mMaxValue = meshDeformDataChunk.mMaxValue;

                // read the positions
                uint32 d;
                FileFormat::File16BitVector3 deltaPos;
                FileFormat::File16BitVector3* filePosVectors = (FileFormat::File16BitVector3*)MCore::Allocate(sizeof(FileFormat::File16BitVector3) * meshDeformDataChunk.mNumVertices, EMFX_MEMCATEGORY_IMPORTER);
                file->Read(filePosVectors, sizeof(FileFormat::File16BitVector3) * meshDeformDataChunk.mNumVertices);

                float compressMin = meshDeformDataChunk.mMinValue;
                float compressMax = meshDeformDataChunk.mMaxValue;
                /*
                            // if needed first of all find the real min and max value
                            if (MCore::GetCoordinateSystem().GetIsIdentityConversion() == false)    // if we need to convert coordinate system
                            {
                                // find the minimum and maximum values (the range of the floats in the deltas)
                                deformData->mMinValue = FLT_MAX;
                                deformData->mMaxValue = -FLT_MAX;

                                for (d=0; d<meshDeformDataChunk.mNumVertices; ++d)
                                {
                                    deltaPos = filePosVectors[d];

                                    // convert endian
                                    MCore::Endian::ConvertUnsignedInt16( &deltaPos.mX, endianType, 3 );

                                    // decompress and convert coordinate system
                                    MCore::Vector3 tempPos = MCore::Compressed16BitVector3(deltaPos.mX, deltaPos.mY, deltaPos.mZ).ToVector3(meshDeformDataChunk.mMinValue, meshDeformDataChunk.mMaxValue);
                                    MCore::GetCoordinateSystem().ConvertVector3( &tempPos );

                                    if (tempPos.SafeLength() > MCore::Math::epsilon)
                                    {
                                        const float minValue = MCore::Min3<float>( tempPos.x, tempPos.y, tempPos.z );
                                        const float maxValue = MCore::Max3<float>( tempPos.x, tempPos.y, tempPos.z );
                                        deformData->mMinValue = MCore::Min<float>( minValue, deformData->mMinValue );
                                        deformData->mMaxValue = MCore::Max<float>( maxValue, deformData->mMaxValue );
                                    }
                                }

                                // make sure the values won't be too small
                                if (deformData->mMaxValue - deformData->mMinValue < 1.0f)
                                {
                                    if (deformData->mMinValue < 0.0f && deformData->mMinValue > -1.0f)
                                        deformData->mMinValue = -1.0f;

                                    if (deformData->mMaxValue > 0.0f && deformData->mMaxValue < 1.0f)
                                        deformData->mMaxValue = 1.0f;
                                }

                                // use the following new deltas to recompress the deltas after coordinate system conversion in the next loop
                                compressMin = deformData->mMinValue;
                                compressMax = deformData->mMaxValue;
                            }
                */
                for (d = 0; d < meshDeformDataChunk.mNumVertices; ++d)
                {
                    deltaPos = filePosVectors[d];

                    // convert endian
                    MCore::Endian::ConvertUnsignedInt16(&deltaPos.mX, endianType, 3);

                    // decompress and convert coordinate system
                    AZ::Vector3 tempPos = MCore::Compressed16BitVector3(deltaPos.mX, deltaPos.mY, deltaPos.mZ).ToVector3(meshDeformDataChunk.mMinValue, meshDeformDataChunk.mMaxValue);

                    // compress the value after coordinate system conversion
                    deformData->mDeltas[d].mPosition = MCore::Compressed16BitVector3(tempPos, compressMin, compressMax);
                }
                MCore::Free(filePosVectors);

                // read the normals
                FileFormat::File8BitVector3* file8BitVectors = (FileFormat::File8BitVector3*)MCore::Allocate(sizeof(FileFormat::File8BitVector3) * meshDeformDataChunk.mNumVertices, EMFX_MEMCATEGORY_IMPORTER);
                file->Read(file8BitVectors, sizeof(FileFormat::File8BitVector3) * meshDeformDataChunk.mNumVertices);
                FileFormat::File8BitVector3 delta;
                for (d = 0; d < meshDeformDataChunk.mNumVertices; ++d)
                {
                    delta = file8BitVectors[d];

                    // decompress and convert coordinate system
                    AZ::Vector3 temp = MCore::Compressed8BitVector3(delta.mX, delta.mY, delta.mZ).ToVector3(-2.0f, +2.0f);

                    // compress the value after coordinate system conversion
                    deformData->mDeltas[d].mNormal = MCore::Compressed8BitVector3(temp, -2.0f, +2.0f);
                }

                // read the tangents
                file->Read(file8BitVectors, sizeof(FileFormat::File8BitVector3) * meshDeformDataChunk.mNumVertices);
                FileFormat::File8BitVector3 deltaT;
                for (d = 0; d < meshDeformDataChunk.mNumVertices; ++d)
                {
                    deltaT = file8BitVectors[d];

                    // decompress and convert coordinate system
                    AZ::Vector3 temp = MCore::Compressed8BitVector3(deltaT.mX, deltaT.mY, deltaT.mZ).ToVector3(-1.0f, +1.0f);

                    // compress the value after coordinate system conversion
                    deformData->mDeltas[d].mTangent = MCore::Compressed8BitVector3(temp, -1.0f, +1.0f);
                }
                MCore::Free(file8BitVectors);

                // read the vertex numbers
                uint32* fileIndices = (uint32*)MCore::Allocate(sizeof(uint32) * meshDeformDataChunk.mNumVertices, EMFX_MEMCATEGORY_IMPORTER);
                file->Read(fileIndices, sizeof(uint32) * meshDeformDataChunk.mNumVertices);
                MCore::Endian::ConvertUnsignedInt32(fileIndices, endianType, meshDeformDataChunk.mNumVertices);
                for (d = 0; d < meshDeformDataChunk.mNumVertices; ++d)
                {
                    deformData->mDeltas[d].mVertexNr = fileIndices[d];
                }
                MCore::Free(fileIndices);

                //-------------------------------
                // create the mesh deformer
                MeshDeformerStack* stack = actor->GetMeshDeformerStack(morphTargetLOD, deformNode->GetNodeIndex());

                // create the stack if it doesn't yet exist
                if (stack == nullptr)
                {
                    stack = MeshDeformerStack::Create(mesh);
                    actor->SetMeshDeformerStack(morphTargetLOD, deformNode->GetNodeIndex(), stack);
                }

                // try to see if there is already some  morph deformer
                MorphMeshDeformer* deformer = nullptr;
                MeshDeformerStack* stackPtr = stack;
                deformer = (MorphMeshDeformer*)stackPtr->FindDeformerByType(MorphMeshDeformer::TYPE_ID);
                if (deformer == nullptr) // there isn't one, so create and add one
                {
                    deformer = MorphMeshDeformer::Create(mesh);
                    stack->InsertDeformer(0, deformer);
                }

                // add the deform pass to the mesh deformer
                MorphMeshDeformer::DeformPass deformPass;
                deformPass.mDeformDataNr = morphTarget->GetNumDeformDatas() - 1;
                deformPass.mMorphTarget  = morphTarget;
                deformer->AddDeformPass(deformPass);
            }
            else // if there is no mesh, skip this mesh morph target
            {
                const uint32 numBytes = (meshDeformDataChunk.mNumVertices * sizeof(FileFormat::File16BitVector3)) + // positions
                    (meshDeformDataChunk.mNumVertices * sizeof(FileFormat::File8BitVector3)) +                  // normals
                    (meshDeformDataChunk.mNumVertices * sizeof(FileFormat::File8BitVector3)) +                  // tangents
                    (meshDeformDataChunk.mNumVertices * sizeof(uint32));                                        // vertex numbers

                // seek forward in the file
                file->Forward(numBytes);
            }
        }


        // read the facial transformations
        for (i = 0; i < morphTargetChunk.mNumTransformations; ++i)
        {
            // read the facial transformation from disk
            FileFormat::Actor_MorphTargetTransform transformChunk;
            file->Read(&transformChunk, sizeof(FileFormat::Actor_MorphTargetTransform));

            // create Core objects from the data
            AZ::PackedVector3f pos(transformChunk.mPosition.mX, transformChunk.mPosition.mY, transformChunk.mPosition.mZ);
            AZ::PackedVector3f scale(transformChunk.mScale.mX, transformChunk.mScale.mY, transformChunk.mScale.mZ);
            MCore::Quaternion rot(transformChunk.mRotation.mX, transformChunk.mRotation.mY, transformChunk.mRotation.mZ, transformChunk.mRotation.mW);
            MCore::Quaternion scaleRot(transformChunk.mScaleRotation.mX, transformChunk.mScaleRotation.mY, transformChunk.mScaleRotation.mZ, transformChunk.mScaleRotation.mW);

            // convert endian and coordinate system
            ConvertVector3(&pos, endianType);
            ConvertScale(&scale, endianType);
            ConvertQuaternion(&rot, endianType);
            ConvertQuaternion(&scaleRot, endianType);
            MCore::Endian::ConvertUnsignedInt32(&transformChunk.mNodeIndex, endianType);

            // create our transformation
            MorphTargetStandard::Transformation transform;
            transform.mPosition         = AZ::Vector3(pos);
            transform.mScale            = AZ::Vector3(scale);
            transform.mRotation         = rot;
            transform.mScaleRotation    = scaleRot;
            transform.mNodeIndex        = transformChunk.mNodeIndex;

            if (GetLogging())
            {
                MCore::LogDetailedInfo("    - Transform #%d: Node='%s' (index=%d)", i, skeleton->GetNode(transform.mNodeIndex)->GetName(), transform.mNodeIndex);
                MCore::LogDetailedInfo("       + Pos:      %f, %f, %f",
                    static_cast<float>(transform.mPosition.GetX()),
                    static_cast<float>(transform.mPosition.GetY()),
                    static_cast<float>(transform.mPosition.GetZ()));
                MCore::LogDetailedInfo("       + Rotation: %f, %f, %f %f", transform.mRotation.x, transform.mRotation.y, transform.mRotation.z, transform.mRotation.w);
                MCore::LogDetailedInfo("       + Scale:    %f, %f, %f",
                    static_cast<float>(transform.mScale.GetX()),
                    static_cast<float>(transform.mScale.GetY()),
                    static_cast<float>(transform.mScale.GetZ()));
                MCore::LogDetailedInfo("       + ScaleRot: %f, %f, %f %f", scaleRot.x, scaleRot.y, scaleRot.z, scaleRot.w);
            }

            // add the transformation to the bones expression part
            morphTarget->AddTransformation(transform);
        }

        return true;
    }


    //-----------------

    // the node groups chunk
    bool ChunkProcessorActorNodeGroups::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);

        // read the number of groups to follow
        uint16 numGroups;
        file->Read(&numGroups, sizeof(uint16));
        MCore::Endian::ConvertUnsignedInt16(&numGroups, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Node groups: %d", numGroups);
        }

        // read all groups
        for (uint32 i = 0; i < numGroups; ++i)
        {
            // read the group header
            FileFormat::Actor_NodeGroup fileGroup;
            file->Read(&fileGroup, sizeof(FileFormat::Actor_NodeGroup));
            MCore::Endian::ConvertUnsignedInt16(&fileGroup.mNumNodes, endianType);

            // read the group name
            const char* groupName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // log some info
            if (GetLogging())
            {
                MCore::LogDetailedInfo("   + Group '%s'", groupName);
                MCore::LogDetailedInfo("     - Num nodes: %d", fileGroup.mNumNodes);
                MCore::LogDetailedInfo("     - Disabled on default: %s", fileGroup.mDisabledOnDefault ? "Yes" : "No");
            }

            // create the new group inside the actor
            NodeGroup* newGroup = NodeGroup::Create(groupName, fileGroup.mNumNodes, fileGroup.mDisabledOnDefault ? false : true);

            // read the node numbers
            uint16 nodeIndex;
            for (uint16 n = 0; n < fileGroup.mNumNodes; ++n)
            {
                file->Read(&nodeIndex, sizeof(uint16));
                MCore::Endian::ConvertUnsignedInt16(&nodeIndex, endianType);
                newGroup->SetNode(n, nodeIndex);
            }

            // add the group to the actor
            actor->AddNodeGroup(newGroup);
        }

        return true;
    }


    // all submotions in one chunk
    bool ChunkProcessorMotionMorphSubMotions::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Motion* motion = importParams.mMotion;
        MCORE_ASSERT(motion);

        // cast to  morph motion
        SkeletalMotion* morphMotion = (SkeletalMotion*)motion;

        FileFormat::Motion_MorphSubMotions subMotionsHeader;
        file->Read(&subMotionsHeader, sizeof(FileFormat::Motion_MorphSubMotions));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&subMotionsHeader.mNumSubMotions, endianType);

        // pre-allocate the number of submotions
        morphMotion->SetNumMorphSubMotions(subMotionsHeader.mNumSubMotions);

        // for all submotions
        for (uint32 s = 0; s < subMotionsHeader.mNumSubMotions; ++s)
        {
            // get the  morph motion part
            FileFormat::Motion_MorphSubMotion morphSubMotionChunk;
            file->Read(&morphSubMotionChunk, sizeof(FileFormat::Motion_MorphSubMotion));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&morphSubMotionChunk.mNumKeys, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphSubMotionChunk.mPhonemeSet, endianType);
            MCore::Endian::ConvertFloat(&morphSubMotionChunk.mPoseWeight, endianType);
            MCore::Endian::ConvertFloat(&morphSubMotionChunk.mMinWeight, endianType);
            MCore::Endian::ConvertFloat(&morphSubMotionChunk.mMaxWeight, endianType);

            // read the name of the submotion
            const char* name = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // create a new  morph motion part and set it's id
            const int32 id = MCore::GetStringIdPool().GenerateIdForString(name);
            MorphSubMotion* morphSubMotion = MorphSubMotion::Create(id);

            // get the part's keytrack
            KeyTrackLinear<float, MCore::Compressed16BitFloat>* keytrack = morphSubMotion->GetKeyTrack();
            keytrack->SetNumKeys(morphSubMotionChunk.mNumKeys);

            // set the interpolator(always using hermite interpolation)
            //keytrack->SetInterpolator( new HermiteInterpolator<float, MCore::Compressed16BitFloat>() );
            //      keytrack->SetInterpolator( new LinearInterpolator<float, MCore::Compressed16BitFloat>() );

            if (GetLogging())
            {
                MCore::LogDetailedInfo("    - Morph Submotion: %s", name);
                MCore::LogDetailedInfo("       + NrKeys             = %d", morphSubMotionChunk.mNumKeys);
                MCore::LogDetailedInfo("       + Pose Weight        = %f", morphSubMotionChunk.mPoseWeight);
                MCore::LogDetailedInfo("       + Minimum Weight     = %f", morphSubMotionChunk.mMinWeight);
                MCore::LogDetailedInfo("       + Maximum Weight     = %f", morphSubMotionChunk.mMaxWeight);
                MCore::LogDetailedInfo("       + PhonemeSet         = %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphSubMotionChunk.mPhonemeSet).c_str());
            }

            // set the pose weight
            morphSubMotion->SetPoseWeight(morphSubMotionChunk.mPoseWeight);

            // set the phoneme set
            morphSubMotion->SetPhonemeSet((MorphTarget::EPhonemeSet)morphSubMotionChunk.mPhonemeSet);

            // add keyframes
            for (uint32 i = 0; i < morphSubMotionChunk.mNumKeys; ++i)
            {
                FileFormat::Motion_UnsignedShortKey keyframeChunk;

                // read the keyframe
                file->Read(&keyframeChunk, sizeof(FileFormat::Motion_UnsignedShortKey));

                // convert endian
                MCore::Endian::ConvertFloat(&keyframeChunk.mTime, endianType);
                MCore::Endian::ConvertUnsignedInt16(&keyframeChunk.mValue, endianType);

                // gives it into range of [minWeight..maxWeight]
                //const float normalizedValue = keyframeChunk.mValue / 65535.0f;

                // calculate the range [minWeight..maxWeight]
                //const float range = morphSubMotionChunk.mMaxWeight - morphSubMotionChunk.mMinWeight;

                // calculate the absolute value
                //const float absoluteValue = morphSubMotionChunk.mMinWeight + (range * normalizedValue);

                //LogInfo("     + Key#%i: Time=%f Value=%i", i, keyframeChunk.mTime, keyframeChunk.mValue);

                // add the keyframe to the keytrack
                keytrack->SetStorageTypeKey(i, keyframeChunk.mTime, /*absoluteValue*/ keyframeChunk.mValue);
            }

            // init the keytrack
            keytrack->Init();

            // add the  morph submotion
            morphMotion->SetMorphSubMotion(s, morphSubMotion);
        } // for all submotions

        // each keytrack which stops before maxtime gets an ending key with time=maxTime.
        //Importer::SyncMotionTrackEnds( morphMotion );

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // the material info chunk
    bool ChunkProcessorActorMaterialInfo::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);

        // read the file data
        FileFormat::Actor_MaterialInfo matInfo;
        file->Read(&matInfo, sizeof(FileFormat::Actor_MaterialInfo));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&matInfo.mLOD, endianType);
        MCore::Endian::ConvertUnsignedInt32(&matInfo.mNumTotalMaterials, endianType);
        MCore::Endian::ConvertUnsignedInt32(&matInfo.mNumStandardMaterials, endianType);
        MCore::Endian::ConvertUnsignedInt32(&matInfo.mNumFXMaterials, endianType);
        MCore::Endian::ConvertUnsignedInt32(&matInfo.mNumGenericMaterials, endianType);

        // log details
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Material Totals Information:");
            MCore::LogDetailedInfo("   + LOD                    = %d", matInfo.mLOD);
            MCore::LogDetailedInfo("   + Total materials        = %d", matInfo.mNumTotalMaterials);
            MCore::LogDetailedInfo("   + Num generic materials  = %d", matInfo.mNumGenericMaterials);
            MCore::LogDetailedInfo("   + Num standard materials = %d", matInfo.mNumStandardMaterials);
            MCore::LogDetailedInfo("   + Num FX materials       = %d", matInfo.mNumFXMaterials);
        }

        // pre-allocate memory in the array of materials for the given LOD level
        if (importParams.mActorSettings->mLoadGeometryLODs)
        {
            actor->ReserveMaterials(matInfo.mLOD, matInfo.mNumTotalMaterials);
        }
        else
        {
            if (matInfo.mLOD == 0)
            {
                actor->ReserveMaterials(0, matInfo.mNumTotalMaterials);
            }
        }

        return true;
    }

    //------------------------------------------------------------------------------------------

    // morph targets
    bool ChunkProcessorActorProgMorphTargets::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the header
        FileFormat::Actor_MorphTargets morphTargetsHeader;
        file->Read(&morphTargetsHeader, sizeof(FileFormat::Actor_MorphTargets));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.mNumMorphTargets, endianType);
        MCore::Endian::ConvertUnsignedInt32(&morphTargetsHeader.mLOD, endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Morph targets: %d (LOD=%d)", morphTargetsHeader.mNumMorphTargets, morphTargetsHeader.mLOD);
        }

        // check if the morph setup has already been created, if not create it
        if (actor->GetMorphSetup(morphTargetsHeader.mLOD) == nullptr)
        {
            // create the morph setup
            MorphSetup* morphSetup = MorphSetup::Create();

            // set the morph setup
            actor->SetMorphSetup(morphTargetsHeader.mLOD, morphSetup);
        }

        // pre-allocate the morph targets
        MorphSetup* setup = actor->GetMorphSetup(morphTargetsHeader.mLOD);
        setup->ReserveMorphTargets(morphTargetsHeader.mNumMorphTargets);

        // read in all morph targets
        for (uint32 mt = 0; mt < morphTargetsHeader.mNumMorphTargets; ++mt)
        {
            // read the expression part from disk
            FileFormat::Actor_MorphTarget morphTargetChunk;
            file->Read(&morphTargetChunk, sizeof(FileFormat::Actor_MorphTarget));

            // convert endian
            MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMin, endianType);
            MCore::Endian::ConvertFloat(&morphTargetChunk.mRangeMax, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mLOD, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mNumMeshDeformDeltas, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mNumTransformations, endianType);
            MCore::Endian::ConvertUnsignedInt32(&morphTargetChunk.mPhonemeSets, endianType);

            // make sure they match
            MCORE_ASSERT(morphTargetChunk.mLOD == morphTargetsHeader.mLOD);

            // get the expression name
            const char* morphTargetName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // get the level of detail of the expression part
            const uint32 morphTargetLOD = morphTargetChunk.mLOD;

            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + Morph Target:");
                MCore::LogDetailedInfo("     - Name               = '%s'", morphTargetName);
                MCore::LogDetailedInfo("     - LOD Level          = %d", morphTargetChunk.mLOD);
                MCore::LogDetailedInfo("     - RangeMin           = %f", morphTargetChunk.mRangeMin);
                MCore::LogDetailedInfo("     - RangeMax           = %f", morphTargetChunk.mRangeMax);
                MCore::LogDetailedInfo("     - NumDeformDatas     = %d", morphTargetChunk.mNumMeshDeformDeltas);
                MCore::LogDetailedInfo("     - NumTransformations = %d", morphTargetChunk.mNumTransformations);
                MCore::LogDetailedInfo("     - PhonemeSets: %s", MorphTarget::GetPhonemeSetString((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets).c_str());
            }

            // create the mesh morph target
            MorphTargetStandard* morphTarget = MorphTargetStandard::Create(morphTargetName);

            // set the slider range
            morphTarget->SetRangeMin(morphTargetChunk.mRangeMin);
            morphTarget->SetRangeMax(morphTargetChunk.mRangeMax);

            // set the phoneme sets
            morphTarget->SetPhonemeSets((MorphTarget::EPhonemeSet)morphTargetChunk.mPhonemeSets);

            // add the morph target
            setup->AddMorphTarget(morphTarget);

            // pre-allocate space in the array of deform datas inside the morph target
            morphTarget->ReserveDeformDatas(morphTargetChunk.mNumMeshDeformDeltas);

            // the same for the transformations
            morphTarget->ReserveTransformations(morphTargetChunk.mNumTransformations);

            // read all deform datas
            // each node that gets deformed by the mesh morph results in a deform data object that we read here
            // so if a given morph target affects 3 meshes, there are 3 deform datas
            uint32 i;
            for (i = 0; i < morphTargetChunk.mNumMeshDeformDeltas; ++i)
            {
                FileFormat::Actor_MorphTargetMeshDeltas meshDeformDataChunk;
                file->Read(&meshDeformDataChunk, sizeof(FileFormat::Actor_MorphTargetMeshDeltas));

                // convert endian
                MCore::Endian::ConvertFloat(&meshDeformDataChunk.mMinValue, endianType);
                MCore::Endian::ConvertFloat(&meshDeformDataChunk.mMaxValue, endianType);
                MCore::Endian::ConvertUnsignedInt32(&meshDeformDataChunk.mNumVertices, endianType);
                MCore::Endian::ConvertUnsignedInt32(&meshDeformDataChunk.mNodeIndex, endianType);

                // get the deformation node
                MCORE_ASSERT(meshDeformDataChunk.mNodeIndex < actor->GetNumNodes());
                Node* deformNode = skeleton->GetNode(meshDeformDataChunk.mNodeIndex);
                MCORE_ASSERT(deformNode);

                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     - Deform data #%d: Node='%s' (index=%d)", i, deformNode->GetName(), deformNode->GetNodeIndex());
                    MCore::LogDetailedInfo("        + NumVertices = %d", meshDeformDataChunk.mNumVertices);
                    MCore::LogDetailedInfo("        + MinValue    = %f", meshDeformDataChunk.mMinValue);
                    MCore::LogDetailedInfo("        + MaxValue    = %f", meshDeformDataChunk.mMaxValue);
                }

                // if the node has no mesh, we can skip it
                Mesh* mesh = actor->GetMesh(morphTargetLOD, deformNode->GetNodeIndex());
                if (mesh) // if there is a mesh
                {
                    // create and add the deform data object
                    MorphTargetStandard::DeformData* deformData = aznew MorphTargetStandard::DeformData(deformNode->GetNodeIndex(), meshDeformDataChunk.mNumVertices);
                    morphTarget->AddDeformData(deformData);

                    // set the min and max values, used to define the compression/quantitization range for the positions
                    deformData->mMinValue = meshDeformDataChunk.mMinValue;
                    deformData->mMaxValue = meshDeformDataChunk.mMaxValue;

                    // read the positions
                    uint32 d;
                    FileFormat::File16BitVector3 deltaPos;
                    FileFormat::File16BitVector3* filePosVectors = (FileFormat::File16BitVector3*)MCore::Allocate(sizeof(FileFormat::File16BitVector3) * meshDeformDataChunk.mNumVertices, EMFX_MEMCATEGORY_IMPORTER);
                    file->Read(filePosVectors, sizeof(FileFormat::File16BitVector3) * meshDeformDataChunk.mNumVertices);

                    float compressMin = meshDeformDataChunk.mMinValue;
                    float compressMax = meshDeformDataChunk.mMaxValue;

                    // read and coordinate system convert the deltas
                    for (d = 0; d < meshDeformDataChunk.mNumVertices; ++d)
                    {
                        deltaPos = filePosVectors[d];

                        // convert endian
                        MCore::Endian::ConvertUnsignedInt16(&deltaPos.mX, endianType, 3);

                        // decompress and convert coordinate system
                        AZ::Vector3 tempPos = MCore::Compressed16BitVector3(deltaPos.mX, deltaPos.mY, deltaPos.mZ).ToVector3(meshDeformDataChunk.mMinValue, meshDeformDataChunk.mMaxValue);

                        // compress the value after coordinate system conversion
                        deformData->mDeltas[d].mPosition = MCore::Compressed16BitVector3(tempPos, compressMin, compressMax);
                    }
                    MCore::Free(filePosVectors);

                    // read the normals
                    FileFormat::File8BitVector3* file8BitVectors = (FileFormat::File8BitVector3*)MCore::Allocate(sizeof(FileFormat::File8BitVector3) * meshDeformDataChunk.mNumVertices, EMFX_MEMCATEGORY_IMPORTER);
                    file->Read(file8BitVectors, sizeof(FileFormat::File8BitVector3) * meshDeformDataChunk.mNumVertices);
                    FileFormat::File8BitVector3 delta;
                    for (d = 0; d < meshDeformDataChunk.mNumVertices; ++d)
                    {
                        delta = file8BitVectors[d];

                        // decompress and convert coordinate system
                        AZ::Vector3 temp = MCore::Compressed8BitVector3(delta.mX, delta.mY, delta.mZ).ToVector3(-1.0f, +1.0f);

                        // compress the value after coordinate system conversion
                        deformData->mDeltas[d].mNormal = MCore::Compressed8BitVector3(temp, -1.0f, +1.0f);
                    }

                    // read the tangents
                    file->Read(file8BitVectors, sizeof(FileFormat::File8BitVector3) * meshDeformDataChunk.mNumVertices);
                    FileFormat::File8BitVector3 deltaT;
                    for (d = 0; d < meshDeformDataChunk.mNumVertices; ++d)
                    {
                        deltaT = file8BitVectors[d];

                        // decompress and convert coordinate system
                        AZ::Vector3 temp = MCore::Compressed8BitVector3(deltaT.mX, deltaT.mY, deltaT.mZ).ToVector3(-1.0f, +1.0f);

                        // compress the value after coordinate system conversion
                        deformData->mDeltas[d].mTangent = MCore::Compressed8BitVector3(temp, -1.0f, +1.0f);
                    }
                    MCore::Free(file8BitVectors);

                    // read the vertex numbers
                    uint32* fileIndices = (uint32*)MCore::Allocate(sizeof(uint32) * meshDeformDataChunk.mNumVertices, EMFX_MEMCATEGORY_IMPORTER);
                    file->Read(fileIndices, sizeof(uint32) * meshDeformDataChunk.mNumVertices);
                    MCore::Endian::ConvertUnsignedInt32(fileIndices, endianType, meshDeformDataChunk.mNumVertices);
                    for (d = 0; d < meshDeformDataChunk.mNumVertices; ++d)
                    {
                        deformData->mDeltas[d].mVertexNr = fileIndices[d];
                    }
                    MCore::Free(fileIndices);

                    //-------------------------------
                    // create the mesh deformer
                    MeshDeformerStack* stack = actor->GetMeshDeformerStack(morphTargetLOD, deformNode->GetNodeIndex());

                    // create the stack if it doesn't yet exist
                    if (stack == nullptr)
                    {
                        stack = MeshDeformerStack::Create(mesh);
                        actor->SetMeshDeformerStack(morphTargetLOD, deformNode->GetNodeIndex(), stack);
                    }

                    // try to see if there is already some  morph deformer
                    MorphMeshDeformer* deformer = nullptr;
                    MeshDeformerStack* stackPtr = stack;
                    deformer = (MorphMeshDeformer*)stackPtr->FindDeformerByType(MorphMeshDeformer::TYPE_ID);
                    if (deformer == nullptr) // there isn't one, so create and add one
                    {
                        deformer = MorphMeshDeformer::Create(mesh);
                        stack->InsertDeformer(0, deformer);
                        deformer->ReserveDeformPasses(morphTargetsHeader.mNumMorphTargets); // assume that this mesh is influenced by all morph targets
                    }

                    // add the deform pass to the mesh deformer
                    MorphMeshDeformer::DeformPass deformPass;
                    deformPass.mDeformDataNr = morphTarget->GetNumDeformDatas() - 1;
                    deformPass.mMorphTarget  = morphTarget;
                    deformer->AddDeformPass(deformPass);
                    //-------------------------------
                }
                else // if there is no mesh, skip this mesh morph target
                {
                    const uint32 numBytes = (meshDeformDataChunk.mNumVertices * sizeof(FileFormat::File16BitVector3)) + // positions
                        (meshDeformDataChunk.mNumVertices * sizeof(FileFormat::File8BitVector3)) +                  // normals
                        (meshDeformDataChunk.mNumVertices * sizeof(FileFormat::File8BitVector3)) +                  // tangents
                        (meshDeformDataChunk.mNumVertices * sizeof(uint32));                                        // vertex numbers

                    // seek forward in the file
                    file->Forward(numBytes);
                }
            }


            // read the facial transformations
            for (i = 0; i < morphTargetChunk.mNumTransformations; ++i)
            {
                // read the facial transformation from disk
                FileFormat::Actor_MorphTargetTransform transformChunk;
                file->Read(&transformChunk, sizeof(FileFormat::Actor_MorphTargetTransform));

                // create Core objects from the data
                AZ::PackedVector3f pos(transformChunk.mPosition.mX, transformChunk.mPosition.mY, transformChunk.mPosition.mZ);
                AZ::PackedVector3f scale(transformChunk.mScale.mX, transformChunk.mScale.mY, transformChunk.mScale.mZ);
                MCore::Quaternion rot(transformChunk.mRotation.mX, transformChunk.mRotation.mY, transformChunk.mRotation.mZ, transformChunk.mRotation.mW);
                MCore::Quaternion scaleRot(transformChunk.mScaleRotation.mX, transformChunk.mScaleRotation.mY, transformChunk.mScaleRotation.mZ, transformChunk.mScaleRotation.mW);

                // convert endian and coordinate system
                ConvertVector3(&pos, endianType);
                ConvertScale(&scale, endianType);
                ConvertQuaternion(&rot, endianType);
                ConvertQuaternion(&scaleRot, endianType);
                MCore::Endian::ConvertUnsignedInt32(&transformChunk.mNodeIndex, endianType);

                // create our transformation
                MorphTargetStandard::Transformation transform;
                transform.mPosition         = AZ::Vector3(pos);
                transform.mScale            = AZ::Vector3(scale);
                transform.mRotation         = rot;
                transform.mScaleRotation    = scaleRot;
                transform.mNodeIndex        = transformChunk.mNodeIndex;

                if (GetLogging())
                {
                    MCore::LogDetailedInfo("     + Transform #%d: Node='%s' (index=%d)", i, skeleton->GetNode(transform.mNodeIndex)->GetName(), transform.mNodeIndex);
                    MCore::LogDetailedInfo("        - Pos:      %f, %f, %f",
                        static_cast<float>(transform.mPosition.GetX()),
                        static_cast<float>(transform.mPosition.GetY()),
                        static_cast<float>(transform.mPosition.GetZ()));
                    MCore::LogDetailedInfo("        - Rotation: %f, %f, %f %f", transform.mRotation.x, transform.mRotation.y, transform.mRotation.z, transform.mRotation.w);
                    MCore::LogDetailedInfo("        - Scale:    %f, %f, %f",
                        static_cast<float>(transform.mScale.GetX()),
                        static_cast<float>(transform.mScale.GetY()),
                        static_cast<float>(transform.mScale.GetZ()));
                    MCore::LogDetailedInfo("        - ScaleRot: %f, %f, %f %f", scaleRot.x, scaleRot.y, scaleRot.z, scaleRot.w);
                }

                // add the transformation to the bones expression part
                morphTarget->AddTransformation(transform);
            }
        } // for all morph targets

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // the node motion sources chunk
    bool ChunkProcessorActorNodeMotionSources::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        uint32 i;
        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the file data
        FileFormat::Actor_NodeMotionSources2 nodeMotionSourcesChunk;
        file->Read(&nodeMotionSourcesChunk, sizeof(FileFormat::Actor_NodeMotionSources2));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodeMotionSourcesChunk.mNumNodes, endianType);
        const uint32 numNodes = nodeMotionSourcesChunk.mNumNodes;
        if (numNodes == 0)
        {
            return true;
        }

        // allocate the node motion sources array and recheck the number of nodes with the
        // information given in this chunk
        MCORE_ASSERT(actor->GetNumNodes() == numNodes);
        actor->AllocateNodeMirrorInfos();

        // read all node motion sources and convert endian
        for (i = 0; i < numNodes; ++i)
        {
            uint16 sourceNode;
            file->Read(&sourceNode, sizeof(uint16));
            MCore::Endian::ConvertUnsignedInt16(&sourceNode, endianType);
            actor->GetNodeMirrorInfo(i).mSourceNode = sourceNode;
        }

        // read all axes
        for (i = 0; i < numNodes; ++i)
        {
            uint8 axis;
            file->Read(&axis, sizeof(uint8));
            actor->GetNodeMirrorInfo(i).mAxis = axis;
        }

        // read all flags
        for (i = 0; i < numNodes; ++i)
        {
            uint8 flags;
            file->Read(&flags, sizeof(uint8));
            actor->GetNodeMirrorInfo(i).mFlags = flags;
        }

        // log details
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Node Motion Sources (%i):", numNodes);
            for (i = 0; i < numNodes; ++i)
            {
                if (actor->GetNodeMirrorInfo(i).mSourceNode != MCORE_INVALIDINDEX16)
                {
                    MCore::LogDetailedInfo("   + '%s' (%i) -> '%s' (%i) [axis=%d] [flags=%d]", skeleton->GetNode(i)->GetName(), i, skeleton->GetNode(actor->GetNodeMirrorInfo(i).mSourceNode)->GetName(), actor->GetNodeMirrorInfo(i).mSourceNode, actor->GetNodeMirrorInfo(i).mAxis, actor->GetNodeMirrorInfo(i).mFlags);
                }
            }
        }

        return true;
    }


    //----------------------------------------------------------------------------------------------------------

    //
    bool ChunkProcessorActorAttachmentNodes::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;
        Actor* actor = importParams.mActor;

        uint32 i;
        MCORE_ASSERT(actor);
        Skeleton* skeleton = actor->GetSkeleton();

        // read the file data
        FileFormat::Actor_AttachmentNodes attachmentNodesChunk;
        file->Read(&attachmentNodesChunk, sizeof(FileFormat::Actor_AttachmentNodes));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&attachmentNodesChunk.mNumNodes, endianType);
        const uint32 numAttachmentNodes = attachmentNodesChunk.mNumNodes;

        // read all node attachment nodes
        for (i = 0; i < numAttachmentNodes; ++i)
        {
            // get the attachment node index and endian convert it
            uint16 nodeNr;
            file->Read(&nodeNr, sizeof(uint16));
            MCore::Endian::ConvertUnsignedInt16(&nodeNr, endianType);

            // get the attachment node from the actor
            MCORE_ASSERT(nodeNr < actor->GetNumNodes());
            Node* node = skeleton->GetNode(nodeNr);

            // enable the attachment node flag
            node->SetIsAttachmentNode(true);
        }

        // log details
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Attachment Nodes (%i):", numAttachmentNodes);

            const uint32 numNodes = actor->GetNumNodes();
            for (i = 0; i < numNodes; ++i)
            {
                // get the current node
                Node* node = skeleton->GetNode(i);

                // only log the attachment nodes
                if (node->GetIsAttachmentNode())
                {
                    MCore::LogDetailedInfo("   + '%s' (%i)", node->GetName(), node->GetNodeIndex());
                }
            }
        }

        return true;
    }


    //----------------------------------------------------------------------------------------------------------

    // animGraph state transitions
    bool ChunkProcessorAnimGraphStateTransitions::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        // read the number of transitions to follow
        uint32 numTransitions;
        file->Read(&numTransitions, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numTransitions, importParams.mEndianType);

        // read the state machine index
        uint32 stateMachineIndex;
        file->Read(&stateMachineIndex, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&stateMachineIndex, importParams.mEndianType);

        // get the loaded anim graph nodes
        MCore::Array<AnimGraphNode*>& blendNodes = SharedHelperData::GetBlendNodes(importParams.mSharedData);
        if (stateMachineIndex >= blendNodes.GetLength())
        {
            if (GetLogging())
            {
                AZ_Error("EMotionFX", false, "State machine refers to invalid blend node, state machine index: %d, amount of blend node: %d", stateMachineIndex, blendNodes.GetLength());
            }
            return false;
        }
        AZ_Assert(azrtti_typeid(blendNodes[stateMachineIndex]) == azrtti_typeid<AnimGraphStateMachine>(), "ChunkProcessorAnimGraphStateTransitions::Process : Unexpected node type expected AnimGraphStateMachine. Found %u instead", azrtti_typeid(blendNodes[stateMachineIndex]));
        AnimGraphStateMachine* stateMachine = static_cast<AnimGraphStateMachine*>(blendNodes[stateMachineIndex]);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num transitions for state machine '%s' = %d", blendNodes[stateMachineIndex]->GetName(), numTransitions);
        }

        stateMachine->ReserveTransitions(numTransitions);

        // read the transitions
        FileFormat::AnimGraph_StateTransition transition;
        for (uint32 i = 0; i < numTransitions; ++i)
        {
            // read the transition
            file->Read(&transition, sizeof(FileFormat::AnimGraph_StateTransition));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&transition.mSourceNode,   importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&transition.mDestNode,     importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&transition.mNumConditions, importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&transition.mStartOffsetX,   importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&transition.mStartOffsetY,   importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&transition.mEndOffsetX,     importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&transition.mEndOffsetY,     importParams.mEndianType);

            //----------------------------------------------
            // read the node header
            FileFormat::AnimGraph_NodeHeader nodeHeader;
            file->Read(&nodeHeader, sizeof(FileFormat::AnimGraph_NodeHeader));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mTypeID,               importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mParentIndex,          importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mVersion,              importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumCustomDataBytes,   importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumChildNodes,        importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumAttributes,        importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&nodeHeader.mVisualPosX,             importParams.mEndianType);
            MCore::Endian::ConvertSignedInt32(&nodeHeader.mVisualPosY,             importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mVisualizeColor,       importParams.mEndianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("- State Transition Node:");
                MCore::LogDetailedInfo("  + Type            = %d", nodeHeader.mTypeID);
                MCore::LogDetailedInfo("  + Version         = %d", nodeHeader.mVersion);
                MCore::LogDetailedInfo("  + Num data bytes  = %d", nodeHeader.mNumCustomDataBytes);
                MCore::LogDetailedInfo("  + Num attributes  = %d", nodeHeader.mNumAttributes);
                MCore::LogDetailedInfo("  + Num conditions  = %d", transition.mNumConditions);
                MCore::LogDetailedInfo("  + Source node     = %d", transition.mSourceNode);
                MCore::LogDetailedInfo("  + Dest node       = %d", transition.mDestNode);
            }

            // create the transition object
            AnimGraphStateTransition* emfxTransition = nullptr;
            if (GetNewTypeIdByOldNodeTypeId(nodeHeader.mTypeID) == azrtti_typeid<AnimGraphStateTransition>())
            {
                emfxTransition = aznew AnimGraphStateTransition();
            }

            if (emfxTransition)
            {
                if (transition.mDestNode >= blendNodes.GetLength())
                {
                    if (GetLogging())
                    {
                        AZ_Error("EMotionFX", false, "State machine transition refers to invalid destination blend node, transition index %d, blend node: %d", i, transition.mDestNode);
                    }
                    delete emfxTransition;
                    emfxTransition = nullptr;
                }
                // A source node index of MCORE_INVALIDINDEX32 indicates that the transition is a wildcard transition. Don't go into error state in this case.
                else if (transition.mSourceNode != MCORE_INVALIDINDEX32 && transition.mSourceNode >= blendNodes.GetLength())
                {
                    if (GetLogging())
                    {
                        AZ_Error("EMotionFX", false, "State machine transition refers to invalid source blend node, transition index %d, blend node: %d", i, transition.mSourceNode);
                    }
                    delete emfxTransition;
                    emfxTransition = nullptr;
                }
                else
                {
                    AnimGraphNode* targetNode = blendNodes[transition.mDestNode];
                    if (targetNode == nullptr)
                    {
                        delete emfxTransition;
                        emfxTransition = nullptr;
                    }
                    else
                    {
                        AZ_Assert(azrtti_istypeof<AnimGraphStateTransition>(emfxTransition), "ChunkProcessorAnimGraphStateTransitions::Process : Unexpected node type expected AnimGraphStateTransition. Found %u instead", azrtti_typeid(blendNodes[stateMachineIndex]));

                        // Now apply the transition settings
                        // Check if we are dealing with a wildcard transition
                        if (transition.mSourceNode == MCORE_INVALIDINDEX32)
                        {
                            emfxTransition->SetSourceNode(nullptr);
                            emfxTransition->SetIsWildcardTransition(true);
                        }
                        else
                        {
                            // set the source node
                            emfxTransition->SetSourceNode(blendNodes[transition.mSourceNode]);
                        }

                        // set the destination node
                        emfxTransition->SetTargetNode(targetNode);

                        emfxTransition->SetVisualOffsets(transition.mStartOffsetX, transition.mStartOffsetY, transition.mEndOffsetX, transition.mEndOffsetY);

                        // now read the attributes
                        if (!LegacyAnimGraphNodeParser::ParseLegacyAttributes<AnimGraphStateTransition>(file, nodeHeader.mNumAttributes, importParams.mEndianType, importParams, *emfxTransition))
                        {
                            delete emfxTransition;
                            emfxTransition = nullptr;
                            AZ_Error("EMotionFX", false, "Unable to parse state transition");
                            return false;
                        }
                        // add the transition to the state machine
                        stateMachine->AddTransition(emfxTransition);
                    }
                }
            }

            if (emfxTransition)
            {
                // iterate through all conditions
                for (uint32 c = 0; c < transition.mNumConditions; ++c)
                {
                    // read the condition node header
                    FileFormat::AnimGraph_NodeHeader conditionHeader;
                    file->Read(&conditionHeader, sizeof(FileFormat::AnimGraph_NodeHeader));

                    // convert endian
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mTypeID,              importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mVersion,             importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mNumCustomDataBytes,  importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mNumAttributes,       importParams.mEndianType);

                    if (GetLogging())
                    {
                        MCore::LogDetailedInfo("   - Transition Condition:");
                        MCore::LogDetailedInfo("     + Type            = %d", conditionHeader.mTypeID);
                        MCore::LogDetailedInfo("     + Version         = %d", conditionHeader.mVersion);
                        MCore::LogDetailedInfo("     + Num data bytes  = %d", conditionHeader.mNumCustomDataBytes);
                        MCore::LogDetailedInfo("     + Num attributes  = %d", conditionHeader.mNumAttributes);
                    }

                    AnimGraphTransitionCondition* emfxCondition = nullptr;
                    if (!LegacyAnimGraphNodeParser::ParseTransitionConditionChunk(file, importParams, conditionHeader, emfxCondition))
                    {
                        AZ_Error("EMotionFX", false, "Unable to parse Transition condition of type %u in legacy file", azrtti_typeid(emfxCondition));
                        delete emfxCondition;
                        emfxCondition = nullptr;
                        return false;
                    }
                    // add the condition to the transition
                    emfxTransition->AddCondition(emfxCondition);
                }

                //emfxTransition->Init( animGraph );
            }
            // something went wrong with creating the transition
            else
            {
                MCore::LogWarning("Cannot load and instantiate state transition. State transition from %d to %d will be skipped.", transition.mSourceNode, transition.mDestNode);

                // skip reading the attributes
                if (!ForwardAttributes(file, importParams.mEndianType, nodeHeader.mNumAttributes))
                {
                    return false;
                }

                // skip reading the node custom data
                if (file->Forward(nodeHeader.mNumCustomDataBytes) == false)
                {
                    return false;
                }

                // iterate through all conditions and skip them as well
                for (uint32 c = 0; c < transition.mNumConditions; ++c)
                {
                    // read the condition node header
                    FileFormat::AnimGraph_NodeHeader conditionHeader;
                    file->Read(&conditionHeader, sizeof(FileFormat::AnimGraph_NodeHeader));

                    // convert endian
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mTypeID,              importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mVersion,             importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mNumCustomDataBytes,  importParams.mEndianType);
                    MCore::Endian::ConvertUnsignedInt32(&conditionHeader.mNumAttributes,       importParams.mEndianType);

                    // skip reading the attributes
                    if (!ForwardAttributes(file, importParams.mEndianType, conditionHeader.mNumAttributes))
                    {
                        return false;
                    }

                    // skip reading the node custom data
                    if (file->Forward(conditionHeader.mNumCustomDataBytes) == false)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // animGraph state transitions
    bool ChunkProcessorAnimGraphAdditionalInfo::Process(MCore::File* file, Importer::ImportParameters& /*importParams*/)
    {
        return file->Forward(sizeof(FileFormat::AnimGraph_AdditionalInfo));
    }

    //----------------------------------------------------------------------------------------------------------

    // animGraph node connections
    bool ChunkProcessorAnimGraphNodeConnections::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        // read the number of transitions to follow
        uint32 numConnections;
        file->Read(&numConnections, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numConnections, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num node connections = %d", numConnections);
        }

        // get the array of currently loaded nodes
        MCore::Array<AnimGraphNode*>& blendNodes = SharedHelperData::GetBlendNodes(importParams.mSharedData);

        // read the connections
        FileFormat::AnimGraph_NodeConnection connection;
        for (uint32 i = 0; i < numConnections; ++i)
        {
            // read the transition
            file->Read(&connection, sizeof(FileFormat::AnimGraph_NodeConnection));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&connection.mSourceNode,       importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&connection.mTargetNode,       importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt16(&connection.mSourceNodePort,   importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt16(&connection.mTargetNodePort,   importParams.mEndianType);

            // log details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + Connection #%d = From node %d (port id %d) into node %d (port id %d)", i, connection.mSourceNode, connection.mSourceNodePort, connection.mTargetNode, connection.mTargetNodePort);
            }

            // get the source and the target node and check if they are valid
            AnimGraphNode* sourceNode = blendNodes[connection.mSourceNode];
            AnimGraphNode* targetNode = blendNodes[connection.mTargetNode];
            if (sourceNode == nullptr || targetNode == nullptr)
            {
                MCore::LogWarning("EMotionFX::ChunkProcessorAnimGraphNodeConnections() - Connection cannot be created because the source or target node is invalid! (sourcePortID=%d targetPortID=%d sourceNode=%d targetNode=%d)", connection.mSourceNodePort, connection.mTargetNodePort, connection.mSourceNode, connection.mTargetNode);
                continue;
            }

            // create the connection
            const uint32 sourcePort = blendNodes[connection.mSourceNode]->FindOutputPortByID(connection.mSourceNodePort);
            const uint32 targetPort = blendNodes[connection.mTargetNode]->FindInputPortByID(connection.mTargetNodePort);
            if (sourcePort != MCORE_INVALIDINDEX32 && targetPort != MCORE_INVALIDINDEX32)
            {
                blendNodes[connection.mTargetNode]->AddConnection(blendNodes[connection.mSourceNode], static_cast<uint16>(sourcePort), static_cast<uint16>(targetPort));
            }
            else
            {
                MCore::LogWarning("EMotionFX::ChunkProcessorAnimGraphNodeConnections() - Connection cannot be created because the source or target port doesn't exist! (sourcePortID=%d targetPortID=%d sourceNode='%s' targetNode=%s')", connection.mSourceNodePort, connection.mTargetNodePort, blendNodes[connection.mSourceNode]->GetName(), blendNodes[connection.mTargetNode]->GetName());
            }
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // animGraph node
    bool ChunkProcessorAnimGraphNode::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // read the node header
        FileFormat::AnimGraph_NodeHeader nodeHeader;
        file->Read(&nodeHeader, sizeof(FileFormat::AnimGraph_NodeHeader));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mTypeID,               importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mParentIndex,          importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mVersion,              importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumCustomDataBytes,   importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumChildNodes,        importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mNumAttributes,        importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&nodeHeader.mVisualizeColor,       importParams.mEndianType);
        MCore::Endian::ConvertSignedInt32(&nodeHeader.mVisualPosX,           importParams.mEndianType);
        MCore::Endian::ConvertSignedInt32(&nodeHeader.mVisualPosY,           importParams.mEndianType);

        const char* nodeName = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Blend Node:");
            MCore::LogDetailedInfo("  + Name            = %s", nodeName);
            MCore::LogDetailedInfo("  + Parent index    = %d", nodeHeader.mParentIndex);
            MCore::LogDetailedInfo("  + Type            = %d", nodeHeader.mTypeID);
            MCore::LogDetailedInfo("  + Version         = %d", nodeHeader.mVersion);
            MCore::LogDetailedInfo("  + Num data bytes  = %d", nodeHeader.mNumCustomDataBytes);
            MCore::LogDetailedInfo("  + Num child nodes = %d", nodeHeader.mNumChildNodes);
            MCore::LogDetailedInfo("  + Num attributes  = %d", nodeHeader.mNumAttributes);
            MCore::LogDetailedInfo("  + Visualize Color = %d, %d, %d", MCore::ExtractRed(nodeHeader.mVisualizeColor), MCore::ExtractGreen(nodeHeader.mVisualizeColor), MCore::ExtractBlue(nodeHeader.mVisualizeColor));
            MCore::LogDetailedInfo("  + Visual pos      = (%d, %d)", nodeHeader.mVisualPosX, nodeHeader.mVisualPosY);
            MCore::LogDetailedInfo("  + Collapsed       = %s", (nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_COLLAPSED) ? "Yes" : "No");
            MCore::LogDetailedInfo("  + Visualized      = %s", (nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_VISUALIZED) ? "Yes" : "No");
            MCore::LogDetailedInfo("  + Disabled        = %s", (nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_DISABLED) ? "Yes" : "No");
            MCore::LogDetailedInfo("  + Virtual FinalOut= %s", (nodeHeader.mFlags & FileFormat::ANIMGRAPH_NODEFLAG_VIRTUALFINALOUTPUT) ? "Yes" : "No");
        }

        AnimGraphNode* node = nullptr;
        if (!LegacyAnimGraphNodeParser::ParseAnimGraphNodeChunk(file
            , importParams
            , nodeName
            , nodeHeader
            , node))
        {
            if (importParams.mAnimGraph->GetRootStateMachine() == node)
            {
                importParams.mAnimGraph->SetRootStateMachine(nullptr);
            }

            if (node)
            {
                AnimGraphNode* parentNode = node->GetParentNode();
                if (parentNode)
                {
                    parentNode->RemoveChildNodeByPointer(node, false);
                }
            }

            delete node;
            node = nullptr;
            return false;
        }

        EMotionFX::GetEventManager().OnCreatedNode(animGraph, node);

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // animGraph parameters
    bool ChunkProcessorAnimGraphParameters::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // read the number of parameters
        uint32 numParams;
        file->Read(&numParams, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numParams, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num parameters = %d", numParams);
        }

        // read all parameters
        for (uint32 p = 0; p < numParams; ++p)
        {
            // read the parameter info header
            FileFormat::AnimGraph_ParameterInfo paramInfo;
            file->Read(&paramInfo, sizeof(FileFormat::AnimGraph_ParameterInfo));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&paramInfo.mNumComboValues,    importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&paramInfo.mInterfaceType,     importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&paramInfo.mAttributeType,     importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt16(&paramInfo.mFlags,             importParams.mEndianType);

            // check the attribute type
            const uint32 attribType = paramInfo.mAttributeType;
            if (attribType == 0)
            {
                MCore::LogError("EMotionFX::ChunkProcessorAnimGraphParameters::Process() - Failed to convert interface type %d to an attribute type.", attribType);
                return false;
            }

            const AZ::TypeId parameterTypeId = EMotionFX::FileFormat::GetParameterTypeIdForInterfaceType(paramInfo.mInterfaceType);
            const AZStd::string name = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            AZStd::unique_ptr<EMotionFX::Parameter> newParam(EMotionFX::ParameterFactory::Create(parameterTypeId));
            AZ_Assert(azrtti_istypeof<EMotionFX::ValueParameter>(newParam.get()), "Expected a value parameter");

            if (!newParam)
            {
                MCore::LogError("EMotionFX::ChunkProcessorAnimGraphParameters::Process() - Failed to create parameter: '%s'.", name.c_str());
                return false;
            }

            // read the strings
            newParam->SetName(name);
            SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType); // We dont use internal name anymore
            newParam->SetDescription(SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType));

            // log the details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Parameter #%d:", p);
                MCore::LogDetailedInfo("  + Name           = %s", newParam->GetName().c_str());
                MCore::LogDetailedInfo("  + Description    = %s", newParam->GetDescription().c_str());
                MCore::LogDetailedInfo("  + type           = %s", newParam->RTTI_GetTypeName());
                MCore::LogDetailedInfo("  + Attribute type = %d", paramInfo.mAttributeType);
                MCore::LogDetailedInfo("  + Has MinMax     = %d", paramInfo.mHasMinMax);
                MCore::LogDetailedInfo("  + Flags          = %d", paramInfo.mFlags);
            }

            MCore::Attribute* attr(MCore::GetAttributeFactory().CreateAttributeByType(attribType));
            EMotionFX::ValueParameter* valueParameter = static_cast<EMotionFX::ValueParameter*>(newParam.get());

            // create the min, max and default value attributes
            if (paramInfo.mHasMinMax == 1)
            {
                // min value
                attr->Read(file, importParams.mEndianType);
                valueParameter->SetMinValueFromAttribute(attr);

                // max value
                attr->Read(file, importParams.mEndianType);
                valueParameter->SetMaxValueFromAttribute(attr);
            }

            // default value
            attr->Read(file, importParams.mEndianType);
            valueParameter->SetDefaultValueFromAttribute(attr);
            attr->Destroy();

            // Parameters were previously stored in "AttributeSettings". The calss supported
            // multiple values, however, the UI did not, so this ended up not being used.
            // Support for multiple values in parameters is possible, however we dont need it now.
            // Leaving this code as reference
            for (uint32 i = 0; i < paramInfo.mNumComboValues; ++i)
            {
                SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            }

            if (!animGraph->AddParameter(newParam.get()))
            {
                MCore::LogError("EMotionFX::ChunkProcessorAnimGraphParameters::Process() - Failed to add parameter: '%s'.", name.c_str());
                return false;
            }
            newParam.release(); // ownership moved to animGraph
        }

        return true;
    }


    // animGraph node groups
    bool ChunkProcessorAnimGraphNodeGroups::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // read the number of node groups
        uint32 numNodeGroups;
        file->Read(&numNodeGroups, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numNodeGroups, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num Node Groups = %d", numNodeGroups);
        }

        // read all node groups
        for (uint32 g = 0; g < numNodeGroups; ++g)
        {
            // read the node group header
            FileFormat::AnimGraph_NodeGroup nodeGroupChunk;
            file->Read(&nodeGroupChunk, sizeof(FileFormat::AnimGraph_NodeGroup));

            MCore::RGBAColor emfxColor(nodeGroupChunk.mColor.mR, nodeGroupChunk.mColor.mG, nodeGroupChunk.mColor.mB, nodeGroupChunk.mColor.mA);

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&nodeGroupChunk.mNumNodes, importParams.mEndianType);
            MCore::Endian::ConvertRGBAColor(&emfxColor,                    importParams.mEndianType);

            const AZ::Color color128 = MCore::EmfxColorToAzColor(emfxColor);
            const AZ::u32 color32 = color128.ToU32();

            const char* groupName   = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            const uint32    numNodes    = nodeGroupChunk.mNumNodes;

            // create and fill the new node group
            AnimGraphNodeGroup* nodeGroup = aznew AnimGraphNodeGroup(groupName);
            animGraph->AddNodeGroup(nodeGroup);
            nodeGroup->SetIsVisible(nodeGroupChunk.mIsVisible != 0);
            nodeGroup->SetColor(color32);

            // set the nodes of the node group
            MCore::Array<AnimGraphNode*>& blendNodes = SharedHelperData::GetBlendNodes(importParams.mSharedData);
            nodeGroup->SetNumNodes(numNodes);
            for (uint32 i = 0; i < numNodes; ++i)
            {
                // read the node index of the current node inside the group
                uint32 nodeNr;
                file->Read(&nodeNr, sizeof(uint32));
                MCore::Endian::ConvertUnsignedInt32(&nodeNr, importParams.mEndianType);

                MCORE_ASSERT(nodeNr != MCORE_INVALIDINDEX32);

                // set the id of the given node to the group
                if (nodeNr != MCORE_INVALIDINDEX32 && blendNodes[nodeNr])
                {
                    nodeGroup->SetNode(i, blendNodes[nodeNr]->GetId());
                }
                else
                {
                    nodeGroup->SetNode(i, AnimGraphNodeId::InvalidId);
                }
            }

            // log the details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Node Group #%d:", g);
                MCore::LogDetailedInfo("  + Name           = %s", nodeGroup->GetName());
                MCore::LogDetailedInfo("  + Color          = (%.2f, %.2f, %.2f, %.2f)", static_cast<float>(color128.GetR()), static_cast<float>(color128.GetG()), static_cast<float>(color128.GetB()), static_cast<float>(color128.GetA()));
                MCore::LogDetailedInfo("  + Num Nodes      = %i", nodeGroup->GetNumNodes());
            }
        }

        return true;
    }


    // animGraph group parameters
    bool ChunkProcessorAnimGraphGroupParameters::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // read the number of group parameters
        uint32 numGroupParameters;
        file->Read(&numGroupParameters, sizeof(uint32));
        MCore::Endian::ConvertUnsignedInt32(&numGroupParameters, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Num group parameters = %d", numGroupParameters);
        }

        // Group parameters is going to re-shuffle the value parameter indices, therefore we
        // need to update the connections downstream of parameter nodes.
        EMotionFX::ValueParameterVector valueParametersBeforeChange = animGraph->RecursivelyGetValueParameters();

        // Since relocating a parameter to another parent changes its index, we are going to
        // compute all the relationships leaving the value parameters at the root, then relocate
        // them.
        AZStd::vector<AZStd::pair<const EMotionFX::GroupParameter*, EMotionFX::ParameterVector>> parametersByGroup;

        // read all group parameters
        for (uint32 g = 0; g < numGroupParameters; ++g)
        {
            // read the group parameter header
            FileFormat::AnimGraph_GroupParameter groupChunk;
            file->Read(&groupChunk, sizeof(FileFormat::AnimGraph_GroupParameter));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&groupChunk.mNumParameters, importParams.mEndianType);

            const char* groupName = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            const uint32 numParameters = groupChunk.mNumParameters;

            // create and fill the new group parameter
            AZStd::unique_ptr<EMotionFX::Parameter> parameter(EMotionFX::ParameterFactory::Create(azrtti_typeid<EMotionFX::GroupParameter>()));
            parameter->SetName(groupName);

            // Previously collapsed/expanded state in group parameters was stored in the animgraph file. However, that
            // would require to check out the animgraph file if you expand/collapse a group. Because this change was not
            // done through commands, the dirty state was not properly restored.
            // Collapsing state should be more of a setting per-user than something saved in the animgraph
            //groupParameter->SetIsCollapsed(groupChunk.mCollapsed != 0);

            if (!animGraph->AddParameter(parameter.get()))
            {
                continue;
            }
            const EMotionFX::GroupParameter* groupParameter = static_cast<EMotionFX::GroupParameter*>(parameter.release());

            parametersByGroup.emplace_back(groupParameter, EMotionFX::ParameterVector());
            AZStd::vector<EMotionFX::Parameter*>& parametersInGroup = parametersByGroup.back().second;
            
            // set the parameters of the group parameter
            for (uint32 i = 0; i < numParameters; ++i)
            {
                // read the parameter index
                uint32 parameterIndex;
                file->Read(&parameterIndex, sizeof(uint32));
                MCore::Endian::ConvertUnsignedInt32(&parameterIndex, importParams.mEndianType);

                MCORE_ASSERT(parameterIndex != MCORE_INVALIDINDEX32);
                if (parameterIndex != MCORE_INVALIDINDEX32)
                {
                    const EMotionFX::Parameter* childParameter = animGraph->FindValueParameter(parameterIndex);
                    parametersInGroup.emplace_back(const_cast<EMotionFX::Parameter*>(childParameter));
                }
            }

            // log the details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Group parameter #%d:", g);
                MCore::LogDetailedInfo("  + Name           = %s", groupParameter->GetName().c_str());
                MCore::LogDetailedInfo("  + Num Parameters = %i", groupParameter->GetNumParameters());
            }
        }

        // Now move the parameters to their groups
        for (const AZStd::pair<const EMotionFX::GroupParameter*, EMotionFX::ParameterVector>& groupAndParameters : parametersByGroup)
        {
            const EMotionFX::GroupParameter* groupParameter = groupAndParameters.first;
            for (EMotionFX::Parameter* parameter : groupAndParameters.second)
            {
                animGraph->TakeParameterFromParent(parameter);
                animGraph->AddParameter(const_cast<EMotionFX::Parameter*>(parameter), groupParameter);
            }
        }

        const EMotionFX::ValueParameterVector valueParametersAfterChange = animGraph->RecursivelyGetValueParameters();

        CommandSystem::RewireConnectionsForParameterNodesAfterParameterIndexChange(animGraph, valueParametersBeforeChange, valueParametersAfterChange);

        return true;
    }


    // animGraph game controller settings
    bool ChunkProcessorAnimGraphGameControllerSettings::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        uint32 i;

        AnimGraph* animGraph = importParams.mAnimGraph;
        MCORE_ASSERT(animGraph);

        // get the game controller settings for the anim graph and clear it
        AnimGraphGameControllerSettings& gameControllerSettings = animGraph->GetGameControllerSettings();
        gameControllerSettings.Clear();

        // read the number of presets and the active preset index
        uint32 activePresetIndex, numPresets;
        file->Read(&activePresetIndex, sizeof(uint32));
        file->Read(&numPresets, sizeof(uint32));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&activePresetIndex, importParams.mEndianType);
        MCore::Endian::ConvertUnsignedInt32(&numPresets, importParams.mEndianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Game Controller Settings (NumPresets=%d, ActivePreset=%d)", numPresets, activePresetIndex);
        }

        // preallocate memory for the presets
        gameControllerSettings.SetNumPresets(numPresets);

        // read all presets
        for (uint32 p = 0; p < numPresets; ++p)
        {
            // read the preset chunk
            FileFormat::AnimGraph_GameControllerPreset presetChunk;
            file->Read(&presetChunk, sizeof(FileFormat::AnimGraph_GameControllerPreset));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&presetChunk.mNumParameterInfos, importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt32(&presetChunk.mNumButtonInfos, importParams.mEndianType);

            // read the preset name and get the number of parameter and button infos
            const char*     presetName      = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);
            const uint32    numParamInfos   = presetChunk.mNumParameterInfos;
            const uint32    numButtonInfos  = presetChunk.mNumButtonInfos;

            // create and fill the new preset
            AnimGraphGameControllerSettings::Preset* preset = aznew AnimGraphGameControllerSettings::Preset(presetName);
            gameControllerSettings.SetPreset(p, preset);

            // read the parameter infos
            preset->SetNumParamInfos(numParamInfos);
            for (i = 0; i < numParamInfos; ++i)
            {
                // read the parameter info chunk
                FileFormat::AnimGraph_GameControllerParameterInfo paramInfoChunk;
                file->Read(&paramInfoChunk, sizeof(FileFormat::AnimGraph_GameControllerParameterInfo));

                // read the parameter name
                const char* parameterName = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);

                // construct and fill the parameter info
                AnimGraphGameControllerSettings::ParameterInfo* parameterInfo = aznew AnimGraphGameControllerSettings::ParameterInfo(parameterName);
                parameterInfo->m_axis    = paramInfoChunk.mAxis;
                parameterInfo->m_invert  = (paramInfoChunk.mInvert != 0);
                parameterInfo->m_mode    = (AnimGraphGameControllerSettings::ParameterMode)paramInfoChunk.mMode;

                preset->SetParamInfo(i, parameterInfo);
            }

            // read the button infos
            preset->SetNumButtonInfos(numButtonInfos);
            for (i = 0; i < numButtonInfos; ++i)
            {
                // read the button info chunk
                FileFormat::AnimGraph_GameControllerButtonInfo buttonInfoChunk;
                file->Read(&buttonInfoChunk, sizeof(FileFormat::AnimGraph_GameControllerButtonInfo));

                // read the button string
                const char* buttonString = SharedHelperData::ReadString(file, importParams.mSharedData, importParams.mEndianType);

                // construct and fill the button info
                AnimGraphGameControllerSettings::ButtonInfo* buttonInfo = aznew AnimGraphGameControllerSettings::ButtonInfo(buttonInfoChunk.mButtonIndex);
                buttonInfo->m_mode   = (AnimGraphGameControllerSettings::ButtonMode)buttonInfoChunk.mMode;
                buttonInfo->m_string = buttonString;

                preset->SetButtonInfo(i, buttonInfo);
            }

            // log the details
            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Preset '%s':", preset->GetName());
                MCore::LogDetailedInfo("  + Num Param Infos  = %d", preset->GetNumParamInfos());
                MCore::LogDetailedInfo("  + Num Button Infos = %d", preset->GetNumButtonInfos());
            }
        }

        // set the active preset
        if (activePresetIndex != MCORE_INVALIDINDEX32)
        {
            AnimGraphGameControllerSettings::Preset* activePreset = gameControllerSettings.GetPreset(activePresetIndex);
            gameControllerSettings.SetActivePreset(activePreset);
        }

        return true;
    }

    //----------------------------------------------------------------------------------------------------------

    // the motion wavelet main info
    bool ChunkProcessorMotionWaveletInfo::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;

        // typecast the motion object
        MCORE_ASSERT(importParams.mMotion->GetType() == WaveletSkeletalMotion::TYPE_ID);
        WaveletSkeletalMotion* waveletMotion = static_cast<WaveletSkeletalMotion*>(importParams.mMotion);

        // read the struct
        FileFormat::Motion_WaveletInfo waveletInfo;
        file->Read(&waveletInfo, sizeof(FileFormat::Motion_WaveletInfo));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mNumChunks,                   endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mSamplesPerChunk,             endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mDecompressedRotNumBytes,     endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mDecompressedPosNumBytes,     endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mDecompressedScaleNumBytes,   endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mDecompressedMorphNumBytes,   endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mNumRotTracks,                endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mNumScaleTracks,              endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mNumPosTracks,                endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mNumMorphTracks,              endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mChunkOverhead,               endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mCompressedSize,              endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mOptimizedSize,               endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mUncompressedSize,            endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mNumSubMotions,               endianType);
        MCore::Endian::ConvertUnsignedInt32(&waveletInfo.mNumMorphSubMotions,          endianType);
        MCore::Endian::ConvertFloat(&waveletInfo.mSampleSpacing,       endianType);
        MCore::Endian::ConvertFloat(&waveletInfo.mSecondsPerChunk,     endianType);
        MCore::Endian::ConvertFloat(&waveletInfo.mMaxTime,             endianType);
        MCore::Endian::ConvertFloat(&waveletInfo.mPosQuantFactor,      endianType);
        MCore::Endian::ConvertFloat(&waveletInfo.mRotQuantFactor,      endianType);
        MCore::Endian::ConvertFloat(&waveletInfo.mScaleQuantFactor,    endianType);
        MCore::Endian::ConvertFloat(&waveletInfo.mMorphQuantFactor,    endianType);
        MCore::Endian::ConvertFloat(&waveletInfo.mScale,               endianType);

        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Wavelet Motion:");
            MCore::LogDetailedInfo("  + Number of chunks     = %d", waveletInfo.mNumChunks);
            MCore::LogDetailedInfo("  + Samples per chunk    = %d", waveletInfo.mSamplesPerChunk);
            MCore::LogDetailedInfo("  + Decompressed rot     = %d bytes", waveletInfo.mDecompressedRotNumBytes);
            MCore::LogDetailedInfo("  + Decompressed pos     = %d bytes", waveletInfo.mDecompressedPosNumBytes);
            MCore::LogDetailedInfo("  + Decompressed scale   = %d bytes", waveletInfo.mDecompressedScaleNumBytes);
            MCore::LogDetailedInfo("  + Decompressed morph   = %d bytes", waveletInfo.mDecompressedMorphNumBytes);
            MCore::LogDetailedInfo("  + Num pos tracks       = %d", waveletInfo.mNumPosTracks);
            MCore::LogDetailedInfo("  + Num rot tracks       = %d", waveletInfo.mNumRotTracks);
            MCore::LogDetailedInfo("  + Num scale tracks     = %d", waveletInfo.mNumScaleTracks);
            MCore::LogDetailedInfo("  + Num morph tracks     = %d", waveletInfo.mNumMorphTracks);
            MCore::LogDetailedInfo("  + Chunk overhead       = %d", waveletInfo.mChunkOverhead);
            MCore::LogDetailedInfo("  + Compressed size      = %d bytes", waveletInfo.mCompressedSize);
            MCore::LogDetailedInfo("  + Optimized size       = %d bytes", waveletInfo.mOptimizedSize);
            MCore::LogDetailedInfo("  + Uncompressed size    = %d bytes", waveletInfo.mUncompressedSize);
            MCore::LogDetailedInfo("  + Max time             = %f", waveletInfo.mMaxTime);
            MCore::LogDetailedInfo("  + Num submotions       = %d", waveletInfo.mNumSubMotions);
            MCore::LogDetailedInfo("  + Num morph submotions = %d", waveletInfo.mNumMorphSubMotions);
            MCore::LogDetailedInfo("  + Sample spacing       = %f", waveletInfo.mSampleSpacing);
            MCore::LogDetailedInfo("  + Seconds per chunk    = %f", waveletInfo.mSecondsPerChunk);
            MCore::LogDetailedInfo("  + Pos quant factor     = %f", waveletInfo.mPosQuantFactor);
            MCore::LogDetailedInfo("  + Rot quant factor     = %f", waveletInfo.mRotQuantFactor);
            MCore::LogDetailedInfo("  + Scale quant factor   = %f", waveletInfo.mScaleQuantFactor);
            MCore::LogDetailedInfo("  + Morph quant factor   = %f", waveletInfo.mMorphQuantFactor);
            MCore::LogDetailedInfo("  + Wavelet ID           = %d", waveletInfo.mWaveletID);
            MCore::LogDetailedInfo("  + Compressor ID        = %d", waveletInfo.mCompressorID);
            MCore::LogDetailedInfo("  + Unit Type            = %d", waveletInfo.mUnitType);
            MCore::LogDetailedInfo("  + Scale                = %f", waveletInfo.mScale);
        }

        // apply the settings
        waveletMotion->SetNumChunks(waveletInfo.mNumChunks);
        waveletMotion->SetSamplesPerChunk(waveletInfo.mSamplesPerChunk);
        waveletMotion->SetDecompressedRotNumBytes(waveletInfo.mDecompressedRotNumBytes);
        waveletMotion->SetDecompressedPosNumBytes(waveletInfo.mDecompressedPosNumBytes);
        waveletMotion->SetDecompressedMorphNumBytes(waveletInfo.mDecompressedMorphNumBytes);
        waveletMotion->SetNumPosTracks(waveletInfo.mNumPosTracks);
        waveletMotion->SetNumRotTracks(waveletInfo.mNumRotTracks);
        waveletMotion->SetNumMorphTracks(waveletInfo.mNumMorphTracks);
        waveletMotion->SetChunkOverhead(waveletInfo.mChunkOverhead);
        waveletMotion->SetCompressedSize(waveletInfo.mCompressedSize);
        waveletMotion->SetUncompressedSize(waveletInfo.mUncompressedSize);
        waveletMotion->SetOptimizedSize(waveletInfo.mOptimizedSize);
        //waveletMotion->SetScaleRotOffset( waveletInfo.mScaleRotOffset );
        waveletMotion->SetSampleSpacing (waveletInfo.mSampleSpacing);
        waveletMotion->SetSecondsPerChunk(waveletInfo.mSecondsPerChunk);
        waveletMotion->SetMaxTime(waveletInfo.mMaxTime);
        waveletMotion->SetMotionFPS(waveletInfo.mSamplesPerChunk / waveletInfo.mSecondsPerChunk);

        waveletMotion->SetPosQuantFactor(waveletInfo.mPosQuantFactor);
        waveletMotion->SetRotQuantFactor(waveletInfo.mRotQuantFactor);
        waveletMotion->SetMorphQuantFactor(waveletInfo.mMorphQuantFactor);
        waveletMotion->SetUnitType(static_cast<MCore::Distance::EUnitType>(waveletInfo.mUnitType));

        EMFX_SCALECODE
        (
            waveletMotion->SetDecompressedScaleNumBytes(waveletInfo.mDecompressedScaleNumBytes);
            waveletMotion->SetNumScaleTracks(waveletInfo.mNumScaleTracks);
            waveletMotion->SetScaleQuantFactor(waveletInfo.mScaleQuantFactor);
        )

        // we currently only support huffman
        MCORE_ASSERT(waveletInfo.mCompressorID == FileFormat::MOTION_COMPRESSOR_HUFFMAN);

        // set the right wavelet to use
        switch (waveletInfo.mWaveletID)
        {
        case FileFormat::MOTION_WAVELET_HAAR:
            waveletMotion->SetWavelet(WaveletSkeletalMotion::WAVELET_HAAR);
            break;
        case FileFormat::MOTION_WAVELET_D4:
            waveletMotion->SetWavelet(WaveletSkeletalMotion::WAVELET_DAUB4);
            break;
        case FileFormat::MOTION_WAVELET_CDF97:
            waveletMotion->SetWavelet(WaveletSkeletalMotion::WAVELET_CDF97);
            break;
        default:
            MCORE_ASSERT(1 == 0);
        }
        ;

        // read the mapping values
        waveletMotion->ResizeMappingArray(waveletInfo.mNumSubMotions);
        WaveletSkeletalMotion::Mapping emfxMap;
        uint32 i;
        FileFormat::Motion_WaveletMapping mapping;
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Wavelet Submotion Map:");
        }
        for (i = 0; i < waveletInfo.mNumSubMotions; ++i)
        {
            // read from the file
            file->Read(&mapping, sizeof(FileFormat::Motion_WaveletMapping));

            // convert endian
            MCore::Endian::ConvertUnsignedInt16(&mapping.mPosIndex,        importParams.mEndianType);
            MCore::Endian::ConvertUnsignedInt16(&mapping.mRotIndex,        importParams.mEndianType);

            EMFX_SCALECODE
            (
                MCore::Endian::ConvertUnsignedInt16(&mapping.mScaleIndex,      importParams.mEndianType);
            )

            // update the EMotion FX map
            emfxMap.mPosIndex       = mapping.mPosIndex;
            emfxMap.mRotIndex       = mapping.mRotIndex;

            EMFX_SCALECODE
            (
                emfxMap.mScaleIndex     = mapping.mScaleIndex;
            )

            waveletMotion->SetSubMotionMapping(i, emfxMap);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("    + SubMotion #%d = (posIndex:%d, rotIndex:%d, scaleIndex:%d", i, mapping.mPosIndex, mapping.mRotIndex, mapping.mScaleIndex);
            }
        }

        // read the submotions
        waveletMotion->SetNumSubMotions(waveletInfo.mNumSubMotions);
        FileFormat::Motion_WaveletSkeletalSubMotion fileSubMotion;
        for (i = 0; i < waveletInfo.mNumSubMotions; ++i)
        {
            // read it from the file
            file->Read(&fileSubMotion, sizeof(FileFormat::Motion_WaveletSkeletalSubMotion));

            // read the name
            const char* subMotionName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            // convert endian and coordinate system
            AZ::PackedVector3f posePos(fileSubMotion.mPosePos.mX, fileSubMotion.mPosePos.mY, fileSubMotion.mPosePos.mZ);
            AZ::PackedVector3f poseScale(fileSubMotion.mPoseScale.mX, fileSubMotion.mPoseScale.mY, fileSubMotion.mPoseScale.mZ);
            AZ::PackedVector3f bindPosePos(fileSubMotion.mBindPosePos.mX, fileSubMotion.mBindPosePos.mY, fileSubMotion.mBindPosePos.mZ);
            AZ::PackedVector3f bindPoseScale(fileSubMotion.mBindPoseScale.mX, fileSubMotion.mBindPoseScale.mY, fileSubMotion.mBindPoseScale.mZ);

            ConvertVector3(&posePos,           endianType);
            ConvertVector3(&bindPosePos,       endianType);
            ConvertScale  (&poseScale,         endianType);
            ConvertScale  (&bindPoseScale,     endianType);

            // convert endian and coordinate system
            MCore::Compressed16BitQuaternion poseRot(fileSubMotion.mPoseRot.mX, fileSubMotion.mPoseRot.mY, fileSubMotion.mPoseRot.mZ, fileSubMotion.mPoseRot.mW);
            MCore::Compressed16BitQuaternion bindPoseRot(fileSubMotion.mBindPoseRot.mX, fileSubMotion.mBindPoseRot.mY, fileSubMotion.mBindPoseRot.mZ, fileSubMotion.mBindPoseRot.mW);

            Convert16BitQuaternion(&poseRot, endianType);
            Convert16BitQuaternion(&bindPoseRot, endianType);

            if (GetLogging())
            {
                MCore::Quaternion uncompressedPoseRot           = poseRot.ToQuaternion();
                MCore::Quaternion uncompressedBindPoseRot       = bindPoseRot.ToQuaternion();

                MCore::LogDetailedInfo("- Wavelet Skeletal SubMotion = '%s'", subMotionName);
                MCore::LogDetailedInfo("    + Pose Position:         x=%f, y=%f, z=%f",
                    static_cast<float>(posePos.GetX()),
                    static_cast<float>(posePos.GetY()),
                    static_cast<float>(posePos.GetZ()));
                MCore::LogDetailedInfo("    + Pose Rotation:         x=%f, y=%f, z=%f, w=%f", uncompressedPoseRot.x, uncompressedPoseRot.y, uncompressedPoseRot.z, uncompressedPoseRot.w);
                MCore::LogDetailedInfo("    + Pose Scale:            x=%f, y=%f, z=%f",
                    static_cast<float>(poseScale.GetX()),
                    static_cast<float>(poseScale.GetY()),
                    static_cast<float>(poseScale.GetZ()));
                MCore::LogDetailedInfo("    + Bind Pose Position:    x=%f, y=%f, z=%f",
                    static_cast<float>(bindPosePos.GetX()),
                    static_cast<float>(bindPosePos.GetY()),
                    static_cast<float>(bindPosePos.GetZ()));
                MCore::LogDetailedInfo("    + Bind Pose Rotation:    x=%f, y=%f, z=%f, w=%f", uncompressedBindPoseRot.x, uncompressedBindPoseRot.y, uncompressedBindPoseRot.z, uncompressedBindPoseRot.w);
                MCore::LogDetailedInfo("    + Bind Pose Scale:       x=%f, y=%f, z=%f",
                    static_cast<float>(bindPoseScale.GetX()),
                    static_cast<float>(bindPoseScale.GetY()),
                    static_cast<float>(bindPoseScale.GetZ()));
            }

            // create the part, and add it to the motion
            SkeletalSubMotion* subMotion = SkeletalSubMotion::Create(subMotionName);

            subMotion->SetPosePos                       (AZ::Vector3(posePos));
            subMotion->SetCompressedPoseRot             (poseRot);
            subMotion->SetBindPosePos                   (AZ::Vector3(bindPosePos));
            subMotion->SetCompressedBindPoseRot         (bindPoseRot);

            EMFX_SCALECODE
            (
                subMotion->SetPoseScale                 (AZ::Vector3(poseScale));
                subMotion->SetBindPoseScale             (AZ::Vector3(bindPoseScale));
            )

            waveletMotion->SetSubMotion(i, subMotion);
        }


        // read the morph submotions
        waveletMotion->SetNumMorphSubMotions(waveletInfo.mNumMorphSubMotions);
        FileFormat::Motion_WaveletMorphSubMotion fileMorphSubMotion;
        for (i = 0; i < waveletInfo.mNumMorphSubMotions; ++i)
        {
            // read it from the file
            file->Read(&fileMorphSubMotion, sizeof(FileFormat::Motion_WaveletMorphSubMotion));

            // convert endian
            MCore::Endian::ConvertFloat(&fileMorphSubMotion.mPoseWeight,   endianType);

            // read the name
            const char* subMotionName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Wavelet Morph SubMotion = '%s'", subMotionName);
                MCore::LogDetailedInfo("    + Pose Weight:  %f", fileMorphSubMotion.mPoseWeight);
            }

            // create the morph submotion, and add it to the motion
            MorphSubMotion* subMotion = MorphSubMotion::Create(MCore::GetStringIdPool().GenerateIdForString(subMotionName));
            subMotion->SetPoseWeight(fileMorphSubMotion.mPoseWeight);

            waveletMotion->SetMorphSubMotion(i, subMotion);
        }


        // read the morph submotion mapping array
        waveletMotion->ResizeMorphMappingArray(waveletInfo.mNumMorphSubMotions);
        for (i = 0; i < waveletInfo.mNumMorphSubMotions; ++i)
        {
            uint16 value;
            file->Read(&value, sizeof(uint16));
            MCore::Endian::ConvertUnsignedInt16(&value, endianType);
            waveletMotion->SetMorphSubMotionMapping(i, value);
        }

        // read the chunks
        FileFormat::Motion_WaveletChunk fileChunk;
        for (i = 0; i < waveletInfo.mNumChunks; ++i)
        {
            // read it from the file
            file->Read(&fileChunk, sizeof(FileFormat::Motion_WaveletChunk));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&fileChunk.mCompressedRotNumBytes,     endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileChunk.mCompressedPosNumBytes,     endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileChunk.mCompressedScaleNumBytes,   endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileChunk.mCompressedMorphNumBytes,   endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileChunk.mCompressedPosNumBits,      endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileChunk.mCompressedRotNumBits,      endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileChunk.mCompressedScaleNumBits,    endianType);
            MCore::Endian::ConvertUnsignedInt32(&fileChunk.mCompressedMorphNumBits,    endianType);
            MCore::Endian::ConvertFloat(&fileChunk.mRotQuantScale,     endianType);
            MCore::Endian::ConvertFloat(&fileChunk.mPosQuantScale,     endianType);
            MCore::Endian::ConvertFloat(&fileChunk.mScaleQuantScale,   endianType);
            MCore::Endian::ConvertFloat(&fileChunk.mMorphQuantScale,   endianType);
            MCore::Endian::ConvertFloat(&fileChunk.mStartTime, endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("- Wavelet Chunk");
                MCore::LogDetailedInfo("    + RotQuantScale:           %f", fileChunk.mRotQuantScale);
                MCore::LogDetailedInfo("    + PosQuantScale:           %f", fileChunk.mPosQuantScale);
                MCore::LogDetailedInfo("    + ScaleQuantScale:         %f", fileChunk.mScaleQuantScale);
                MCore::LogDetailedInfo("    + MorphQuantScale:         %f", fileChunk.mMorphQuantScale);
                MCore::LogDetailedInfo("    + StartTime:               %f", fileChunk.mStartTime);
                MCore::LogDetailedInfo("    + CompressedRotNumBytes:   %i", fileChunk.mCompressedRotNumBytes);
                MCore::LogDetailedInfo("    + CompressedPosNumBytes:   %i", fileChunk.mCompressedPosNumBytes);
                MCore::LogDetailedInfo("    + CompressedScaleNumBytes: %i", fileChunk.mCompressedScaleNumBytes);
                MCore::LogDetailedInfo("    + CompressedMorphNumBytes: %i", fileChunk.mCompressedMorphNumBytes);
                MCore::LogDetailedInfo("    + CompressedPosNumBits:    %i", fileChunk.mCompressedPosNumBits);
                MCore::LogDetailedInfo("    + CompressedRotNumBits:    %i", fileChunk.mCompressedRotNumBits);
                MCore::LogDetailedInfo("    + CompressedScaleNumBits:  %i", fileChunk.mCompressedScaleNumBits);
                MCore::LogDetailedInfo("    + CompressedMorphNumBits:  %i", fileChunk.mCompressedMorphNumBits);
            }

            // create the new chunk
            WaveletSkeletalMotion::Chunk* newChunk = WaveletSkeletalMotion::Chunk::Create();
            waveletMotion->SetChunk(i, newChunk);

            // setup the chunk properties
            newChunk->mCompressedPosNumBytes                    = fileChunk.mCompressedPosNumBytes;
            newChunk->mCompressedRotNumBytes                    = fileChunk.mCompressedRotNumBytes;
            newChunk->mCompressedMorphNumBytes                  = fileChunk.mCompressedMorphNumBytes;
            EMFX_SCALECODE(newChunk->mCompressedScaleNumBytes  = fileChunk.mCompressedScaleNumBytes;
                )
            newChunk->mRotQuantScale                            = fileChunk.mRotQuantScale;
            newChunk->mPosQuantScale                            = fileChunk.mPosQuantScale;
            newChunk->mMorphQuantScale                          = fileChunk.mMorphQuantScale;
            EMFX_SCALECODE(newChunk->mScaleQuantScale          = fileChunk.mScaleQuantScale;
                )
            newChunk->mStartTime                                = fileChunk.mStartTime;
            newChunk->mCompressedPosNumBits                     = fileChunk.mCompressedPosNumBits;
            newChunk->mCompressedRotNumBits                     = fileChunk.mCompressedRotNumBits;
            newChunk->mCompressedMorphNumBits                   = fileChunk.mCompressedMorphNumBits;
            EMFX_SCALECODE(newChunk->mCompressedScaleNumBits   = fileChunk.mCompressedScaleNumBits;
                )

            // read the rotation data
            if (fileChunk.mCompressedRotNumBytes > 0)
            {
                newChunk->mCompressedRotData = (uint8*)MCore::Allocate(fileChunk.mCompressedRotNumBytes, EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS);
                file->Read(newChunk->mCompressedRotData, fileChunk.mCompressedRotNumBytes);
            }

            // read the position data
            if (fileChunk.mCompressedPosNumBytes > 0)
            {
                newChunk->mCompressedPosData = (uint8*)MCore::Allocate(fileChunk.mCompressedPosNumBytes, EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS);
                file->Read(newChunk->mCompressedPosData, fileChunk.mCompressedPosNumBytes);
            }

            // read the morph data
            if (fileChunk.mCompressedMorphNumBytes > 0)
            {
                newChunk->mCompressedMorphData = (uint8*)MCore::Allocate(fileChunk.mCompressedMorphNumBytes, EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS);
                file->Read(newChunk->mCompressedMorphData, fileChunk.mCompressedMorphNumBytes);
            }

            // read the scale data
            if (fileChunk.mCompressedScaleNumBytes > 0)
            {
#ifndef EMFX_SCALE_DISABLED
                newChunk->mCompressedScaleData = (uint8*)MCore::Allocate(fileChunk.mCompressedScaleNumBytes, EMFX_MEMCATEGORY_MOTIONS_WAVELETSKELETALMOTIONS);
                file->Read(newChunk->mCompressedScaleData, fileChunk.mCompressedScaleNumBytes);
#else
                file->Forward(fileChunk.mCompressedScaleNumBytes);
#endif
            }
        }

        //MCore::LogInfo("endian = %d", importParams.mEndianType);

        // swap the endian of the chunk data if needed
    #ifdef MCORE_LITTLE_ENDIAN
        if (importParams.mEndianType == MCore::Endian::ENDIAN_BIG)
        {
            waveletMotion->SwapChunkDataEndian();
        }
    #else
        if (importParams.mEndianType == MCore::Endian::ENDIAN_LITTLE)
        {
            waveletMotion->SwapChunkDataEndian();
        }
    #endif

        waveletMotion->Scale(waveletInfo.mScale);

        return true;
    }



    //----------------------------------------------------------------------------------------------------------
    // MotionSet
    //----------------------------------------------------------------------------------------------------------

    // all submotions in one chunk
    bool ChunkProcessorMotionSet::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;

        FileFormat::MotionSetsChunk motionSetsChunk;
        file->Read(&motionSetsChunk, sizeof(FileFormat::MotionSetsChunk));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&motionSetsChunk.mNumSets, endianType);

        // get the number of motion sets and iterate through them
        const uint32 numMotionSets = motionSetsChunk.mNumSets;
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            FileFormat::MotionSetChunk motionSetChunk;
            file->Read(&motionSetChunk, sizeof(FileFormat::MotionSetChunk));

            // convert endian
            MCore::Endian::ConvertUnsignedInt32(&motionSetChunk.mNumChildSets, endianType);
            MCore::Endian::ConvertUnsignedInt32(&motionSetChunk.mNumMotionEntries, endianType);

            // get the parent set
            const char* parentSetName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
            GetMotionManager().Lock();
            MotionSet* parentSet = GetMotionManager().FindMotionSetByName(parentSetName, importParams.m_isOwnedByRuntime);
            GetMotionManager().Unlock();

            // read the motion set name and create our new motion set
            const char* motionSetName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
            MotionSet* motionSet = aznew MotionSet(motionSetName, parentSet);
            motionSet->SetIsOwnedByRuntime(importParams.m_isOwnedByRuntime);

            // set the root motion set to the importer params motion set, this will be returned by the Importer::LoadMotionSet() function
            if (parentSet == nullptr)
            {
                assert(importParams.mMotionSet == nullptr);
                importParams.mMotionSet = motionSet;
            }

            // read the filename and set it
            /*const char* motionSetFileName = */ SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
            //motionSet->SetFileName( motionSetFileName );

            // in case this is not a root motion set add the new motion set as child set to the parent set
            if (parentSet)
            {
                parentSet->AddChildSet(motionSet);
            }

            // Read all motion entries.
            const uint32 numMotionEntries = motionSetChunk.mNumMotionEntries;
            motionSet->ReserveMotionEntries(numMotionEntries);
            AZStd::string nativeMotionFileName;
            for (uint32 j = 0; j < numMotionEntries; ++j)
            {
                // read the motion entry
                const char* motionFileName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
                nativeMotionFileName = motionFileName;

                // read the string id and set it
                const char* motionStringID = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

                // add the motion entry to the motion set
                MotionSet::MotionEntry* motionEntry = aznew MotionSet::MotionEntry(nativeMotionFileName.c_str(), motionStringID);
                motionSet->AddMotionEntry(motionEntry);
            }
        }

        return true;
    }



    //----------------------------------------------------------------------------------------------------------
    // NodeMap
    //----------------------------------------------------------------------------------------------------------

    // node map
    bool ChunkProcessorNodeMap::Process(MCore::File* file, Importer::ImportParameters& importParams)
    {
        const MCore::Endian::EEndianType endianType = importParams.mEndianType;

        // read the header
        FileFormat::NodeMapChunk nodeMapChunk;
        file->Read(&nodeMapChunk, sizeof(FileFormat::NodeMapChunk));

        // convert endian
        MCore::Endian::ConvertUnsignedInt32(&nodeMapChunk.mNumEntries, endianType);

        // load the source actor filename string
        AZStd::string sourceActorFileName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
        importParams.mNodeMap->SetSourceActorFileName(sourceActorFileName.c_str());

        // log some info
        if (GetLogging())
        {
            MCore::LogDetailedInfo("- Node Map:");
            MCore::LogDetailedInfo("  + Num entries = %d", nodeMapChunk.mNumEntries);
            MCore::LogDetailedInfo("  + Source actor filename = '%s'", sourceActorFileName.c_str());
        }

        // for all entries
        const uint32 numEntries = nodeMapChunk.mNumEntries;
        importParams.mNodeMap->Reserve(numEntries);
        AZStd::string firstName;
        AZStd::string secondName;
        for (uint32 i = 0; i < numEntries; ++i)
        {
            // read both names
            firstName  = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);
            secondName = SharedHelperData::ReadString(file, importParams.mSharedData, endianType);

            if (GetLogging())
            {
                MCore::LogDetailedInfo("  + [%d] '%s' -> '%s'", i, firstName.c_str(), secondName.c_str());
            }

            // create the entry
            if (importParams.mNodeMapSettings->mLoadNodes)
            {
                importParams.mNodeMap->AddEntry(firstName.c_str(), secondName.c_str());
            }
        }

        return true;
    }
} // namespace EMotionFX
