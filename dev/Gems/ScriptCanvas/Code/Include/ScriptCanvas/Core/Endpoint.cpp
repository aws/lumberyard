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

#include "precompiled.h"

#include "Endpoint.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    Endpoint::Endpoint(const AZ::EntityId& nodeId, const SlotId& slotId)
        : m_nodeId(nodeId)
        , m_slotId(slotId)
    {}

    bool Endpoint::operator==(const Endpoint& other) const
    {
        return m_nodeId == other.m_nodeId && m_slotId == other.m_slotId;
    }

    void Endpoint::Reflect(AZ::ReflectContext* reflection)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
        if (serializeContext)
        {
            serializeContext->Class<Endpoint>()
                ->Version(1)
                ->Field("nodeId", &Endpoint::m_nodeId)
                ->Field("slotId", &Endpoint::m_slotId)
                ;
        }

    }
}