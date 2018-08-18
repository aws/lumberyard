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

#ifndef __EMSTUDIO_GRAPHNODEFACTORY_H
#define __EMSTUDIO_GRAPHNODEFACTORY_H

// include required headers
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include "../StandardPluginsConfig.h"
#include <QWidget>


namespace EMStudio
{
    // forward declarations
    class GraphNode;

    /**
     *
     *
     */
    class GraphNodeCreator
    {
        MCORE_MEMORYOBJECTCATEGORY(GraphNodeCreator, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        GraphNodeCreator();
        virtual ~GraphNodeCreator();

        virtual GraphNode* CreateGraphNode(const char* name);   // creates a new GraphNode on default
        virtual QWidget* CreateAttributeWidget();               // returns nullptr on default, indicating it should auto generate its interface widget

        virtual AZ::TypeId GetAnimGraphNodeType() = 0;            // return the TypeId for the EMotionFX::AnimGraphNode that it is linked to
    };


    /**
     *
     *
     */
    class GraphNodeFactory
    {
        MCORE_MEMORYOBJECTCATEGORY(GraphNodeFactory, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        GraphNodeFactory();
        ~GraphNodeFactory();

        bool Register(GraphNodeCreator* creator);
        void Unregister(GraphNodeCreator* creator, bool delFromMem = true);
        void UnregisterAll(bool delFromMem = true);

        GraphNode* CreateGraphNode(const AZ::TypeId& animGraphNodeType, const char* name);
        QWidget* CreateAttributeWidget(const AZ::TypeId& animGraphNodeType);

        GraphNodeCreator* FindCreator(const AZ::TypeId& animGraphNodeType) const;

    private:
        MCore::Array<GraphNodeCreator*> mCreators;
    };
}   // namespace EMStudio

#endif
