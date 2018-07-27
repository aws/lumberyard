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
#include "GraphNodeFactory.h"
#include "GraphNode.h"
#include <MCore/Source/LogManager.h>


namespace EMStudio
{
    //--------------------------------------------------------------------------------------
    // class GraphNodeCreator
    //--------------------------------------------------------------------------------------

    // constructor
    GraphNodeCreator::GraphNodeCreator()
    {
    }


    // destructor
    GraphNodeCreator::~GraphNodeCreator()
    {
    }


    // create the node
    GraphNode* GraphNodeCreator::CreateGraphNode(const char* name)
    {
        return new GraphNode(name);
    }


    // create the attribute widget
    QWidget* GraphNodeCreator::CreateAttributeWidget()
    {
        return nullptr; // nullptr means it will auto generate it based on its parameters
    }



    //--------------------------------------------------------------------------------------
    // class GraphNodeFactory
    //--------------------------------------------------------------------------------------

    // constructor
    GraphNodeFactory::GraphNodeFactory()
    {
        // pre-alloc some space
        mCreators.Reserve(20);
        mCreators.SetMemoryCategory(MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);
    }


    // destructor
    GraphNodeFactory::~GraphNodeFactory()
    {
        UnregisterAll(true); // true = delete objects from memory as well
    }


    // register
    bool GraphNodeFactory::Register(GraphNodeCreator* creator)
    {
        assert(creator);

        // check if we already registered this one before
        if (FindCreator(creator->GetAnimGraphNodeType()))
        {
            MCore::LogWarning("GraphNodeFactory::Register() - There has already been a creator registered for the given node type %d.", creator->GetAnimGraphNodeType());
            return false;
        }

        // register it
        mCreators.Add(creator);

        return true;
    }


    // unregister
    void GraphNodeFactory::Unregister(GraphNodeCreator* creator, bool delFromMem)
    {
        mCreators.RemoveByValue(creator);
        if (delFromMem)
        {
            delete creator;
        }
    }


    // unregister all
    void GraphNodeFactory::UnregisterAll(bool delFromMem)
    {
        // if we want to delete the creators from memory as well
        if (delFromMem)
        {
            const uint32 numCreators = mCreators.GetLength();
            for (uint32 i = 0; i < numCreators; ++i)
            {
                delete mCreators[i];
            }
        }

        // clear the array
        mCreators.Clear();
    }


    // create a given node
    GraphNode* GraphNodeFactory::CreateGraphNode(const AZ::TypeId& animGraphNodeType, const char* name)
    {
        // try to locate the creator
        GraphNodeCreator* creator = FindCreator(animGraphNodeType);
        if (creator == nullptr)
        {
            return nullptr;
        }

        return creator->CreateGraphNode(name);
    }


    // create an attribute widget
    QWidget* GraphNodeFactory::CreateAttributeWidget(const AZ::TypeId& animGraphNodeType)
    {
        // try to locate the creator
        GraphNodeCreator* creator = FindCreator(animGraphNodeType);
        if (creator == nullptr)
        {
            return nullptr;
        }

        return creator->CreateAttributeWidget();
    }


    // search for the right creator
    GraphNodeCreator* GraphNodeFactory::FindCreator(const AZ::TypeId& animGraphNodeType) const
    {
        // locate the creator linked to the specified type ID
        const uint32 numCreators = mCreators.GetLength();
        for (uint32 i = 0; i < numCreators; ++i)
        {
            if (mCreators[i]->GetAnimGraphNodeType() == animGraphNodeType)
            {
                return mCreators[i];
            }
        }

        return nullptr;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNodeFactory.moc>
