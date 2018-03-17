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

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/StringIDGenerator.h>
#include "AnimGraphAttributeTypes.h"
#include "BlendTreeConnection.h"
#include "AnimGraphObject.h"
#include "AnimGraphInstance.h"
#include "AnimGraphNodeData.h"

#include <AzCore/std/functional.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraph;
    class Pose;
    class AnimGraphNode;
    class MotionInstance;
    class ActorInstance;
    class AnimGraphStateMachine;
    class BlendTree;
    class AnimGraphStateTransition;
    class AnimGraphRefCountedData;
    class AnimGraph;
    class AnimGraphTransitionCondition;


    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphNode
        : public AnimGraphObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(AnimGraphNode, "{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}", AnimGraphObject);

        enum
        {
            BASETYPE_ID = 0x00000001
        };

        struct EMFX_API Port
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphNode::Port, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

            BlendTreeConnection*    mConnection;            // the connection plugged in this port
            uint32                  mCompatibleTypes[4];    // four possible compatible types
            uint32                  mPortID;                // the unique port ID (unique inside the node input or output port lists)
            uint32                  mNameID;                // the name of the port (using the StringIDGenerator)
            uint32                  mAttributeIndex;        // the index into the animgraph instance global attributes array

            MCORE_INLINE const char* GetName() const                    { return MCore::GetStringIDGenerator().GetName(mNameID).AsChar(); }
            MCORE_INLINE const MCore::String& GetNameString() const     { return MCore::GetStringIDGenerator().GetName(mNameID); }

            // copy settings from another port (always makes the mConnection and mValue nullptr though)
            void InitFrom(const Port& other)
            {
                for (uint32 i = 0; i < 4; ++i)
                {
                    mCompatibleTypes[i] = other.mCompatibleTypes[i];
                }

                mPortID         = other.mPortID;
                mNameID         = other.mNameID;
                mConnection     = nullptr;
                mAttributeIndex = other.mAttributeIndex;
            }

            // get the attribute value
            MCORE_INLINE MCore::Attribute* GetAttribute(AnimGraphInstance* animGraphInstance) const
            {
                return animGraphInstance->GetInternalAttribute(mAttributeIndex);
            }

            // port connection compatibility check
            bool CheckIfIsCompatibleWith(const Port& otherPort) const
            {
                // check the data types
                for (uint32 i = 0; i < 4; ++i)
                {
                    if (otherPort.mCompatibleTypes[i] == mCompatibleTypes[0])
                    {
                        return true;
                    }

                    // if there aren't any more compatibility types and we haven't found a compatible one so far, return false
                    if (mCompatibleTypes[i] == 0)
                    {
                        return false;
                    }
                }

                // not compatible
                return false;
            }

            // clear compatibility types
            void ClearCompatibleTypes()
            {
                mCompatibleTypes[0] = 0;
                mCompatibleTypes[1] = 0;
                mCompatibleTypes[2] = 0;
                mCompatibleTypes[3] = 0;
            }

            void Clear()
            {
                ClearCompatibleTypes();
            }

            Port()
                : mConnection(nullptr)
                , mPortID(MCORE_INVALIDINDEX32)
                , mNameID(MCORE_INVALIDINDEX32)
                , mAttributeIndex(MCORE_INVALIDINDEX32)  { ClearCompatibleTypes(); }
            ~Port() { }
        };

        AnimGraphNode(AnimGraph* animGraph, const char* name, uint32 typeID);
        virtual ~AnimGraphNode();

        virtual void InitForAnimGraph(AnimGraph* animGraph)  { MCORE_UNUSED(animGraph); }
        virtual void Reinit()                                   { }
        virtual void Prepare(AnimGraphInstance* animGraphInstance) { MCORE_UNUSED(animGraphInstance); }

        virtual bool GetSupportsVisualization() const           { return false; }
        virtual bool GetSupportsDisable() const                 { return false; }
        virtual bool GetHasVisualOutputPorts() const            { return true; }
        virtual bool GetCanHaveOnlyOneInsideParent() const      { return false; }
        virtual bool GetIsDeletable() const                     { return true; }
        virtual bool GetIsLastInstanceDeletable() const         { return true; }
        virtual bool GetCanActAsState() const                   { return false; }
        virtual bool GetHasVisualGraph() const                  { return false; }
        virtual bool GetCanBeInsideSubStateMachineOnly() const  { return false; }
        virtual bool GetCanHaveChildren() const                 { return false; }
        virtual bool GetHasOutputPose() const                   { return false; }
        virtual uint32 GetVisualColor() const                   { return MCore::RGBA(72, 63, 238); }
        virtual uint32 GetHasChildIndicatorColor() const        { return MCore::RGBA(255, 255, 0); }

        virtual void InitInternalAttributes(AnimGraphInstance* animGraphInstance) override;
        virtual void RemoveInternalAttributesForAllInstances() override;
        virtual void DecreaseInternalAttributeIndices(uint32 decreaseEverythingHigherThan) override;

        virtual AnimGraphObject* RecursiveClone(AnimGraph* animGraph, AnimGraphObject* parentObject) override;
        virtual void RecursiveClonePostProcess(AnimGraphNode* resultNode);

        uint32 GetBaseType() const override;

        void OutputAllIncomingNodes(AnimGraphInstance* animGraphInstance);
        void UpdateAllIncomingNodes(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        void UpdateIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* node, float timePassedInSeconds);

        void RecursiveOnUpdateUniqueData(AnimGraphInstance* animGraphInstance);
        virtual void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        virtual void OnUpdateAttributes() override;
        virtual void OnRenamedNode(AnimGraph* animGraph, AnimGraphNode* node, const MCore::String& oldName) override;
        virtual void OnCreatedNode(AnimGraph* animGraph, AnimGraphNode* node) override;
        virtual void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;
        void RecursiveUpdateAttributes();

        virtual void RecursiveInit(AnimGraphInstance* animGraphInstance) override;
        virtual void Init(AnimGraphInstance* animGraphInstance) override { MCORE_UNUSED(animGraphInstance); }

        void PerformOutput(AnimGraphInstance* animGraphInstance);
        void PerformTopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        void PerformUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        void PerformPostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);

        MCORE_INLINE float GetDuration(AnimGraphInstance* animGraphInstance) const                 { return FindUniqueNodeData(animGraphInstance)->GetDuration(); }
        virtual void SetCurrentPlayTime(AnimGraphInstance* animGraphInstance, float timeInSeconds) { FindUniqueNodeData(animGraphInstance)->SetCurrentPlayTime(timeInSeconds); }
        virtual float GetCurrentPlayTime(AnimGraphInstance* animGraphInstance) const               { return FindUniqueNodeData(animGraphInstance)->GetCurrentPlayTime(); }

        MCORE_INLINE uint32 GetSyncIndex(AnimGraphInstance* animGraphInstance) const               { return FindUniqueNodeData(animGraphInstance)->GetSyncIndex(); }
        MCORE_INLINE void SetSyncIndex(AnimGraphInstance* animGraphInstance, uint32 syncIndex)     { FindUniqueNodeData(animGraphInstance)->SetSyncIndex(syncIndex); }

        virtual void SetPlaySpeed(AnimGraphInstance* animGraphInstance, float speedFactor)         { FindUniqueNodeData(animGraphInstance)->SetPlaySpeed(speedFactor); }
        virtual float GetPlaySpeed(AnimGraphInstance* animGraphInstance) const                     { return FindUniqueNodeData(animGraphInstance)->GetPlaySpeed(); }
        virtual void SetCurrentPlayTimeNormalized(AnimGraphInstance* animGraphInstance, float normalizedTime);
        virtual void Rewind(AnimGraphInstance* animGraphInstance);

        void AutoSync(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, float weight, ESyncMode syncMode, bool resync, bool modifyMasterSpeed = true);
        void SyncFullNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, float weight, bool modifyMasterSpeed = true);
        void SyncPlayTime(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode);
        void SyncUsingSyncTracks(AnimGraphInstance* animGraphInstance, AnimGraphNode* syncWithNode, AnimGraphSyncTrack* syncTrackA, AnimGraphSyncTrack* syncTrackB, float weight, bool resync, bool modifyMasterSpeed = true);
        void SyncPlaySpeeds(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, float weight, bool modifyMasterSpeed = true);
        virtual void HierarchicalSyncInputNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* inputNode, AnimGraphNodeData* uniqueDataOfThisNode);
        void HierarchicalSyncAllInputNodes(AnimGraphInstance* animGraphInstance, AnimGraphNodeData* uniqueDataOfThisNode);
        static void CalcSyncFactors(AnimGraphInstance* animGraphInstance, AnimGraphNode* masterNode, AnimGraphNode* slaveNode, ESyncMode syncMode, float weight, float* outMasterFactor, float* outSlaveFactor, float* outPlaySpeed);

        void RequestPoses(AnimGraphInstance* animGraphInstance);
        void FreeIncomingPoses(AnimGraphInstance* animGraphInstance);
        void IncreaseInputRefCounts(AnimGraphInstance* animGraphInstance);
        void DecreaseRef(AnimGraphInstance* animGraphInstance);

        void RequestRefDatas(AnimGraphInstance* animGraphInstance);
        void FreeIncomingRefDatas(AnimGraphInstance* animGraphInstance);
        void IncreaseInputRefDataRefCounts(AnimGraphInstance* animGraphInstance);
        void DecreaseRefDataRef(AnimGraphInstance* animGraphInstance);

        /**
         * Get a pointer to the custom data you stored.
         * Custom data can for example link a game or engine object. The pointer that you specify will not be deleted when the object is being destructed.
         * @result A void pointer to the custom data you have specified.
         */
        void* GetCustomData() const;

        /**
         * Set a pointer to the custom data you stored.
         * Custom data can for example link a game or engine object. The pointer that you specify will not be deleted when the object is being destructed.
         * @param dataPointer A void pointer to the custom data, which could for example be your engine or game object.
         */
        void SetCustomData(void* dataPointer);

        virtual void RegisterPorts() = 0;

        virtual AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const  { MCORE_UNUSED(animGraphInstance); MCORE_ASSERT(false); return nullptr; }

        virtual void RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, MCore::Array<AnimGraphNode*>* outNodes, uint32 nodeTypeID = MCORE_INVALIDINDEX32) const;

        void CollectChildNodesOfType(uint32 nodeTypeID, MCore::Array<AnimGraphNode*>* outNodes) const; // note: outNodes is NOT cleared internally, nodes are added to the array

        void RecursiveCollectNodesOfType(uint32 nodeTypeID, MCore::Array<AnimGraphNode*>* outNodes) const; // note: outNodes is NOT cleared internally, nodes are added to the array
        void RecursiveCollectTransitionConditionsOfType(uint32 conditionTypeID, MCore::Array<AnimGraphTransitionCondition*>* outConditions) const; // note: outNodes is NOT cleared internally, nodes are added to the array

        virtual void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition)
        {
            MCORE_UNUSED(animGraphInstance);
            MCORE_UNUSED(previousState);
            MCORE_UNUSED(usedTransition);
            //MCore::LogInfo("OnStateEntering '%s', with transition 0x%x, prev state='%s'", GetName(), usedTransition, (previousState!=nullptr) ? previousState->GetName() : "");
        }

        virtual void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* previousState, AnimGraphStateTransition* usedTransition)
        {
            MCORE_UNUSED(animGraphInstance);
            MCORE_UNUSED(previousState);
            MCORE_UNUSED(usedTransition);
            //MCore::LogInfo("OnStateEnter '%s', with transition 0x%x, prev state='%s'", GetName(), usedTransition, (previousState!=nullptr) ? previousState->GetName() : "");
        }

        virtual void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* targetState, AnimGraphStateTransition* usedTransition)
        {
            MCORE_UNUSED(animGraphInstance);
            MCORE_UNUSED(targetState);
            MCORE_UNUSED(usedTransition);
            //MCore::LogInfo("OnStateExit '%s', with transition 0x%x, target state='%s'", GetName(), usedTransition, (targetState!=nullptr) ? targetState->GetName() : "");
        }

        virtual void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* newState, AnimGraphStateTransition* usedTransition)
        {
            MCORE_UNUSED(animGraphInstance);
            MCORE_UNUSED(newState);
            MCORE_UNUSED(usedTransition);
            //MCore::LogInfo("OnStateEnd '%s', with transition 0x%x, new state='%s'", GetName(), usedTransition, (newState!=nullptr) ? newState->GetName() : "");
        }

        void RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet) override;

        const char* GetName() const;
        const MCore::String& GetNameString() const;
        void SetName(const char* name);

        MCORE_INLINE uint32 GetID() const                                   { return mNameID; }
        MCORE_INLINE uint32 GetUniqueID() const                             { return mUniqueID; }

        const MCore::Attribute* GetInputValue(AnimGraphInstance* instance, uint32 inputPort) const;

        uint32 FindInputPortByID(uint32 portID) const;
        uint32 FindOutputPortByID(uint32 portID) const;

        Port* FindInputPortByName(const char* portName);
        Port* FindOutputPortByName(const char* portName);

        // this is about incoming connections
        bool ValidateConnections() const;
        BlendTreeConnection* AddConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort);
        void RemoveConnectionsUsingNode(AnimGraphNode* source);
        void RemoveConnection(BlendTreeConnection* connection, bool delFromMem = true);
        void RemoveConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort);
        bool RemoveConnectionByID(uint32 id, bool delFromMem = true);
        void RemoveAllConnections();

        /**
         * Collect all outgoing connections.
         * As the nodes only store the incoming connections getting access to the outgoing connections is a bit harder. For that we need to process all nodes in the graph where
         * our node is located, iterate over all connections and check if they are coming from our node. Don't call this function at runtime.
         * @param[out] outConnections This will hold all output connections of our node. The array will be cleared upfront.
         * @param[out] outTargetNodes As the connections only store the source node we need to separately store the target nodes of the outgoing connections here. The number of elements in this array will be equal to the number of output connections. The array will hold all output connections of our node. The array will be cleared upfront.
         */
        void CollectOutgoingConnections(MCore::Array<BlendTreeConnection*>* outConnections, MCore::Array<AnimGraphNode*>* outTargetNodes) const;

        MCORE_INLINE bool                           GetInputNumberAsBool(AnimGraphInstance* animGraphInstance, uint32 inputPortNr) const
        {
            const MCore::Attribute* attribute = GetInputAttribute(animGraphInstance, inputPortNr);
            if (attribute == nullptr)
            {
                return false;
            }
            MCORE_ASSERT(attribute->GetType() == MCore::AttributeFloat::TYPE_ID);
            const float floatValue = static_cast<const MCore::AttributeFloat*>(attribute)->GetValue();
            return (MCore::Math::IsFloatZero(floatValue) == false);
        }
        MCORE_INLINE float                          GetInputNumberAsFloat(AnimGraphInstance* animGraphInstance, uint32 inputPortNr) const
        {
            const MCore::Attribute* attribute = GetInputAttribute(animGraphInstance, inputPortNr);
            if (attribute == nullptr)
            {
                return 0.0f;
            }
            MCORE_ASSERT(attribute->GetType() == MCore::AttributeFloat::TYPE_ID);
            return static_cast<const MCore::AttributeFloat*>(attribute)->GetValue();
        }
        MCORE_INLINE int32                          GetInputNumberAsInt32(AnimGraphInstance* animGraphInstance, uint32 inputPortNr) const
        {
            const MCore::Attribute* attribute = GetInputAttribute(animGraphInstance, inputPortNr);
            if (attribute == nullptr)
            {
                return 0;
            }
            MCORE_ASSERT(attribute->GetType() == MCore::AttributeFloat::TYPE_ID);
            return (int32) static_cast<const MCore::AttributeFloat*>(attribute)->GetValue();
        }
        MCORE_INLINE uint32                         GetInputNumberAsUint32(AnimGraphInstance* animGraphInstance, uint32 inputPortNr) const
        {
            const MCore::Attribute* attribute = GetInputAttribute(animGraphInstance, inputPortNr);
            if (attribute == nullptr)
            {
                return 0;
            }
            MCORE_ASSERT(attribute->GetType() == MCore::AttributeFloat::TYPE_ID);
            return (uint32) static_cast<const MCore::AttributeFloat*>(attribute)->GetValue();
        }
        MCORE_INLINE AnimGraphNode*                GetInputNode(uint32 portNr)
        {
            const BlendTreeConnection* con = mInputPorts[portNr].mConnection;
            if (con == nullptr)
            {
                return nullptr;
            }
            return con->GetSourceNode();
        }
        MCORE_INLINE MCore::Attribute*              GetInputAttribute(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            const BlendTreeConnection* con = mInputPorts[portNr].mConnection;
            if (con == nullptr)
            {
                return nullptr;
            }
            return con->GetSourceNode()->GetOutputValue(animGraphInstance, con->GetSourcePort());
        }
        MCORE_INLINE MCore::AttributeFloat*         GetInputFloat(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeFloat::TYPE_ID);
            return static_cast<MCore::AttributeFloat*>(attrib);
        }
        MCORE_INLINE MCore::AttributeInt32*         GetInputInt32(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeInt32::TYPE_ID);
            return static_cast<MCore::AttributeInt32*>(attrib);
        }
        MCORE_INLINE MCore::AttributeString*        GetInputString(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeString::TYPE_ID);
            return static_cast<MCore::AttributeString*>(attrib);
        }
        MCORE_INLINE MCore::AttributeBool*          GetInputBool(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeBool::TYPE_ID);
            return static_cast<MCore::AttributeBool*>(attrib);
        }
        MCORE_INLINE MCore::AttributeVector2*       GetInputVector2(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeVector2::TYPE_ID);
            return static_cast<MCore::AttributeVector2*>(attrib);
        }
        MCORE_INLINE MCore::AttributeVector3*       GetInputVector3(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeVector3::TYPE_ID);
            return static_cast<MCore::AttributeVector3*>(attrib);
        }
        MCORE_INLINE MCore::AttributeVector4*       GetInputVector4(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeVector4::TYPE_ID);
            return static_cast<MCore::AttributeVector4*>(attrib);
        }
        MCORE_INLINE MCore::AttributeQuaternion*    GetInputQuaternion(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeQuaternion::TYPE_ID);
            return static_cast<MCore::AttributeQuaternion*>(attrib);
        }
        MCORE_INLINE MCore::AttributeColor*         GetInputColor(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == MCore::AttributeColor::TYPE_ID);
            return static_cast<MCore::AttributeColor*>(attrib);
        }
        MCORE_INLINE AttributeRotation*             GetInputRotation(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == AttributeRotation::TYPE_ID);
            return static_cast<AttributeRotation*>(attrib);
        }
        MCORE_INLINE AttributeMotionInstance*       GetInputMotionInstance(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == AttributeMotionInstance::TYPE_ID);
            return static_cast<AttributeMotionInstance*>(attrib);
        }
        MCORE_INLINE AttributeNodeMask*             GetInputNodeMask(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == AttributeNodeMask::TYPE_ID);
            return static_cast<AttributeNodeMask*>(attrib);
        }
        MCORE_INLINE AttributePose*                 GetInputPose(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == AttributePose::TYPE_ID);
            return static_cast<AttributePose*>(attrib);
        }
        MCORE_INLINE AttributeGoalNode*             GetInputGoalNode(AnimGraphInstance* animGraphInstance, uint32 portNr) const
        {
            MCore::Attribute* attrib = GetInputAttribute(animGraphInstance, portNr);
            if (attrib == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(attrib->GetType() == AttributeGoalNode::TYPE_ID);
            return static_cast<AttributeGoalNode*>(attrib);
        }

        MCORE_INLINE MCore::Attribute*              GetOutputAttribute(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const                     { return mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance); }
        MCORE_INLINE MCore::AttributeFloat*         GetOutputNumber(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeFloat::TYPE_ID);
            return static_cast<MCore::AttributeFloat*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeFloat*         GetOutputFloat(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeFloat::TYPE_ID);
            return static_cast<MCore::AttributeFloat*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeInt32*         GetOutputInt32(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeInt32::TYPE_ID);
            return static_cast<MCore::AttributeInt32*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeString*        GetOutputString(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeString::TYPE_ID);
            return static_cast<MCore::AttributeString*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeBool*          GetOutputBool(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeBool::TYPE_ID);
            return static_cast<MCore::AttributeBool*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeVector2*       GetOutputVector2(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeVector2::TYPE_ID);
            return static_cast<MCore::AttributeVector2*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeVector3*       GetOutputVector3(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeVector3::TYPE_ID);
            return static_cast<MCore::AttributeVector3*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeVector4*       GetOutputVector4(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeVector4::TYPE_ID);
            return static_cast<MCore::AttributeVector4*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeQuaternion*    GetOutputQuaternion(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeQuaternion::TYPE_ID);
            return static_cast<MCore::AttributeQuaternion*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE MCore::AttributeColor*         GetOutputColor(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == MCore::AttributeColor::TYPE_ID);
            return static_cast<MCore::AttributeColor*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE AttributeRotation*             GetOutputRotation(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == AttributeRotation::TYPE_ID);
            return static_cast<AttributeRotation*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE AttributePose*                 GetOutputPose(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == AttributePose::TYPE_ID);
            return static_cast<AttributePose*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE AttributeMotionInstance*       GetOutputMotionInstance(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == AttributeMotionInstance::TYPE_ID);
            return static_cast<AttributeMotionInstance*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE AttributeNodeMask*             GetOutputNodeMask(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == AttributeNodeMask::TYPE_ID);
            return static_cast<AttributeNodeMask*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }
        MCORE_INLINE AttributeGoalNode*             GetOutputGoalNode(AnimGraphInstance* animGraphInstance, uint32 outputPortIndex) const
        {
            if (mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance) == nullptr)
            {
                return nullptr;
            }
            MCORE_ASSERT(mOutputPorts[outputPortIndex].mCompatibleTypes[0] == AttributeGoalNode::TYPE_ID);
            return static_cast<AttributeGoalNode*>(mOutputPorts[outputPortIndex].GetAttribute(animGraphInstance));
        }

        void SetupInputPortAsNumber(const char* name, uint32 inputPortNr, uint32 portID);
        void SetupInputPort(const char* name, uint32 inputPortNr, uint32 attributeTypeID, uint32 portID);

        void SetupOutputPort(const char* name, uint32 portIndex, uint32 attributeTypeID, uint32 portID);
        void SetupOutputPortAsPose(const char* name, uint32 outputPortNr, uint32 portID);
        void SetupOutputPortAsMotionInstance(const char* name, uint32 outputPortNr, uint32 portID);

        bool GetHasConnection(AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const;
        BlendTreeConnection* FindConnection(const AnimGraphNode* sourceNode, uint16 sourcePort, uint16 targetPort) const;

        /**
         * Find the connection at the given port.
         * Search over the incoming connections that are stored within this node and check if they are connected at the given port.
         * @param[in] port The port inside this node of connection to search for.
         * @result A pointer to the connection at the given port, nullptr in case there is nothing connected to that port.
         */
        BlendTreeConnection* FindConnection(uint16 port) const;

        /**
         * Check if a connection is connected to the given input port.
         * @param[in] inputPort The input port id to check.
         * @result True in case there is a connection plugged into the given input port, false if not.
         */
        bool CheckIfIsInputPortConnected(uint16 inputPort) const;

        AnimGraphNode* RecursiveFindNodeByID(uint32 id) const;
        AnimGraphNode* RecursiveFindNodeByUniqueID(uint32 id) const;

        virtual void RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToReset = 0xffffffff);

        uint32 GetNumInputs() const;
        uint32 GetNumOutputs() const;
        const MCore::Array<AnimGraphNode::Port>& GetInputPorts() const;
        const MCore::Array<AnimGraphNode::Port>& GetOutputPorts() const;
        void SetInputPorts(const MCore::Array<AnimGraphNode::Port>& inputPorts);
        void InitInputPorts(uint32 numPorts);
        void InitOutputPorts(uint32 numPorts);
        void SetInputPortName(uint32 portIndex, const char* name);
        void SetOutputPortName(uint32 portIndex, const char* name);
        uint32 FindOutputPortIndex(const char* name) const;
        uint32 FindInputPortIndex(const char* name) const;
        uint32 AddOutputPort();
        uint32 AddInputPort();
        void InsertOutputPort(uint32 index)                                         { mOutputPorts.Insert(index); }
        void InsertInputPort(uint32 index)                                          { mInputPorts.Insert(index); }
        void RemoveOutputPort(uint32 index)                                         { mOutputPorts.Remove(index); }
        void RemoveInputPort(uint32 index)                                          { mInputPorts.Remove(index); }
        virtual bool GetIsStateTransitionNode() const                               { return false; }
        MCORE_INLINE MCore::Attribute* GetOutputValue(AnimGraphInstance* animGraphInstance, uint32 portIndex) const            { return animGraphInstance->GetInternalAttribute(mOutputPorts[portIndex].mAttributeIndex); }
        MCORE_INLINE Port& GetInputPort(uint32 index)                               { return mInputPorts[index]; }
        MCORE_INLINE Port& GetOutputPort(uint32 index)                              { return mOutputPorts[index]; }
        MCORE_INLINE const Port& GetInputPort(uint32 index) const                   { return mInputPorts[index]; }
        MCORE_INLINE const Port& GetOutputPort(uint32 index) const                  { return mOutputPorts[index]; }

        MCORE_INLINE uint32 GetNumConnections() const                               { return mConnections.GetLength(); }
        MCORE_INLINE BlendTreeConnection* GetConnection(uint32 index) const         { return mConnections[index]; }

        MCORE_INLINE AnimGraphNode* GetParentNode() const                          { return mParentNode; }
        MCORE_INLINE void SetParentNode(AnimGraphNode* node)                       { mParentNode = node; }

        /**
         * Check if the given node is the parent or the parent of the parent etc. of the node.
         * @param[in] node The parent node we try to search.
         * @result True in case the given node is the parent or the parent of the parent etc. of the node, false in case the given node wasn't found in any of the parents.
         */
        bool RecursiveIsParentNode(AnimGraphNode* node) const;

        /**
         * Check if the given node is a child or a child of a child etc. of the node.
         * @param[in] node The child node we try to search.
         * @result True in case the given node is a child or the child of a child etc. of the node, false in case the given node wasn't found in any of the child nodes.
         */
        bool RecursiveIsChildNode(AnimGraphNode* node) const;

        uint32 FindConnectionFromNode(AnimGraphNode* sourceNode, uint16 sourcePort) const;

        /**
         * Find child node by name. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] name The name of the node to search.
         * @return A pointer to the child node with the given name in case of success, in the other case a nullptr pointer will be returned.
         */
        AnimGraphNode* FindChildNode(const char* name) const;

        /**
         * Find child node index by name. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] name The name of the node to search.
         * @return The index of the child node with the given name in case of success, in the other case MCORE_INVALIDINDEX32 will be returned.
         */
        uint32 FindChildNodeIndex(const char* name) const;

        /**
         * Find child node index. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] node A pointer to the node for which we want to find the child node index.
         * @return The index of the child node in case of success, in the other case MCORE_INVALIDINDEX32 will be returned.
         */
        uint32 FindChildNodeIndex(AnimGraphNode* node) const;

        /**
         * Check if a child node of the given type exists. This will only iterate through the child nodes and isn't a recursive process.
         * @param[in] uuid The rtti type id of the node to check for.
         * @return True in case a child node of the given type was found, false if not.
         */
        bool HasChildNodeOfType(const AZ::Uuid& uuid) const;

        uint32 RecursiveCalcNumNodes() const;
        uint32 RecursiveCalcNumNodeConnections() const;

        void CopyBaseNodeTo(AnimGraphNode* node) const;

        MCORE_INLINE uint32 GetNumChildNodes() const                        { return mChildNodes.GetLength(); }
        MCORE_INLINE AnimGraphNode* GetChildNode(uint32 index) const       { return mChildNodes[index]; }

        void SetNodeInfo(const char* info);
        const char* GetNodeInfo() const;
        const MCore::String& GetNodeInfoString() const;

        void AddChildNode(AnimGraphNode* node);
        void ReserveChildNodes(uint32 numChildNodes);

        void RemoveChildNode(uint32 index, bool delFromMem = true);
        void RemoveChildNodeByPointer(AnimGraphNode* node, bool delFromMem = true);
        void RemoveAllChildNodes(bool delFromMem = true);
        bool CheckIfHasChildOfType(uint32 nodeTypeID) const;    // non-recursive

        void MarkConnectionVisited(AnimGraphNode* sourceNode);
        void OutputIncomingNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* nodeToOutput);

        MCORE_INLINE AnimGraphNodeData* FindUniqueNodeData(AnimGraphInstance* animGraphInstance) const            { return animGraphInstance->FindUniqueNodeData(this); }

        bool GetIsEnabled() const;
        void SetIsEnabled(bool enabled);
        bool GetIsCollapsed() const;
        void SetIsCollapsed(bool collapsed);
        void SetVisualizeColor(uint32 color);
        uint32 GetVisualizeColor() const;
        void SetVisualPos(int32 x, int32 y);
        int32 GetVisualPosX() const;
        int32 GetVisualPosY() const;
        bool GetIsVisualizationEnabled() const;
        void SetVisualization(bool enabled);

        #ifdef EMFX_EMSTUDIOBUILD
        bool HierarchicalHasError(AnimGraphInstance* animGraphInstance, bool onlyCheckChildNodes = false) const;
        void SetHasError(AnimGraphInstance* animGraphInstance, bool hasError);
        #endif

        // collect internal objects
        virtual void RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const override;
        virtual void RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled);

        void FilterEvents(AnimGraphInstance* animGraphInstance, EEventMode eventMode, AnimGraphNode* nodeA, AnimGraphNode* nodeB, float localWeight, AnimGraphRefCountedData* refData);

        bool GetCanVisualize(AnimGraphInstance* animGraphInstance) const;

        MCORE_INLINE uint32 GetNodeIndex() const                                            { return mNodeIndex; }
        MCORE_INLINE void SetNodeIndex(uint32 index)                                        { mNodeIndex = index; }

        MCORE_INLINE void ResetPoseRefCount(AnimGraphInstance* animGraphInstance)              { FindUniqueNodeData(animGraphInstance)->SetPoseRefCount(0); }
        MCORE_INLINE void IncreasePoseRefCount(AnimGraphInstance* animGraphInstance)           { FindUniqueNodeData(animGraphInstance)->IncreasePoseRefCount(); }
        MCORE_INLINE void DecreasePoseRefCount(AnimGraphInstance* animGraphInstance)           { FindUniqueNodeData(animGraphInstance)->DecreasePoseRefCount(); }
        MCORE_INLINE uint32 GetPoseRefCount(AnimGraphInstance* animGraphInstance) const        { return FindUniqueNodeData(animGraphInstance)->GetPoseRefCount(); }

        MCORE_INLINE void ResetRefDataRefCount(AnimGraphInstance* animGraphInstance)           { FindUniqueNodeData(animGraphInstance)->SetRefDataRefCount(0); }
        MCORE_INLINE void IncreaseRefDataRefCount(AnimGraphInstance* animGraphInstance)        { FindUniqueNodeData(animGraphInstance)->IncreaseRefDataRefCount(); }
        MCORE_INLINE void DecreaseRefDataRefCount(AnimGraphInstance* animGraphInstance)        { FindUniqueNodeData(animGraphInstance)->DecreaseRefDataRefCount(); }
        MCORE_INLINE uint32 GetRefDataRefCount(AnimGraphInstance* animGraphInstance) const     { return FindUniqueNodeData(animGraphInstance)->GetRefDataRefCount(); }

        void GatherRequiredConnectionChangesInputPorts(const MCore::Array<Port>& newPorts, MCore::Array<BlendTreeConnection*>* outToRemoveConnections, MCore::Array<BlendTreeConnection*>* outChangedConnections);
        void SetInputPortChangeFunction(const AZStd::function<void(AnimGraphNode* node, const MCore::Array<AnimGraphNode::Port>& newPorts)>& func);
        void ExecuteInputPortChangeFunction(const MCore::Array<Port>& newPorts);

    protected:
        uint32                              mNodeIndex;
        AnimGraphNode*                      mParentNode;
        MCore::Array<BlendTreeConnection*>  mConnections;
        MCore::Array<Port>                  mInputPorts;
        MCore::Array<Port>                  mOutputPorts;
        MCore::Array<AnimGraphNode*>        mChildNodes;
        MCore::String                       mNodeInfo;
        void*                               mCustomData;
        uint32                              mNameID;
        uint32                              mUniqueID;
        int32                               mPosX;
        int32                               mPosY;
        uint32                              mVisualizeColor;
        bool                                mVisEnabled;
        bool                                mIsCollapsed;
        bool                                mDisabled;
        AZStd::function<void(AnimGraphNode* node, const MCore::Array<AnimGraphNode::Port>& newPorts)>   mInputPortChangeFunction;

        virtual void Output(AnimGraphInstance* animGraphInstance);
        virtual void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);
        virtual void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        virtual void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds);

        void RecursiveCountChildNodes(uint32& numNodes) const;
        void RecursiveCountNodeConnections(uint32& numConnections) const;
    };


    // the default input port structure change handler function
    // this automatically removes connections that became invalid and updates ones that have to be remapped to other ports
    // it figures this out based on port names
    void AnimGraphNodeDefaultInputPortChangeFunction(AnimGraphNode* node, const MCore::Array<AnimGraphNode::Port>& newPorts);
}   // namespace EMotionFX
