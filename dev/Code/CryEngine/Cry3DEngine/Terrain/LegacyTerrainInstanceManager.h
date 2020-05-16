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

#include <CrySystemBus.h>
#include <Terrain/Bus/LegacyTerrainBus.h>
#include "LegacyTerrainBase.h"

namespace LegacyTerrain
{
    //! Liason between CTerrain, C3DEngine and LegacyTerrainSystemComponent.
    class LegacyTerrainInstanceManager
        : public LegacyTerrainBase
        , public CrySystemEventBus::Handler
        , public LegacyTerrainInstanceRequestBus::Handler
    {
    public:
        LegacyTerrainInstanceManager();
        ~LegacyTerrainInstanceManager();

        ///////////////////////////////////////////////////////////////////////
        // CrySystemEventBus START
        void OnCrySystemInitialized(ISystem&, const SSystemInitParams&) override;
        // CrySystemEventBus END
        ///////////////////////////////////////////////////////////////////////


        ///////////////////////////////////////////////////////////////////////
        // LegacyTerrainInstanceRequestBus START
        bool CreateTerrainSystem(uint8* octreeData, size_t octreeDataSize) override;
        bool CreateUninitializedTerrainSystem(const STerrainInfo& terrainInfo) override;
        bool IsTerrainSystemInstantiated() const override;
        void DestroyTerrainSystem() override;
        // LegacyTerrainInstanceRequestBus END
        ///////////////////////////////////////////////////////////////////////
    };

} //namespace LegacyTerrain
