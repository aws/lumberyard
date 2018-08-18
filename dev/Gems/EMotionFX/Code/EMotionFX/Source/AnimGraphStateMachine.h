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

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include "EMotionFXConfig.h"
#include <MCore/Source/Array.h>
#include "AnimGraphNode.h"
#include "AnimGraphNodeData.h"


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class ActorInstance;
    class AnimGraphStateTransition;

    class EMFX_API AnimGraphStateMachine
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(AnimGraphStateMachine, "{272E90D2-8A18-46AF-AD82-6A8B7EC508ED}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE  = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            void Reset() override;

        public:
            AnimGraphStateTransition*   mTransition;            /**< The transition we're in. */
            AnimGraphNode*              mCurrentState;          /**< The current state. */
            AnimGraphNode*              mTargetState;           /**< The next state we want to move into. */
            AnimGraphNode*              mPreviousState;         /**< The previously used state, so the one used before the current one, the one from which we transitioned into the current one. */
            bool                        mReachedExitState;      /**< True in case the state machine's current state is an exit state, false it not. */
        };

        AnimGraphStateMachine();
        ~AnimGraphStateMachine();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void OnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;
        void RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

        void RecursiveResetUniqueData(AnimGraphInstance* animGraphInstance) override;
        void RecursiveOnUpdateUniqueData(AnimGraphInstance* animGraphInstance) override;
        void RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable = 0xffffffff) override;

        const char* GetPaletteName() const override                     { return "State Machine"; }
        AnimGraphObject::ECategory GetPaletteCategory() const override { return AnimGraphObject::CATEGORY_SOURCES; }
        bool GetCanActAsState() const override                          { return true; }
        bool GetHasVisualGraph() const override                         { return true; }
        bool GetCanHaveChildren() const override                        { return true; }
        bool GetSupportsDisable() const override                        { return true; }
        bool GetSupportsVisualization() const override                  { return true; }
        bool GetHasOutputPose() const override                          { return true; }
        uint32 GetHasChildIndicatorColor() const override               { return MCore::RGBA(64, 98, 247); }

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override             { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void RecursiveCollectObjects(MCore::Array<AnimGraphObject*>& outObjects) const override;

        //----------------------------------------------------------------------------------------------------------------------------

        /**
         * Add the given transition to the state machine.
         * The state machine will take care of destructing the memory of the transition once it gets destructed itself or by using the RemoveTransition() function.
         * @param[in] transition A pointer to the state machine transition to add.
         */
        void AddTransition(AnimGraphStateTransition* transition);

        /**
         * Get the number of transitions inside this state machine. This includes all kinds of transitions, so also wildcard transitions.
         * @result The number of transitions inside the state machine.
         */
        MCORE_INLINE uint32 GetNumTransitions() const                               { return static_cast<uint32>(mTransitions.size()); }

        /**
         * Get a pointer to the state machine transition of the given index.
         * @param[in] index The index of the transition to return.
         * @result A pointer to the state machine transition at the given index.
         */
        MCORE_INLINE AnimGraphStateTransition* GetTransition(uint32 index) const   { return mTransitions[index]; }

        /**
         * Remove the state machine transition at the given index.
         * @param[in] transitionIndex The index of the transition to remove.
         * @param delFromMem Set to true (default) when you wish to also delete the specified transition from memory.
         */
        void RemoveTransition(uint32 transitionIndex, bool delFromMem = true);

        /**
         * Remove all transitions from the state machine and also get rid of the allocated memory. This will automatically be called in the state machine destructor.
         */
        void RemoveAllTransitions();

        void ReserveTransitions(size_t numTransitions);

        /**
         * Find the transition index for the given transition id.
         * @param[in] transitionID The identification number to search for.
         * @result The index of the transition. MCORE_INVALIDINDEX32 in case no transition has been found.
         */
        uint32 FindTransitionIndexByID(uint32 transitionID) const;

        /**
         * Find the transition by the given transition id.
         * @param[in] transitionID The identification number to search for.
         * @result A pointer to the transition. nullptr in case no transition has been found.
         */
        AnimGraphStateTransition* FindTransitionByID(uint32 transitionID) const;

        /**
         * Find the transition index by comparing pointers.
         * @param[in] transition A pointer to the transition to search the index for.
         * @result The index of the transition. MCORE_INVALIDINDEX32 in case no transition has been found.
         */
        uint32 FindTransitionIndex(AnimGraphStateTransition* transition) const;

        /**
         * Check if there is a wildcard transition with the given state as target node. Each state can only have one wildcard transition. The wildcard transition will be used in case there
         * is no other connection between the current state and the given state.
         * @param[in] state A pointer to the state to check.
         * @result True in case the given state has a wildcard transition, false if not.
         */
        bool CheckIfHasWildcardTransition(AnimGraphNode* state) const;

        /**
         * Check if the state machine is transitioning at the moment.
         * @param[in] animGraphInstance The anim graph instance to check.
         * @return True in case the state machine is transitioning at the moment, false in case a state is fully active and blended in.
         */
        bool GetIsTransitioning(AnimGraphInstance* animGraphInstance) const;

        /**
         * Get a pointer to the currently active transition.
         * @param[in] animGraphInstance The anim graph instance to check.
         * @return A pointer to the currently active transition, nullptr in case the state machine is not transitioning at the moment.
         */
        AnimGraphStateTransition* GetActiveTransition(AnimGraphInstance* animGraphInstance) const;

        /**
         *
         *
         *
         *
         */
        AnimGraphStateTransition* FindTransition(AnimGraphInstance* animGraphInstance, AnimGraphNode* currentState, AnimGraphNode* targetState) const;

        uint32 CalcNumIncomingTransitions(AnimGraphNode* state) const;
        uint32 CalcNumOutgoingTransitions(AnimGraphNode* state) const;
        uint32 CalcNumWildcardTransitions(AnimGraphNode* state) const;

        //----------------------------------------------------------------------------------------------------------------------------

        void GetActiveStates(AnimGraphInstance* animGraphInstance, AnimGraphNode** outStateA, AnimGraphNode** outStateB) const;

        /**
         * Get the initial state of the state machine.
         * The start state is usually shown drawn with an arrow "pointing at it from any where".
         * @return A pointer to the start state of the state machine.
         */
        AnimGraphNode* GetEntryState();

        AZ_FORCE_INLINE AnimGraphNodeId GetEntryStateId() const                     { return m_entryStateId; }

        /**
         * Set the initial state of the state machine.
         * The start state is usually shown drawn with an arrow "pointing at it from any where".
         * @param[in] entryState A pointer to the start state of the state machine.
         */
        void SetEntryState(AnimGraphNode* entryState);

        /**
         * Get the currently active state.
         * @param[in] animGraphInstance The anim graph instance to use.
         * @return A pointer to the currently active state. In case the state machine is transitioning this will be the source state.
         */
        AnimGraphNode* GetCurrentState(AnimGraphInstance* animGraphInstance);

        /**
         * Check if the state machine has reached an exit state.
         * @param[in] animGraphInstance The anim graph instance to check.
         * @return True in case the state machine has reached an exit state, false in case not.
         */
        bool GetExitStateReached(AnimGraphInstance* animGraphInstance) const;

        /**
         *
         *
         *
         *
         */
        void SwitchToState(AnimGraphInstance* animGraphInstance, AnimGraphNode* targetState);
        void TransitionToState(AnimGraphInstance* animGraphInstance, AnimGraphNode* targetState);

        void RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled) override;
        void RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, MCore::Array<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType) const override;

        //----------------------------------------------------------------------------------------------------------------------------

        /**
         * Get the size in bytes of the custom data which will be saved with the object.
         * @return The size in bytes of the custom data.
         */
        uint32 GetCustomDataSize() const override;

        /**
         * Read the custom data which got saved with the object.
         * @param[in] stream A pointer to a stream from which the custom data will be read from.
         * @param[in] endianType The endian type in which the custom data should be saved in.
         */
        bool ReadCustomData(MCore::Stream* stream, uint32 version, MCore::Endian::EEndianType endianType) override;

        void SetAlwaysStartInEntryState(bool alwaysStartInEntryState);
        void SetEntryStateId(AnimGraphNodeId entryStateNodeId);

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<AnimGraphStateTransition*>    mTransitions;
        AnimGraphNode*                              mEntryState;                /**< A pointer to the initial state, so the state where the machine starts. */
        uint32                                      mEntryStateNodeNr;          /**< Used only in the legacy file format. Remove after the legacy file format will be removed. */
        AZ::u64                                     m_entryStateId;             /**< The node id of the entry state. */
        bool                                        m_alwaysStartInEntryState;

        /**
         * Reset all conditions from wild card and outgoing transitions of the given state.
         * @param[in] animGraphInstance A pointer to the anim graph instance.
         * @param[in] state All conditions of transitions starting from this node will get reset.
         */
        void ResetOutgoingTransitionConditions(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);

        void UpdateExitStateReachedFlag(UniqueData* uniqueData);
        void StartTransition(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, AnimGraphStateTransition* transition);
        void CheckConditions(AnimGraphNode* sourceNode, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, bool allowTransition = true);
        void UpdateConditions(AnimGraphInstance* animGraphInstance, AnimGraphNode* animGraphNode, UniqueData* uniqueData, float timePassedInSeconds);
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX
