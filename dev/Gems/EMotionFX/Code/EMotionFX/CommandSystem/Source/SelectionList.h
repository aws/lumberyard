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

#include "CommandSystemConfig.h"
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/MorphTarget.h>


namespace CommandSystem
{
    /**
     * A selection list stores links to objects which are selected at a
     * specific time stamp in a scene.
     */
    class COMMANDSYSTEM_API SelectionList
    {
        MCORE_MEMORYOBJECTCATEGORY(SelectionList, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_COMMANDSYSTEM);

    public:
        /**
         * The default constructor.
         * Creates an empty selection list.
         */
        SelectionList();

        /**
         * The destructor.
         */
        ~SelectionList();

        /**
         * Get the number of selected nodes.
         * @return The number of selected nodes.
         */
        MCORE_INLINE uint32 GetNumSelectedNodes() const                             { return mSelectedNodes.GetLength(); }

        /**
         * Get the number of selected actors
         */
        MCORE_INLINE uint32 GetNumSelectedActors() const                            { return mSelectedActors.GetLength(); }

        /**
         * Get the number of selected actor instances.
         * @return The number of selected actor instances.
         */
        MCORE_INLINE uint32 GetNumSelectedActorInstances() const                    { return mSelectedActorInstances.GetLength(); }

        /**
         * Get the number of selected motion instances.
         * @return The number of selected motion instances.
         */
        MCORE_INLINE uint32 GetNumSelectedMotionInstances() const                   { return mSelectedMotionInstances.GetLength(); }

        /**
         * Get the number of selected motions.
         * @return The number of selected motions.
         */
        MCORE_INLINE uint32 GetNumSelectedMotions() const                           { return mSelectedMotions.GetLength(); }

        /**
         * Get the number of selected anim graphs.
         * @return The number of selected anim graphs.
         */
        MCORE_INLINE uint32 GetNumSelectedAnimGraphs() const                       { return mSelectedAnimGraphs.GetLength(); }

        /**
         * Get the number of selected morph targets.
         * @return The number of selected morph targets.
         */
        MCORE_INLINE uint32 GetNumSelectedMorphTargets() const                     { return mSelectedMorphTargets.GetLength(); }

        /**
         * Get the total number of selected objects.
         * @return The number of selected nodes, actors and motions.
         */
        MCORE_INLINE uint32 GetNumTotalItems() const                                { return mSelectedNodes.GetLength() + mSelectedActors.GetLength() + mSelectedActorInstances.GetLength() + mSelectedMotions.GetLength() + mSelectedMotionInstances.GetLength() + mSelectedAnimGraphs.GetLength(); }

        /**
         * Check whether or not the selection list contains any objects.
         * @return True if the selection list is empty / if there are no selected objects, false if not.
         */
        MCORE_INLINE bool GetIsEmpty() const
        {
            if (mSelectedNodes.GetIsEmpty() && mSelectedActors.GetIsEmpty() && mSelectedActorInstances.GetIsEmpty() && mSelectedMotions.GetIsEmpty() && mSelectedMotionInstances.GetIsEmpty() && mSelectedAnimGraphs.GetIsEmpty() && mSelectedMorphTargets.GetIsEmpty())
            {
                return true;
            }
            else
            {
                return false;
            }
        }

        /**
         * Clear the selection list. This will unselect all objects.
         */
        MCORE_INLINE void Clear()                                                   { mSelectedNodes.Clear(); mSelectedActors.Clear(); mSelectedActorInstances.Clear(); mSelectedMotions.Clear(); mSelectedMotionInstances.Clear(); mSelectedAnimGraphs.Clear(); mSelectedMorphTargets.Clear(); }

        /**
         * Add a node to the selection list. Select node without replacing the old selection.
         * @param node The node which will be added to the selection.
         */
        void AddNode(EMotionFX::Node* node);

        /**
         * Add actor
         */
        void AddActor(EMotionFX::Actor* actor);

        /**
         * Add an actor instance to the selection list. Select actor instance without replacing the old selection.
         * @param actorInstance The actor which will be added to the selection.
         */
        void AddActorInstance(EMotionFX::ActorInstance* actorInstance);

        /**
         * Add a motion to the selection list. Select motion without replacing the old selection.
         * @param motion The motion which will be added to the selection.
         */
        void AddMotion(EMotionFX::Motion* motion);

        /**
         * Add a motion instance to the selection list. Select motion instance without replacing the old selection.
         * @param motion The motion instance which will be added to the selection.
         */
        void AddMotionInstance(EMotionFX::MotionInstance* motionInstance);

        /**
         * Add an anim graph to the selection list. Selects anim graph without replacing the old selection.
         * @param animGraph The anim graph which will be added to the selection.
         */
        void AddAnimGraph(EMotionFX::AnimGraph* animGraph);

        /**
         * Add a morph target to the selection list. Selects the morph target without replacing the old selection.
         * @param morphTarget The morph target which will be added to the selection.
         */
        void AddMorphTarget(EMotionFX::MorphTarget* morphTarget);

        /**
         * Add a complete selection list to this one. Select motionobjects without replacing the old selection.
         * @param selection The selection list which will be added to the selection.
         */
        void Add(SelectionList& selection);

        /**
         * Get the given node from the selection list.
         * @param index The index of the node to get from the selection list.
         * @return A pointer to the given node from the selection list.
         */
        MCORE_INLINE EMotionFX::Node* GetNode(uint32 index) const                               { return mSelectedNodes[index]; }

        /**
         * Get the first node from the selection list.
         * @return A pointer to the first node from the selection list. nullptr is there is no node selected.
         */
        MCORE_INLINE EMotionFX::Node* GetFirstNode() const
        {
            if (mSelectedNodes.GetIsEmpty())
            {
                return nullptr;
            }
            return mSelectedNodes[0];
        }

        /**
         * Get the given actor from the selection list.
         * @param index The index of the actor to get from the selection list.
         * @return A pointer to the given actor from the selection list.
         */
        MCORE_INLINE EMotionFX::Actor* GetActor(uint32 index) const                             { return mSelectedActors[index]; }

        /**
         * Get the first actor from the selection list.
         * @return A pointer to the first actor from the selection list. nullptr is there is no actor selected.
         */
        MCORE_INLINE EMotionFX::Actor* GetFirstActor() const
        {
            if (mSelectedActors.GetIsEmpty())
            {
                return nullptr;
            }
            return mSelectedActors[0];
        }

        /**
         * Get the given actor instance from the selection list.
         * @param index The index of the actor instance to get from the selection list.
         * @return A pointer to the given actor instance from the selection list.
         */
        MCORE_INLINE EMotionFX::ActorInstance* GetActorInstance(uint32 index) const             { return mSelectedActorInstances[index]; }

        /**
         * Get the first actor instance from the selection list.
         * @return A pointer to the first actor instance from the selection list. nullptr if there is no actor instance selected.
         */
        MCORE_INLINE EMotionFX::ActorInstance* GetFirstActorInstance() const
        {
            if (mSelectedActorInstances.GetIsEmpty())
            {
                return nullptr;
            }
            return mSelectedActorInstances[0];
        }

        /**
         * Get the given anim graph from the selection list.
         * @param index The index of the anim graph to get from the selection list.
         * @return A pointer to the given anim graph from the selection list.
         */
        MCORE_INLINE EMotionFX::AnimGraph* GetAnimGraph(uint32 index) const                   { return mSelectedAnimGraphs[index]; }

        /**
         * Get the first anim graph from the selection list.
         * @return A pointer to the first anim graph from the selection list. nullptr if there is no anim graph selected.
         */
        MCORE_INLINE EMotionFX::AnimGraph* GetFirstAnimGraph() const
        {
            if (mSelectedAnimGraphs.GetIsEmpty())
            {
                return nullptr;
            }
            return mSelectedAnimGraphs[0];
        }


        MCORE_INLINE EMotionFX::MorphTarget* GetMorphTarget(uint32 index) const              { return mSelectedMorphTargets[index]; }
        MCORE_INLINE EMotionFX::MorphTarget* GetFirstMorphTarget() const
        {
            if (mSelectedMorphTargets.GetIsEmpty())
            {
                return nullptr;
            }
            return mSelectedMorphTargets[0];
        }


        /**
         * Get the single selected actor.
         * @return A pointer to the only selected actor. nullptr is there if no actor selected at all or if there are multiple, so more than one, actor instances selected.
         */
        EMotionFX::Actor* GetSingleActor() const;

        /**
         * Get the single selected actor instance.
         * @return A pointer to the only selected actor instance. nullptr is there if no actor instance selected at all or if there are multiple, so more than one, actor instances selected.
         */
        EMotionFX::ActorInstance* GetSingleActorInstance() const;

        /**
         * Get the given motion from the selection list.
         * @param index The index of the motion to get from the selection list.
         * @return A pointer to the given motion from the selection list.
         */
        MCORE_INLINE EMotionFX::Motion* GetMotion(uint32 index) const                           { return mSelectedMotions[index]; }

        /**
         * Get the first motion from the selection list.
         * @return A pointer to the first motion from the selection list. nullptr is there is no motion selected.
         */
        MCORE_INLINE EMotionFX::Motion* GetFirstMotion() const
        {
            if (mSelectedMotions.GetIsEmpty())
            {
                return nullptr;
            }
            return mSelectedMotions[0];
        }

        /**
         * Get the single selected motion.
         * @return A pointer to the only selected motion. nullptr is there if no motion selected at all or if there are multiple, so more than one, motions selected.
         */
        EMotionFX::Motion* GetSingleMotion() const;

        /**
         * Get the given motion instance from the selection list.
         * @param index The index of the motion instance to get from the selection list.
         * @return A pointer to the given motion instance from the selection list.
         */
        MCORE_INLINE EMotionFX::MotionInstance* GetMotionInstance(uint32 index) const           { return mSelectedMotionInstances[index]; }

        /**
         * Get the first motion instance from the selection list.
         * @return A pointer to the first motion instance from the selection list. nullptr is there is no motion instance selected.
         */
        MCORE_INLINE EMotionFX::MotionInstance* GetFirstMotionInstance() const
        {
            if (mSelectedMotionInstances.GetIsEmpty())
            {
                return nullptr;
            }
            return mSelectedMotionInstances[0];
        }

        /**
         * Remove the given node from the selection list.
         * @param index The index of the node to be removed from the selection list.
         */
        MCORE_INLINE void RemoveNode(uint32 index)                                              { mSelectedNodes.Remove(index); }

        /**
         * Remove the given actor instance from the selection list.
         * @param index The index of the actor instance to be removed from the selection list.
         */
        MCORE_INLINE void RemoveActor(uint32 index)                                             { mSelectedActors.Remove(index); }

        /**
         * Remove the given actor instance from the selection list.
         * @param index The index of the actor instance to be removed from the selection list.
         */
        MCORE_INLINE void RemoveActorInstance(uint32 index)                                     { mSelectedActorInstances.Remove(index); }

        /**
         * Remove the given motion from the selection list.
         * @param index The index of the motion to be removed from the selection list.
         */
        MCORE_INLINE void RemoveMotion(uint32 index)                                            { mSelectedMotions.Remove(index); }

        /**
         * Remove the given motion instance from the selection list.
         * @param index The index of the motion instance to be removed from the selection list.
         */
        MCORE_INLINE void RemoveMotionInstance(uint32 index)                                    { mSelectedMotionInstances.Remove(index); }

        /**
         * Remove the given anim graph from the selection list.
         * @param index The index of the anim graph to remove from the selection list.
         */
        MCORE_INLINE void RemoveAnimGraph(uint32 index)                                        { mSelectedAnimGraphs.Remove(index); }

        /**
         * Remove the given morph target from the selection list.
         * @param index The index of the morph target to remove from the selection list.
         */
        MCORE_INLINE void RemoveMorphTarget(uint32 index)                                      { mSelectedMorphTargets.Remove(index); }

        /**
         * Remove the given node from the selection list.
         * @param node The node to be removed from the selection list.
         */
        void RemoveNode(EMotionFX::Node* node);

        /**
         * Remove the given actor instance from the selection list.
         * @param actorInstance The actor instance to be removed from the selection list.
         */
        void RemoveActor(EMotionFX::Actor* actor);

        /**
         * Remove the given actor instance from the selection list.
         * @param actorInstance The actor instance to be removed from the selection list.
         */
        void RemoveActorInstance(EMotionFX::ActorInstance* actorInstance);

        /**
         * Remove the given motion from the selection list.
         * @param motion The motion to be removed from the selection list.
         */
        void RemoveMotion(EMotionFX::Motion* motion);

        /**
         * Remove the given motion instance from the selection list.
         * @param motionInstance The motion instance to be removed from the selection list.
         */
        void RemoveMotionInstance(EMotionFX::MotionInstance* motionInstance);

        /**
         * Remove the given anim graph from the selection list.
         * @param animGraph The anim graph to be removed from the list.
         */
        void RemoveAnimGraph(EMotionFX::AnimGraph* animGraph);

        /**
         * Remove the given morph target from the selection list.
         * @param morphTarget The morph target to be removed from the list.
         */
        void RemoveMorphTarget(EMotionFX::MorphTarget* morphTarget);

        /**
         * Check if a given node is selected / is in this selection list.
         * @param node A pointer to the node to be checked.
         * @return True if the node is inside this selection list, false if not.
         */
        MCORE_INLINE bool CheckIfHasNode(EMotionFX::Node* node) const                                   { return (mSelectedNodes.Find(node) != MCORE_INVALIDINDEX32); }

        /**
         * Has actor
         */
        MCORE_INLINE bool CheckIfHasActor(EMotionFX::Actor* actor) const                                { return (mSelectedActors.Find(actor) != MCORE_INVALIDINDEX32); }

        /**
         * Check if a given actor instance is selected / is in this selection list.
         * @param node A pointer to the actor instance to be checked.
         * @return True if the actor instance is inside this selection list, false if not.
         */
        MCORE_INLINE bool CheckIfHasActorInstance(EMotionFX::ActorInstance* actorInstance) const        { return (mSelectedActorInstances.Find(actorInstance) != MCORE_INVALIDINDEX32); }

        /**
         * Check if a given motion is selected / is in this selection list.
         * @param node A pointer to the motion to be checked.
         * @return True if the motion is inside this selection list, false if not.
         */
        MCORE_INLINE bool CheckIfHasMotion(EMotionFX::Motion* motion) const                             { return (mSelectedMotions.Find(motion) != MCORE_INVALIDINDEX32); }

        /**
         * Check if a given anim graph is in this selection list.
         * @param animGraph The anim graph to check for.
         * @return True if the anim graph is selected inside the selection list, otherwise false is returned.
         */
        MCORE_INLINE bool CheckIfHasAnimGraph(EMotionFX::AnimGraph* animGraph) const                 { return (mSelectedAnimGraphs.Find(animGraph) != MCORE_INVALIDINDEX32); }

        /**
         * Check if a given motion instance is selected / is in this selection list.
         * @param node A pointer to the motion instance to be checked.
         * @return True if the motion instance is inside this selection list, false if not.
         */
        MCORE_INLINE bool CheckIfHasMotionInstance(EMotionFX::MotionInstance* motionInstance) const     { return (mSelectedMotionInstances.Find(motionInstance) != MCORE_INVALIDINDEX32); }

        MCORE_INLINE bool CheckIfHasMorphTarget(EMotionFX::MorphTarget* morphTarget) const              { return (mSelectedMorphTargets.Find(morphTarget) != MCORE_INVALIDINDEX32); }

        MCORE_INLINE void ClearActorSelection()                                     { mSelectedActors.Clear(); }
        MCORE_INLINE void ClearActorInstanceSelection()                             { mSelectedActorInstances.Clear(); }
        MCORE_INLINE void ClearNodeSelection()                                      { mSelectedNodes.Clear(); }
        MCORE_INLINE void ClearMotionSelection()                                    { mSelectedMotions.Clear(); }
        MCORE_INLINE void ClearMotionInstanceSelection()                            { mSelectedMotionInstances.Clear(); }
        MCORE_INLINE void ClearAnimGraphSelection()                                 { mSelectedAnimGraphs.Clear(); }
        MCORE_INLINE void ClearMorphTargets()                                       { mSelectedMorphTargets.Clear(); }

        void Log();
        void MakeValid();

    private:
        MCore::Array<EMotionFX::Node*>              mSelectedNodes;             /**< Array of selected nodes. */
        MCore::Array<EMotionFX::Actor*>             mSelectedActors;            /**< The selected actors.  */
        MCore::Array<EMotionFX::ActorInstance*>     mSelectedActorInstances;    /**< Array of selected actor instances. */
        MCore::Array<EMotionFX::MotionInstance*>    mSelectedMotionInstances;   /**< Array of selected motion instances. */
        MCore::Array<EMotionFX::Motion*>            mSelectedMotions;           /**< Array of selected motions. */
        MCore::Array<EMotionFX::AnimGraph*>         mSelectedAnimGraphs;        /**< Array of selected anim graphs. */
        MCore::Array<EMotionFX::MorphTarget*>       mSelectedMorphTargets;      /**< Array of selected morph targets. */
    };
} // namespace CommandSystem