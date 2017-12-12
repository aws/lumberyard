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

#include "Start.h"
#include <ScriptCanvas/Core/GraphBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            void Start::OnActivate()
            {
                AZ::EntityBus::Handler::BusConnect(GetGraph()->GetEntityId());
            }

            void Start::OnDeactivate()
            {
                AZ::EntityBus::Handler::BusDisconnect();
            }

            void Start::OnEntityActivated(const AZ::EntityId&)
            {
                SignalOutput(StartProperty::GetOutSlotId(this));
                AZ::EntityBus::Handler::BusDisconnect();
            }
        }
    }
}

#include <Include/ScriptCanvas/Libraries/Core/Start.generated.cpp>