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
#ifndef CRYINCLUDE_CRYSYSTEM_GEMREGISTRY_H
#define CRYINCLUDE_CRYSYSTEM_GEMREGISTRY_H
#pragma once

#include "IGemManager.h"
#include "IGem.h"

#include <unordered_map>
#include <GemRegistry/IGemRegistry.h>

#ifndef AZ_MONOLITHIC_BUILD
#include <AzCore/Module/ModuleManagerBus.h>
#endif//AZ_MONOLITHIC_BUILD

class GemManager
    : public IGemManager
{
public:
    ~GemManager() override;

    // IGemManager
    bool Initialize(const SSystemInitParams& initParams) override;
    bool IsGemEnabled(const AZ::Uuid& id, const AZStd::vector<AZStd::string>& versionConstraints) const override;
private:
    const IGem* GetGem(const CryClassID& classID) const override;
    // ~IGemManager

protected:
#ifndef AZ_MONOLITHIC_BUILD // Loads statically in monolithic mode, doesn't require a module.
    AZStd::shared_ptr<AZ::DynamicModuleHandle> m_registryModule;
#endif//AZ_MONOLITHIC_BUILD
    Gems::IGemRegistry * m_registry = nullptr;
    Gems::IProjectSettings* m_projectSettings = nullptr;
    // Map used to look up Gem instances based on their implementation IDs.
    std::unordered_map<CryGUID, IGemPtr> m_gemPtrs;

    // Loads the Registry module, inits the Registry.
    bool InitRegistry();
    // Loads Gem configs
    bool LoadGems(const SSystemInitParams& initParams);
    // Instantiates instances of IGem
    bool InstantiateGems();
};

inline IGemManager* CreateIGemManager()
{
    return new GemManager();
}

#endif // CRYINCLUDE_CRYSYSTEM_GEMREGISTRY_H
