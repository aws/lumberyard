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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "CryLegacy_precompiled.h"
#include "GameObjectSystem.h"

#include "WorldQuery.h"
#include "Interactor.h"
#include "GameVolumes/GameVolume_Water.h"
#include "GameObjects/MannequinObject.h"

#include <IGameVolumes.h>
#include <IGameFramework.h>


// entityClassString can be different from extensionClassName to allow mapping entity classes to differently
// named C++ Classes
#define REGISTER_GAME_OBJECT_EXTENSION(framework, entityClassString, extensionClassName, script)      \
    {                                                                                                 \
        IEntityClassRegistry::SEntityClassDesc clsDesc;                                               \
        clsDesc.sName = entityClassString;                                                            \
        clsDesc.sScriptFile = script;                                                                 \
        struct C##extensionClassName##Creator                                                         \
            : public IGameObjectExtensionCreatorBase                                                  \
        {                                                                                             \
            IGameObjectExtensionPtr Create()                                                          \
            {                                                                                         \
                return gEnv->pEntitySystem->CreateComponent<C##extensionClassName>();                 \
            }                                                                                         \
            void GetGameObjectExtensionRMIData(void** ppRMI, size_t * nCount)                         \
            {                                                                                         \
                C##extensionClassName::GetGameObjectExtensionRMIData(ppRMI, nCount);                  \
            }                                                                                         \
        };                                                                                            \
        static C##extensionClassName##Creator _creator;                                               \
        framework->GetIGameObjectSystem()->RegisterExtension(entityClassString, &_creator, &clsDesc); \
    }                                                                                                 \

#define HIDE_FROM_EDITOR(className)                                                             \
    { IEntityClass* pItemClass = gEnv->pEntitySystem->GetClassRegistry()->FindClass(className); \
      pItemClass->SetFlags(pItemClass->GetFlags() | ECLF_INVISIBLE); }

#define REGISTER_EDITOR_VOLUME_CLASS(frameWork, className)                                             \
    {                                                                                                  \
        IGameVolumes* pGameVolumes = frameWork->GetIGameVolumesManager();                              \
        IGameVolumesEdit* pGameVolumesEdit = pGameVolumes ? pGameVolumes->GetEditorInterface() : NULL; \
        if (pGameVolumesEdit != NULL)                                                                  \
        {                                                                                              \
            pGameVolumesEdit->RegisterEntityClass(className);                                          \
        }                                                                                              \
    }

void CGameObjectSystem::RegisterFactories(IGameFramework* pFrameWork)
{
    REGISTER_FACTORY(pFrameWork, "WorldQuery", CWorldQuery, false);
    REGISTER_FACTORY(pFrameWork, "Interactor", CInteractor, false);

    REGISTER_GAME_OBJECT_EXTENSION(pFrameWork, "WaterVolume", GameVolume_Water, "Scripts/Entities/Environment/WaterVolume.lua");
    REGISTER_GAME_OBJECT_EXTENSION(pFrameWork, "MannequinObject", MannequinObject, "Scripts/Entities/Anim/MannequinObject.lua");
    HIDE_FROM_EDITOR("WaterVolume");
    REGISTER_EDITOR_VOLUME_CLASS(pFrameWork, "WaterVolume");
}
