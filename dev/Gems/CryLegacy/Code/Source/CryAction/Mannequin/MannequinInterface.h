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

#ifndef CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUININTERFACE_H
#define CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUININTERFACE_H
#pragma once

#include "ICryMannequin.h"

class CProceduralClipFactory;

class CMannequinInterface
    : public IMannequin
{
public:
    CMannequinInterface();
    ~CMannequinInterface();

    // IMannequin
    virtual void UnloadAll();
    virtual void ReloadAll();

    virtual IAnimationDatabaseManager& GetAnimationDatabaseManager();
    virtual IActionController* CreateActionController(IEntity* pEntity, SAnimationContext& context);
    virtual IActionController* CreateActionController(const AZ::EntityId& entityId, SAnimationContext& context);
    virtual IActionController* FindActionController(const IEntity& entity);
    virtual IMannequinEditorManager* GetMannequinEditorManager();
    virtual CMannequinUserParamsManager& GetMannequinUserParamsManager();
    virtual IProceduralClipFactory& GetProceduralClipFactory();

    virtual void AddMannequinGameListener(IMannequinGameListener* pListener);
    virtual void RemoveMannequinGameListener(IMannequinGameListener* pListener);
    virtual uint32 GetNumMannequinGameListeners();
    virtual IMannequinGameListener* GetMannequinGameListener(uint32 idx);
    virtual void SetSilentPlaybackMode (bool bSilentPlaybackMode);
    virtual bool IsSilentPlaybackMode() const;
    // ~IMannequin

private:
    void RegisterCVars();

private:
    class CAnimationDatabaseManager* m_pAnimationDatabaseManager;
    std::vector<IMannequinGameListener*> m_mannequinGameListeners;
    CMannequinUserParamsManager m_userParamsManager;
    std::unique_ptr<CProceduralClipFactory> m_pProceduralClipFactory;
    bool m_bSilentPlaybackMode;
};

#endif // CRYINCLUDE_CRYACTION_MANNEQUIN_MANNEQUININTERFACE_H
