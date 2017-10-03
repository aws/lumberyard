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
#include <MCore/Source/Attribute.h>
#include "BaseObject.h"
#include "AnimGraphEventBuffer.h"
#include "AnimGraphObject.h"

namespace AZ
{
    class Vector2;
}

namespace EMotionFX
{
    // forward declarations
    class ActorInstance;
    class AnimGraph;
    class MotionInstance;
    class Pose;
    class MotionSet;
    class AnimGraphNode;
    class AnimGraphStateTransition;
    class AnimGraphInstanceEventHandler;
    class AnimGraphReferenceNode;
    class AnimGraphObjectData;
    class AnimGraphNodeData;

    /**
     * The anim graph instance class.
     */
    class EMFX_API AnimGraphInstance
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphInstance, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_INSTANCE);

    public:
        enum
        {
            OBJECTFLAGS_OUTPUT_READY                = 1 << 0,
            OBJECTFLAGS_UPDATE_READY                = 1 << 1,
            OBJECTFLAGS_TOPDOWNUPDATE_READY         = 1 << 2,
            OBJECTFLAGS_POSTUPDATE_READY            = 1 << 3,
            OBJECTFLAGS_SYNCED                      = 1 << 4,
            OBJECTFLAGS_RESYNC                      = 1 << 5,
            OBJECTFLAGS_SYNCINDEX_CHANGED           = 1 << 6,
            OBJECTFLAGS_PLAYMODE_BACKWARD           = 1 << 7,
            OBJECTFLAGS_IS_SYNCMASTER               = 1 << 8
        };

        struct EMFX_API ReferenceInfo
        {
            AnimGraphNode*             mSourceNode;        // the node inside the animgraph of this animgraph instance to reference, so the node where this is an instance of
            AnimGraphInstance*         mParameterSource;   // the animgraph instance to reference parameter values from
            AnimGraphReferenceNode*    mReferenceNode;     // the reference node itself, can be nullptr if not used in combination with a reference node

            ReferenceInfo()
            {
                mSourceNode         = nullptr;  // inactive, no node selected
                mParameterSource    = nullptr;  // create unique parameters
                mReferenceNode      = nullptr;  // not linked to any reference node
            }
        };

        struct EMFX_API InitSettings
        {
            bool    mPreInitMotionInstances;

            InitSettings()
            {
                mPreInitMotionInstances = false;
            }
        };

        static AnimGraphInstance* Create(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings = nullptr, ReferenceInfo* referenceInfo = nullptr);

        void Init();
        void Output(Pose* outputPose, bool autoFreeAllPoses = true);

        void Start();
        void Stop();

        MCORE_INLINE ActorInstance* GetActorInstance() const            { return mActorInstance; }
        MCORE_INLINE AnimGraph* GetAnimGraph() const                  { return mAnimGraph; }
        MCORE_INLINE MotionSet* GetMotionSet() const                    { return mMotionSet; }

        bool GetFloatParameterValue(const char* paramName, float* outValue);
        bool GetFloatParameterValueAsBool(const char* paramName, bool* outValue);
        bool GetFloatParameterValueAsInt(const char* paramName, int32* outValue);
        bool GetVector2ParameterValue(const char* paramName, AZ::Vector2* outValue);
        bool GetVector3ParameterValue(const char* paramName, MCore::Vector3* outValue);
        bool GetVector4ParameterValue(const char* paramName, AZ::Vector4* outValue);
        bool GetRotationParameterValue(const char* paramName, MCore::Quaternion* outRotation);

        bool GetFloatParameterValue(uint32 paramIndex, float* outValue);
        bool GetFloatParameterValueAsBool(uint32 paramIndex, bool* outValue);
        bool GetFloatParameterValueAsInt(uint32 paramIndex, int32* outValue);
        bool GetVector2ParameterValue(uint32 paramIndex, AZ::Vector2* outValue);
        bool GetVector3ParameterValue(uint32 paramIndex, MCore::Vector3* outValue);
        bool GetVector4ParameterValue(uint32 paramIndex, AZ::Vector4* outValue);
        bool GetRotationParameterValue(uint32 paramIndex, MCore::Quaternion* outRotation);

        void SetMotionSet(MotionSet* motionSet);

        void CreateParameterValues();
        void AddMissingParameterValues();   // add the missing parameters that the anim graph has to this anim graph instance
        void ReInitParameterValue(uint32 index);
        void RemoveParameterValue(uint32 index, bool delFromMem = true);
        void AddParameterValue();       // add the last anim graph parameter to this instance
        void InsertParameterValue(uint32 index);    // add the parameter of the animgraph, at a given index
        void RemoveAllParameters(bool delFromMem);

        template <typename T>
        MCORE_INLINE T* GetParameterValueChecked(uint32 index) const
        {
            MCore::Attribute* baseAttrib = mParamValues[index];
            if (baseAttrib->GetType() == T::TYPE_ID)
            {
                return static_cast<T*>(baseAttrib);
            }
            return nullptr;
        }

        MCORE_INLINE MCore::Attribute* GetParameterValue(uint32 index) const                            { return mParamValues[index]; }
        MCore::Attribute* FindParameter(const char* name) const;
        uint32 FindParameterIndex(const char* name) const;

        bool SwitchToState(const char* stateName);
        bool TransitionToState(const char* stateName);

        void SwapParameterValues(uint32 whatIndex, uint32 withIndex);

        void OnUpdateUniqueData();
        void ApplyMotionExtraction();

        void RecursiveResetFlags(uint32 flagsToDisable);
        void ResetFlagsForAllObjects(uint32 flagsToDisable);
        void ResetFlagsForAllNodes(uint32 flagsToDisable);
        void ResetFlagsForAllObjects();
        void ResetPoseRefCountsForAllNodes();
        void ResetRefDataRefCountsForAllNodes();

        void InitInternalAttributes();
        void RemoveAllInternalAttributes();
        void ReserveInternalAttributes(uint32 totalNumInternalAttributes);
        void RemoveInternalAttribute(uint32 index, bool delFromMem = true);   // removes the internal attribute (does not update any indices of other attributes)
        uint32 AddInternalAttribute(MCore::Attribute* attribute);           // returns the index of the new added attribute

        MCORE_INLINE MCore::Attribute* GetInternalAttribute(uint32 attribIndex) const                   { return mInternalAttributes[attribIndex]; }

        MCORE_INLINE AnimGraphObjectData* FindUniqueObjectData(const AnimGraphObject* object) const   { return mUniqueDatas[ object->GetObjectIndex() ]; }
        AnimGraphNodeData* FindUniqueNodeData(const AnimGraphNode* node) const;

        void AddUniqueObjectData();
        void RegisterUniqueObjectData(AnimGraphObjectData* data);

        MCORE_INLINE AnimGraphObjectData* GetUniqueObjectData(uint32 index)                            { return mUniqueDatas[index]; }
        MCORE_INLINE uint32 GetNumUniqueObjectDatas() const                                             { return mUniqueDatas.GetLength(); }
        void SetUniqueObjectData(uint32 index, AnimGraphObjectData* data);
        void RemoveUniqueObjectData(uint32 index, bool delFromMem);
        void RemoveUniqueObjectData(AnimGraphObjectData* uniqueData, bool delFromMem);
        void RemoveAllObjectData(bool delFromMem);

        void RecursivePrepareNodes();

        void Update(float timePassedInSeconds);
        void OutputEvents();

        /**
         * Set if we want to automatically unregister the anim graph instance from the anim graph manager when we delete the anim graph instance.
         * On default this is set to true.
         * @param enabled Set to true when you wish to automatically have the anim graph instance unregistered, otherwise set it to false.
         */
        void SetAutoUnregisterEnabled(bool enabled);

        /**
         * Check if the anim graph instance is automatically being unregistered from the anim graph manager when this anim graph instance gets deleted or not.
         * @result Returns true when it will get automatically deleted, otherwise false is returned.
         */
        bool GetAutoUnregisterEnabled() const;

        /**
         * Marks the actor as used by the engine runtime, as opposed to the tool suite.
         */
        void SetIsOwnedByRuntime(bool isOwnedByRuntime);
        bool GetIsOwnedByRuntime() const;

        ActorInstance* FindActorInstanceFromParentDepth(uint32 parentDepth) const;

        void SetVisualizeScale(float scale);
        float GetVisualizeScale() const;

        void SetVisualizationEnabled(bool enabled);
        bool GetVisualizationEnabled() const;

        bool GetRetargetingEnabled() const;
        void SetRetargetingEnabled(bool enabled);

        bool GetIsReference() const;
        const ReferenceInfo& GetReferenceInfo() const;

        AnimGraphNode* GetRootNode() const;

        //-----------------------------------------------------------------------------------------------------------------

        /**
         * Add event handler to the anim graph instance.
         * @param eventHandler The new event handler to register, this must not be nullptr.
         */
        void AddEventHandler(AnimGraphInstanceEventHandler* eventHandler);

        /**
         * Find the index for the given event handler.
         * @param[in] eventHandler A pointer to the event handler to search.
         * @return The index of the event handler inside the event manager. MCORE_INVALIDINDEX32 in case the event handler has not been found.
         */
        uint32 FindEventHandlerIndex(AnimGraphInstanceEventHandler* eventHandler) const;

        /**
         * Remove the given event handler.
         * @param eventHandler A pointer to the event handler to remove.
         * @param delFromMem When set to true, the event handler will be deleted from memory automatically when removing it.
         */
        bool RemoveEventHandler(AnimGraphInstanceEventHandler* eventHandler, bool delFromMem = true);

        /**
         * Remove the event handler at the given index.
         * @param index The index of the event handler to remove.
         * @param delFromMem When set to true, the event handler will be deleted from memory automatically when removing it.
         */
        void RemoveEventHandler(uint32 index, bool delFromMem = true);

        /**
         * Remove all event handlers.
         * @param delFromMem When set to true, the event handlers will be deleted from memory automatically when removing it.
         */
        void RemoveAllEventHandlers(bool delFromMem = true);

        void OnStateEnter(AnimGraphNode* state);
        void OnStateEntering(AnimGraphNode* state);
        void OnStateExit(AnimGraphNode* state);
        void OnStateEnd(AnimGraphNode* state);
        void OnStartTransition(AnimGraphStateTransition* transition);
        void OnEndTransition(AnimGraphStateTransition* transition);

        void CollectActiveAnimGraphNodes(MCore::Array<AnimGraphNode*>* outNodes, uint32 nodeTypeID = MCORE_INVALIDINDEX32); // MCORE_INVALIDINDEX32 means all node types

        MCORE_INLINE uint32 GetObjectFlags(uint32 objectIndex) const                                            { return mObjectFlags[objectIndex]; }
        MCORE_INLINE void SetObjectFlags(uint32 objectIndex, uint32 flags)                                      { mObjectFlags[objectIndex] = flags; }
        MCORE_INLINE void EnableObjectFlags(uint32 objectIndex, uint32 flagsToEnable)                           { mObjectFlags[objectIndex] |= flagsToEnable; }
        MCORE_INLINE void DisableObjectFlags(uint32 objectIndex, uint32 flagsToDisable)                         { mObjectFlags[objectIndex] &= ~flagsToDisable; }
        MCORE_INLINE void SetObjectFlags(uint32 objectIndex, uint32 flags, bool enabled)
        {
            if (enabled)
            {
                mObjectFlags[objectIndex] |= flags;
            }
            else
            {
                mObjectFlags[objectIndex] &= ~flags;
            }
        }
        MCORE_INLINE bool GetIsObjectFlagEnabled(uint32 objectIndex, uint32 flag) const                         { return (mObjectFlags[objectIndex] & flag) != 0; }

        MCORE_INLINE bool GetIsOutputReady(uint32 objectIndex) const                                            { return (mObjectFlags[objectIndex] & OBJECTFLAGS_OUTPUT_READY) != 0; }
        MCORE_INLINE void SetIsOutputReady(uint32 objectIndex, bool isReady)                                    { SetObjectFlags(objectIndex, OBJECTFLAGS_OUTPUT_READY, isReady); }

        MCORE_INLINE bool GetIsSynced(uint32 objectIndex) const                                                 { return (mObjectFlags[objectIndex] & OBJECTFLAGS_SYNCED) != 0; }
        MCORE_INLINE void SetIsSynced(uint32 objectIndex, bool isSynced)                                        { SetObjectFlags(objectIndex, OBJECTFLAGS_SYNCED, isSynced); }

        MCORE_INLINE bool GetIsResynced(uint32 objectIndex) const                                               { return (mObjectFlags[objectIndex] & OBJECTFLAGS_RESYNC) != 0; }
        MCORE_INLINE void SetIsResynced(uint32 objectIndex, bool isResynced)                                    { SetObjectFlags(objectIndex, OBJECTFLAGS_RESYNC, isResynced); }

        MCORE_INLINE bool GetIsUpdateReady(uint32 objectIndex) const                                            { return (mObjectFlags[objectIndex] & OBJECTFLAGS_UPDATE_READY) != 0; }
        MCORE_INLINE void SetIsUpdateReady(uint32 objectIndex, bool isReady)                                    { SetObjectFlags(objectIndex, OBJECTFLAGS_UPDATE_READY, isReady); }

        MCORE_INLINE bool GetIsTopDownUpdateReady(uint32 objectIndex) const                                     { return (mObjectFlags[objectIndex] & OBJECTFLAGS_TOPDOWNUPDATE_READY) != 0; }
        MCORE_INLINE void SetIsTopDownUpdateReady(uint32 objectIndex, bool isReady)                             { SetObjectFlags(objectIndex, OBJECTFLAGS_TOPDOWNUPDATE_READY, isReady); }

        MCORE_INLINE bool GetIsPostUpdateReady(uint32 objectIndex) const                                        { return (mObjectFlags[objectIndex] & OBJECTFLAGS_POSTUPDATE_READY) != 0; }
        MCORE_INLINE void SetIsPostUpdateReady(uint32 objectIndex, bool isReady)                                { SetObjectFlags(objectIndex, OBJECTFLAGS_POSTUPDATE_READY, isReady); }

        const InitSettings& GetInitSettings() const;
        const AnimGraphEventBuffer& GetEventBuffer() const;

    private:
        AnimGraph*                                         mAnimGraph;
        ActorInstance*                                      mActorInstance;         //
        MCore::Array<MCore::Attribute*>                     mParamValues;           // a value for each AnimGraph parameter (the control parameters)
        MCore::Array<AnimGraphObjectData*>                 mUniqueDatas;           // unique object data
        MCore::Array<uint32>                                mObjectFlags;           // the object flags
        MCore::Array<AnimGraphInstanceEventHandler*>       mEventHandlers;         /**< The event handlers to use to process events. */
        MCore::Array<MCore::Attribute*>                     mInternalAttributes;
        MotionSet*                                          mMotionSet;             // the used motion set
        MCore::Mutex                                        mMutex;
        ReferenceInfo                                       mReferenceInfo;
        InitSettings                                        mInitSettings;
        AnimGraphEventBuffer                               mEventBuffer;           /**< The event buffer of the last update. */
        float                                               mVisualizeScale;
        bool                                                mAutoUnregister;        /**< Specifies whether we will automatically unregister this anim graph instance set from the anim graph manager or not, when deleting this object. */
        bool                                                mEnableVisualization;
        bool                                                mRetarget;              /**< Is retargeting enabled? */

#if defined(EMFX_DEVELOPMENT_BUILD)
        bool                                                mIsOwnedByRuntime;
#endif // EMFX_DEVELOPMENT_BUILD

        AnimGraphInstance(AnimGraph* animGraph, ActorInstance* actorInstance, MotionSet* motionSet, const InitSettings* initSettings = nullptr, ReferenceInfo* referenceInfo = nullptr);
        ~AnimGraphInstance();
        void RecursiveSwitchToEntryState(AnimGraphNode* node);
        void RecursiveResetCurrentState(AnimGraphNode* node);
        void RecursivePrepareNode(AnimGraphNode* node);
        void InitUniqueDatas();
    };
}   // namespace EMotionFX
