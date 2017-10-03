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

#include <Libraries/Entity/Entity.h>
#include <AzCore/Math/Vector3.h>
#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Entity
        {
            class Rotate 
                : public Node
            {
            public:

                AZ_COMPONENT(Rotate, "{6F802B53-E726-430D-9F41-63CFC783F91D}", Node);

                static void Reflect(AZ::ReflectContext*);

                void OnInit() override;
                void OnInputSignal(const SlotId&) override;
                void Visit(NodeVisitor& visitor) const override { visitor.Visit(*this); }

            protected:
                static const char* k_setEntityName;
                static const char* k_setAnglesName;
            };
        }
    }
}