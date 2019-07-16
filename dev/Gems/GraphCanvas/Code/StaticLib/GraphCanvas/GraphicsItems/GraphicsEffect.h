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

#include <AzCore/Component/Entity.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/GraphicsItems/GraphicsEffectBus.h>

namespace GraphCanvas
{
    template<class GraphicsClass>
    class GraphicsEffect
        : public GraphicsClass
        , public GraphicsEffectRequestBus::Handler
    {
    public:
        GraphicsEffect()
            : m_id(AZ::Entity::MakeId())
        {
            GraphicsEffectRequestBus::Handler::BusConnect(GetId());
        }
        
        GraphicsEffectId GetId() const
        {
            return m_id;
        }
        
        // GraphicsEffectRequestBus
        virtual QGraphicsItem* AsQGraphicsItem()
        {
            return this;
        }
        
        virtual void OnGraphicsEffectCancelled()
        {            
        }
        ////
        
    private:
        GraphicsEffectId m_id;
    };
}