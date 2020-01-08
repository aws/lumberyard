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

#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    /** An component to reflect scriptable Editor commands
      */
    class PythonEditorFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(PythonEditorFuncsHandler, "{0F470E7E-9741-4608-84B1-7E4735FDA526}")

        static void Reflect(AZ::ReflectContext* context);

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override {}
        void Deactivate() override {}
    };

} // namespace AzToolsFramework