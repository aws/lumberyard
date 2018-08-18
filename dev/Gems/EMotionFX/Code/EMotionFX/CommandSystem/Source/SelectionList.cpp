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
#include "SelectionList.h"
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MorphTarget.h>


namespace CommandSystem
{
    // constructor
    SelectionList::SelectionList()
    {
        mSelectedNodes.SetMemoryCategory(MEMCATEGORY_COMMANDSYSTEM);
        mSelectedActors.SetMemoryCategory(MEMCATEGORY_COMMANDSYSTEM);
        mSelectedActorInstances.SetMemoryCategory(MEMCATEGORY_COMMANDSYSTEM);
        mSelectedMotionInstances.SetMemoryCategory(MEMCATEGORY_COMMANDSYSTEM);
        mSelectedMotions.SetMemoryCategory(MEMCATEGORY_COMMANDSYSTEM);
        mSelectedAnimGraphs.SetMemoryCategory(MEMCATEGORY_COMMANDSYSTEM);
        mSelectedMorphTargets.SetMemoryCategory(MEMCATEGORY_COMMANDSYSTEM);
    }


    // destructor
    SelectionList::~SelectionList()
    {
    }


    // add a node to the selection list
    void SelectionList::AddNode(EMotionFX::Node* node)
    {
        if (CheckIfHasNode(node) == false)
        {
            mSelectedNodes.Add(node);
        }
    }


    // add actor
    void SelectionList::AddActor(EMotionFX::Actor* actor)
    {
        if (CheckIfHasActor(actor) == false)
        {
            mSelectedActors.Add(actor);
        }
    }


    // add an actor instance to the selection list
    void SelectionList::AddActorInstance(EMotionFX::ActorInstance* actorInstance)
    {
        if (CheckIfHasActorInstance(actorInstance) == false)
        {
            mSelectedActorInstances.Add(actorInstance);
        }
    }


    // add a motion to the selection list
    void SelectionList::AddMotion(EMotionFX::Motion* motion)
    {
        if (CheckIfHasMotion(motion) == false)
        {
            mSelectedMotions.Add(motion);
        }
    }


    // add a motion instance to the selection list
    void SelectionList::AddMotionInstance(EMotionFX::MotionInstance* motionInstance)
    {
        if (CheckIfHasMotionInstance(motionInstance) == false)
        {
            mSelectedMotionInstances.Add(motionInstance);
        }
    }


    // add a anim graph to the selection list
    void SelectionList::AddAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        if (CheckIfHasAnimGraph(animGraph) == false)
        {
            mSelectedAnimGraphs.Add(animGraph);
        }
    }

    void SelectionList::AddMorphTarget(EMotionFX::MorphTarget* morphTarget)
    {
        if (!CheckIfHasMorphTarget(morphTarget))
        {
            mSelectedMorphTargets.Add(morphTarget);
        }
    }

    // add a complete selection list to this one
    void SelectionList::Add(SelectionList& selection)
    {
        uint32 i;

        // get the number of selected objects
        const uint32 numSelectedNodes           = selection.GetNumSelectedNodes();
        const uint32 numSelectedActors          = selection.GetNumSelectedActors();
        const uint32 numSelectedActorInstances  = selection.GetNumSelectedActorInstances();
        const uint32 numSelectedMotions         = selection.GetNumSelectedMotions();
        const uint32 numSelectedMotionInstances = selection.GetNumSelectedMotionInstances();
        const uint32 numSelectedAnimGraphs      = selection.GetNumSelectedAnimGraphs();
        const uint32 numSelectedMorphTargets    = selection.GetNumSelectedMorphTargets();

        // iterate through all nodes and select them
        for (i = 0; i < numSelectedNodes; ++i)
        {
            AddNode(selection.GetNode(i));
        }

        // iterate through all actors and select them
        for (i = 0; i < numSelectedActors; ++i)
        {
            AddActor(selection.GetActor(i));
        }

        // iterate through all actor instances and select them
        for (i = 0; i < numSelectedActorInstances; ++i)
        {
            AddActorInstance(selection.GetActorInstance(i));
        }

        // iterate through all motions and select them
        for (i = 0; i < numSelectedMotions; ++i)
        {
            AddMotion(selection.GetMotion(i));
        }

        // iterate through all motion instances and select them
        for (i = 0; i < numSelectedMotionInstances; ++i)
        {
            AddMotionInstance(selection.GetMotionInstance(i));
        }

        // iterate through all anim graphs and select them
        for (i = 0; i < numSelectedAnimGraphs; ++i)
        {
            AddAnimGraph(selection.GetAnimGraph(i));
        }

        for (i = 0; i < numSelectedMorphTargets; ++i)
        {
            AddMorphTarget(selection.GetMorphTarget(i));
        }
    }


    // log the current selection
    void SelectionList::Log()
    {
        uint32 i;

        // get the number of selected objects
        const uint32 numSelectedNodes           = GetNumSelectedNodes();
        const uint32 numSelectedActorInstances  = GetNumSelectedActorInstances();
        const uint32 numSelectedActors          = GetNumSelectedActors();
        const uint32 numSelectedMotions         = GetNumSelectedMotions();
        const uint32 numSelectedMotionInstances = GetNumSelectedMotionInstances();
        const uint32 numSelectedAnimGraphs      = GetNumSelectedAnimGraphs();
        const uint32 numSelectedMorphTargets    = GetNumSelectedMorphTargets();

        MCore::LogInfo("SelectionList:");

        // iterate through all nodes and select them
        MCore::LogInfo(" - Nodes (%i)", numSelectedNodes);
        for (i = 0; i < numSelectedNodes; ++i)
        {
            MCore::LogInfo("    + Node #%.3d: name='%s'", i, GetNode(i)->GetName());
        }

        // iterate through all actors and select them
        MCore::LogInfo(" - Actors (%i)", numSelectedActors);
        for (i = 0; i < numSelectedActors; ++i)
        {
            MCore::LogInfo("    + Actor #%.3d: name='%s'", i, GetActor(i)->GetName());
        }

        // iterate through all actor instances and select them
        MCore::LogInfo(" - Actor instances (%i)", numSelectedActorInstances);
        for (i = 0; i < numSelectedActorInstances; ++i)
        {
            MCore::LogInfo("    + Actor instance #%.3d: name='%s'", i, GetActorInstance(i)->GetActor()->GetName());
        }

        // iterate through all motions and select them
        MCore::LogInfo(" - Motions (%i)", numSelectedMotions);
        for (i = 0; i < numSelectedMotions; ++i)
        {
            MCore::LogInfo("    + Motion #%.3d: name='%s'", i, GetMotion(i)->GetName());
        }

        // iterate through all motion instances and select them
        MCore::LogInfo(" - Motion instances (%i)", numSelectedMotionInstances);
        //for (i=0; i<numSelectedMotionInstances; ++i)

        // iterate through all motions and select them
        MCore::LogInfo(" - AnimGraphs (%i)", numSelectedAnimGraphs);
        for (i = 0; i < numSelectedAnimGraphs; ++i)
        {
            MCore::LogInfo("    + AnimGraph #%.3d: %s", i, GetAnimGraph(i)->GetFileName());
        }

        // iterate through all morph targets and select them
        MCore::LogInfo(" - MorphTargets (%i)", numSelectedMorphTargets);
        for (i = 0; i < numSelectedMorphTargets; ++i)
        {
            MCore::LogInfo("    + MorphTarget #%.3d: name='%s'", i, GetMorphTarget(i)->GetName());
        }
    }


    // remove the node from the selection list
    void SelectionList::RemoveNode(EMotionFX::Node* node)
    {
        const uint32 index = mSelectedNodes.Find(node);

        if (index != MCORE_INVALIDINDEX32)
        {
            mSelectedNodes.Remove(index);
        }
    }


    // remove the actor from the selection list
    void SelectionList::RemoveActor(EMotionFX::Actor* actor)
    {
        const uint32 index = mSelectedActors.Find(actor);

        if (index != MCORE_INVALIDINDEX32)
        {
            mSelectedActors.Remove(index);
        }
    }


    // remove the actor from the selection list
    void SelectionList::RemoveActorInstance(EMotionFX::ActorInstance* actorInstance)
    {
        const uint32 index = mSelectedActorInstances.Find(actorInstance);

        if (index != MCORE_INVALIDINDEX32)
        {
            mSelectedActorInstances.Remove(index);
        }
    }


    // remove the motion from the selection list
    void SelectionList::RemoveMotion(EMotionFX::Motion* motion)
    {
        const uint32 index = mSelectedMotions.Find(motion);

        if (index != MCORE_INVALIDINDEX32)
        {
            mSelectedMotions.Remove(index);
        }
    }


    // remove the motion instance from the selection list
    void SelectionList::RemoveMotionInstance(EMotionFX::MotionInstance* motionInstance)
    {
        const uint32 index = mSelectedMotionInstances.Find(motionInstance);

        if (index != MCORE_INVALIDINDEX32)
        {
            mSelectedMotionInstances.Remove(index);
        }
    }


    // remove the anim graph
    void SelectionList::RemoveAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        const uint32 index = mSelectedAnimGraphs.Find(animGraph);
        if (index != MCORE_INVALIDINDEX32)
        {
            mSelectedAnimGraphs.Remove(index);
        }
    }


    void SelectionList::RemoveMorphTarget(EMotionFX::MorphTarget* morphTarget)
    {
        const uint32 index = mSelectedMorphTargets.Find(morphTarget);
        if (index != MCORE_INVALIDINDEX32)
        {
            mSelectedMorphTargets.Remove(index);
        }
    }


    // make the selection valid
    void SelectionList::MakeValid()
    {
        uint32 i;

        // iterate through all nodes and remove the invalid ones
        //for (i=0; i<numSelectedNodes; ++i)

        // iterate through all actors and remove the invalid ones
        for (i = 0; i < mSelectedActors.GetLength();)
        {
            EMotionFX::Actor* actor = mSelectedActors[i];
            if (EMotionFX::GetActorManager().FindActorIndex(actor) == MCORE_INVALIDINDEX32)
            {
                mSelectedActors.Remove(i);
            }
            else
            {
                i++;
            }
        }

        // iterate through all actor instances and remove the invalid ones
        for (i = 0; i < mSelectedActorInstances.GetLength();)
        {
            EMotionFX::ActorInstance* actorInstance = mSelectedActorInstances[i];

            if (EMotionFX::GetActorManager().CheckIfIsActorInstanceRegistered(actorInstance) == false)
            {
                mSelectedActorInstances.Remove(i);
            }
            else
            {
                i++;
            }
        }

        // iterate through all motions and remove the invalid ones
        //  for (i=0; i<numSelectedMotions; ++i)
        // iterate through all motion instances and remove the invalid ones

        // iterate through all anim graphs and remove all valid ones
        for (i = 0; i < mSelectedAnimGraphs.GetLength();)
        {
            EMotionFX::AnimGraph* animGraph = mSelectedAnimGraphs[i];

            if (EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph) == MCORE_INVALIDINDEX32)
            {
                mSelectedAnimGraphs.Remove(i);
            }
            else
            {
                i++;
            }
        }
    }



    // get a single selected actor
    EMotionFX::Actor* SelectionList::GetSingleActor() const
    {
        if (GetNumSelectedActors() == 0)
        {
            return nullptr;
        }

        if (GetNumSelectedActors() > 1)
        {
            //LogDebug("Cannot get single selected actor. Multiple actors selected.");
            return nullptr;
        }

        return mSelectedActors[0];
    }


    // get a single selected actor instance
    EMotionFX::ActorInstance* SelectionList::GetSingleActorInstance() const
    {
        if (GetNumSelectedActorInstances() == 0)
        {
            //LogDebug("Cannot get single selected actor instance. No actor instance selected.");
            return nullptr;
        }

        if (GetNumSelectedActorInstances() > 1)
        {
            //LogDebug("Cannot get single selected actor instance. Multiple actor instances selected.");
            return nullptr;
        }

        EMotionFX::ActorInstance* actorInstance = mSelectedActorInstances[0];
        if (!actorInstance)
        {
            return nullptr;
        }

        if (actorInstance->GetIsOwnedByRuntime())
        {
            return nullptr;
        }

        return mSelectedActorInstances[0];
    }


    // get a single selected motion
    EMotionFX::Motion* SelectionList::GetSingleMotion() const
    {
        if (GetNumSelectedMotions() == 0)
        {
            return nullptr;
        }

        if (GetNumSelectedMotions() > 1)
        {
            return nullptr;
        }

        EMotionFX::Motion* motion = mSelectedMotions[0];
        if (!motion)
        {
            return nullptr;
        }

        if (motion->GetIsOwnedByRuntime())
        {
            return nullptr;
        }

        return motion;
    }
} // namespace CommandSystem
