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
#ifndef CRYINCLUDE_CRYCOMMON_IGEM_H
#define CRYINCLUDE_CRYCOMMON_IGEM_H
#pragma once

#include "BoostHelpers.h"
#include "CryExtension/ICryUnknown.h"
#include "CryExtension/Impl/ClassWeaver.h"
#include "platform.h"
#include "IEntitySystem.h"
#include "ISystem.h"
#include "IEngineModule.h"
#include "IGemManager.h"
#include "CrySystemBus.h"

#include <AzCore/Module/Module.h>

///////////////////////////////////////////////////////////////////////////////
// Gem Interface
///////////////////////////////////////////////////////////////////////////////

/**
 * \DEPRECATED The IGem class has been deprecated in favor of AZ::Module.
 * Extend this class to define a Gem instance.
 *
 * A Gem is an optional extension to add or extend engine functionality in a modular way.
 *
 * The IGem instance is what handles system and game events, such as initialization and shutdown.
 */
class IGem
    : public IEngineModule
    , public ISystemEventListener
    , public AZ::Module
{
public:
    CRYINTERFACE_DECLARE(IGem, 0x7746709e6bf54585, 0xbc3c96087b2a017b);
    /// Required by IEngineModule.
    const char* GetCategory() const override final { return "Gems"; }
    /// Implemented by GEM_IMPLEMENT.
    virtual const CryClassID& GetClassID() const = 0;
    virtual void RegisterFlowNodes() const = 0;
    bool Initialize(SSystemGlobalEnvironment&, const SSystemInitParams&) override final { return true; }
};

DECLARE_SMART_POINTERS(IGem)

#define _GEM_IMPLEMENT_BEGIN()      \
    CRYINTERFACE_BEGIN()            \
    CRYINTERFACE_ADD(IEngineModule) \
    CRYINTERFACE_ADD(IGem)

#define _GEM_IMPLEMENT_END(GemClass, cidHigh, cidLow)                 \
    CRYINTERFACE_END()                                                \
    CRYGENERATE_SINGLETONCLASS(GemClass, #GemClass, cidHigh, cidLow)  \
    const CryClassID&GetClassID() const override { return GetCID(); } \
    const char* GetName() const override { return #GemClass; }        \
    void RegisterFlowNodes() const override;

/**
 * \DEPRECATED The IGem class has been deprecated in favor of AZ::Module.
 * Use inside of a class to implement it as a gem. Should only used instead of GEM_GENERATE when there is a specific reason.
 *
 * \param GemClass  The name of the surrounding class defining the engine module. Should be called [GemName]Gem.
 * \param cidHigh   The high end half of the generated GUID.
 * \param cidLow    The low end half of the generated GUID.
 */
#define GEM_IMPLEMENT(GemClass, cidHigh, cidLow) \
    _GEM_IMPLEMENT_BEGIN()                       \
    _GEM_IMPLEMENT_END(GemClass, cidHigh, cidLow)

/**
 * \DEPRECATED The IGem class has been deprecated in favor of AZ::Module.
 * Use inside of a class to implement it as a gem. Should only used instead of GEM_GENERATE when there is a specific reason.
 *
 * \param GemClass  The name of the surrounding class defining the engine module. Should be called [GemName]Gem.
 * \param Interface The Gem interface that GemClass implements. Should be called I[GemName]Gem.
 * \param cidHigh   The high end half of the generated GUID.
 * \param cidLow    The low end half of the generated GUID.
 */
#define GEM_IMPLEMENT_WITH_INTERFACE(GemClass, Interface, cidHigh, cidLow) \
    _GEM_IMPLEMENT_BEGIN()                                                 \
    CRYINTERFACE_ADD(Interface)                                            \
    _GEM_IMPLEMENT_END(GemClass, cidHigh, cidLow)

#ifndef AZ_MONOLITHIC_BUILD
#define GEM_FLOW_FACTORY(GemClass)                                                         \
    void GemClass::RegisterFlowNodes() const                                               \
    {                                                                                      \
        RegisterExternalFlowNodes();                                                       \
    }
#define _GEM_MODULE_FLAG extern "C" AZ_DLL_EXPORT int ThisModuleIsAGem() { return 1; }
#else // AZ_MONOLITHIC_BUILD
#define GEM_FLOW_FACTORY(GemClass) void GemClass::RegisterFlowNodes() const { }
#define _GEM_MODULE_FLAG
#endif // AZ_MONOLITHIC_BUILD

/**
 * \DEPRECATED Gems which implement an AZ::Module should use AZ_DECLARE_MODULE_CLASS instead of GEM_REGISTER.
 * Registers the Gem.
 *
 * \param GemClass  The class that implements IGem
 */
#define GEM_REGISTER(GemClass)            \
    AZ_DECLARE_MODULE_INITIALIZATION      \
    CRYREGISTER_SINGLETON_CLASS(GemClass) \
    GEM_FLOW_FACTORY(GemClass)            \
    _GEM_MODULE_FLAG

#if defined(AZ_TESTS_ENABLED)
#define GEM_IMPLEMENT_TEST_RUNNER AZ_UNIT_TEST_HOOK();
#else
#define GEM_IMPLEMENT_TEST_RUNNER
#endif

/**
 * CryHooksModule is an AZ::Module with common hooks into CryEngine systems.
 * - Sets gEnv once CrySystem has initialized.
 * - Registers as a CrySystem event handler.
 */
class CryHooksModule
    : public AZ::Module
    , protected CrySystemEventBus::Handler
    , protected  ISystemEventListener
{
public:
    AZ_RTTI(CryHooksModule, "{BD896D16-6F7D-4EA6-A532-0A9E6BF3C089}", AZ::Module);
    CryHooksModule()
        : Module()
    {
        CrySystemEventBus::Handler::BusConnect();
    }

    ~CryHooksModule() override
    {
        CrySystemEventBus::Handler::BusDisconnect();

        if (gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
        {
            gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
        }
    }

protected:

    void OnCrySystemPreInitialize(ISystem& system, const SSystemInitParams& systemInitParams) override
    {
        (void)systemInitParams;

#if !defined(AZ_MONOLITHIC_BUILD)
        // When module is linked dynamically, we must set our gEnv pointer.
        // When module is linked statically, we'll share the application's gEnv pointer.
        gEnv = system.GetGlobalEnvironment();
#else
        (void)system;
#endif
    }

    void OnCrySystemInitialized(ISystem& system, const SSystemInitParams& systemInitParams) override
    {
        (void)systemInitParams;

        system.GetISystemEventDispatcher()->RegisterListener(this);
    }

    void OnCrySystemPostShutdown() override
    {
#if !defined(AZ_MONOLITHIC_BUILD)
        gEnv = nullptr;
#endif
    }

    void OnSystemEvent(ESystemEvent /*event*/, UINT_PTR /*wparam*/, UINT_PTR /*lparam*/) override {}
};

#endif // CRYINCLUDE_CRYCOMMON_IGEM_H