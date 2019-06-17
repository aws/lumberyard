/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright EntityRef license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "precompiled.h"
#include "ScriptEventsTestFixture.h"
#include "ScriptEventsTestApplication.h"

namespace ScriptEventsTests
{
    ScriptEventsTests::Application* ScriptEventsTestFixture::s_application = nullptr;

    AZ::Debug::DrillerManager* ScriptEventsTestFixture::s_drillerManager = nullptr;
    const bool ScriptEventsTestFixture::s_enableMemoryLeakChecking = false;

    ScriptEventsTests::Application* ScriptEventsTestFixture::GetApplication()
    {
        return s_application;
    }

}

