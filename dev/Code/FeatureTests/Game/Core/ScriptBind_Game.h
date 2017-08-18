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

#include <ScriptHelpers.h>

class ICrySizer;
struct IGameFramework;
struct ISystem;

namespace LYGame
{
    //////////////////////////////////////////////////////////////////////////////////////
    ///
    /// Lua script bind class for a Game instance.
    ///
    //////////////////////////////////////////////////////////////////////////////////////
    class ScriptBind_Game
        : public CScriptableBase
    {
    public:
        ScriptBind_Game(IGameFramework* gameFramework);
        virtual ~ScriptBind_Game() {}

        virtual void GetMemoryUsage(ICrySizer* sizer) const { sizer->AddObject(this, sizeof(*this)); }

    protected:
        int CacheResource(IFunctionHandler* functionHandler, const char* whoIsRequesting, const char* resourceName, int resourceType, int resourceFlags);

    private:
        void RegisterGlobals();
        void RegisterMethods();
    };
} // namespace LYGame