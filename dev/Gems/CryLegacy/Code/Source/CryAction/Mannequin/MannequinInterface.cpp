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

#include "MannequinInterface.h"

#include "ActionController.h"
#include "AnimationDatabaseManager.h"
#include "GameObjects/GameObject.h"
#include "MannequinDebug.h"
#include "ProceduralClipFactory.h"

CMannequinInterface::CMannequinInterface()
    : m_pAnimationDatabaseManager(new CAnimationDatabaseManager())
    , m_mannequinGameListeners(0)
    , m_pProceduralClipFactory(new CProceduralClipFactory())
    , m_bSilentPlaybackMode(false)
{
    RegisterCVars();

    mannequin::RegisterProceduralClipsForModule(*m_pProceduralClipFactory);
}

CMannequinInterface::~CMannequinInterface()
{
    delete m_pAnimationDatabaseManager;
}

void CMannequinInterface::UnloadAll()
{
    CActionController::OnShutdown();
    GetAnimationDatabaseManager().UnloadAll();
    GetMannequinUserParamsManager().Clear();
}

void CMannequinInterface::ReloadAll()
{
    GetAnimationDatabaseManager().ReloadAll();
    GetMannequinUserParamsManager().ReloadAll(GetAnimationDatabaseManager());
}

IAnimationDatabaseManager& CMannequinInterface::GetAnimationDatabaseManager()
{
    CRY_ASSERT(m_pAnimationDatabaseManager != NULL);
    return *m_pAnimationDatabaseManager;
}

IActionController* CMannequinInterface::CreateActionController(IEntity* pEntity, SAnimationContext& context)
{
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Mannequin, 0, "ActionController (%s)", pEntity ? pEntity->GetName() ? pEntity->GetName() : "<unknown>" : "<no entity>");
    return new CActionController(pEntity, context);
}

IActionController* CMannequinInterface::CreateActionController(const AZ::EntityId& entityId, SAnimationContext& context)
{
    MEMSTAT_CONTEXT_FMT(EMemStatContextTypes::MSC_Mannequin, 0, "ActionController (%llu)", entityId);
    return new CActionController(entityId, context);
}

IActionController* CMannequinInterface::FindActionController(const IEntity& entity)
{
    return CActionController::FindActionController(entity);
}

IMannequinEditorManager* CMannequinInterface::GetMannequinEditorManager()
{
    return m_pAnimationDatabaseManager;
}

void CMannequinInterface::AddMannequinGameListener(IMannequinGameListener* pListener)
{
    m_mannequinGameListeners.push_back(pListener);
}

void CMannequinInterface::RemoveMannequinGameListener(IMannequinGameListener* pListener)
{
    m_mannequinGameListeners.erase(std::remove(m_mannequinGameListeners.begin(), m_mannequinGameListeners.end(), pListener), m_mannequinGameListeners.end());
}

uint32 CMannequinInterface::GetNumMannequinGameListeners()
{
    return m_mannequinGameListeners.size();
}

IMannequinGameListener* CMannequinInterface::GetMannequinGameListener(uint32 idx)
{
    CRY_ASSERT(idx < m_mannequinGameListeners.size());
    return m_mannequinGameListeners[idx];
}

CMannequinUserParamsManager& CMannequinInterface::GetMannequinUserParamsManager()
{
    return m_userParamsManager;
}

IProceduralClipFactory& CMannequinInterface::GetProceduralClipFactory()
{
    return *m_pProceduralClipFactory;
}

void CMannequinInterface::SetSilentPlaybackMode (bool bSilentPlaybackMode)
{
    m_bSilentPlaybackMode = bSilentPlaybackMode;
}

bool CMannequinInterface::IsSilentPlaybackMode() const
{
    return m_bSilentPlaybackMode;
}

void CMannequinInterface::RegisterCVars()
{
    mannequin::debug::RegisterCommands();
    CAnimationDatabase::RegisterCVars();
#ifndef _RELEASE
    REGISTER_STRING("mn_sequence_path", "Animations/Mannequin/FragmentSequences/", VF_CHEAT, "Default path for CryMannequin sequence files");
    REGISTER_STRING("mn_override_preview_file", "", VF_CHEAT, "Default CryMannequin preview file to use. When set it overrides the corresponding sandbox setting.");
#endif
}
