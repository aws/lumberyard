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
#include "StdAfx.h"

#include <CrtDebugStats.h>

// Included only once per DLL module.
#include <platform_impl.h>

#include <IEngineModule.h>
#include <CryExtension/ICryFactory.h>
#include <CryExtension/Impl/ClassWeaver.h>


#include <LmbrAWS.h>

namespace LmbrAWS
{
    class EngineModule_LmbrAWS
        : public IEngineModule
    {
        CRYINTERFACE_SIMPLE(IEngineModule)

        CRYGENERATE_SINGLETONCLASS(EngineModule_LmbrAWS, "EngineModule_LmbrAWS", 0xE3EEEBC474C6435C, 0x9AF000711C613950)

        virtual const char* GetName() const {
            return "LmbrAWS";
        };
        virtual const char* GetCategory() const { return "CryEngine"; };

        virtual bool Initialize(SSystemGlobalEnvironment& env, const SSystemInitParams& initParams)
        {
            ISystem* system = env.pSystem;
            ModuleInitISystem(system, "LmbrAWS");
            env.pLmbrAWS = nullptr;
            return true;
        }
    };

    CRYREGISTER_SINGLETON_CLASS(EngineModule_LmbrAWS)

    EngineModule_LmbrAWS::EngineModule_LmbrAWS()
    {
    };

    EngineModule_LmbrAWS::~EngineModule_LmbrAWS()
    {
    };

    IClientManager* GetClientManager()
    {
        return nullptr;
    }

    MaglevConfig* GetMaglevConfig()
    {
        return nullptr;
    }
} // Namespace LmbrAWS

ILmbrAWS* GetLmbrAWS()
{
    return gEnv->pLmbrAWS;
}

