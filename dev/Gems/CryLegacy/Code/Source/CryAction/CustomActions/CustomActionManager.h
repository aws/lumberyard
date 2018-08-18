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

#ifndef CRYINCLUDE_CRYACTION_CUSTOMACTIONS_CUSTOMACTIONMANAGER_H
#define CRYINCLUDE_CRYACTION_CUSTOMACTIONS_CUSTOMACTIONMANAGER_H
#pragma once

#include <IFlowSystem.h>
#include <ICustomActions.h>
#include <CryListenerSet.h>
#include "CustomAction.h"
#include <IEntityPoolManager.h>

///////////////////////////////////////////////////
// CCustomActionManager keeps track of all CustomActions
///////////////////////////////////////////////////
class CCustomActionManager
    : public ICustomActionManager
    , public IEntityPoolListener
{
public:
    CCustomActionManager();
    virtual ~CCustomActionManager();

public:
    // ICustomActionManager
    virtual bool StartAction(IEntity* pObject, const char* szCustomActionGraphName, ICustomActionListener* pListener = NULL);
    virtual bool SucceedAction(IEntity* pObject, const char* szCustomActionGraphName, ICustomActionListener* pListener = NULL);
    virtual bool SucceedWaitAction(IEntity* pObject);
    virtual bool SucceedWaitCompleteAction(IEntity* pObject);
    virtual bool AbortAction(IEntity* pObject);
    virtual bool EndAction(IEntity* pObject, bool bSuccess);
    virtual void LoadLibraryActions(const char* sPath);
    virtual void ClearActiveActions();
    virtual void ClearLibraryActions();
    virtual size_t GetNumberOfCustomActionsFromLibrary() const { return m_actionsLib.size(); }
    virtual ICustomAction* GetCustomActionFromLibrary(const char* szCustomActionGraphName);
    virtual ICustomAction* GetCustomActionFromLibrary(const size_t index);
    virtual size_t GetNumberOfActiveCustomActions() const;
    virtual ICustomAction* GetActiveCustomAction(const IEntity* pObject);
    virtual ICustomAction* GetActiveCustomAction(const size_t index);
    virtual bool UnregisterListener(ICustomActionListener* pEventListener);
    virtual void Serialize(TSerialize ser);
    // ~ICustomActionManager

    // IEntityPoolListener
    virtual void OnEntityReturningToPool(EntityId entityId, IEntity* pEntity);
    // ~IEntityPoolListener

    // Removes deleted Action from the list of active actions
    void Update();

    void GetMemoryUsage(ICrySizer* pSizer) const
    {
        SIZER_COMPONENT_NAME(pSizer, "CustomActionManager");

        pSizer->AddObject(this, sizeof(*this));
        pSizer->AddObject(m_actionsLib);
        pSizer->AddObject(m_activeActions);
    }

protected:
    // Adds an Action in the list of active actions
    ICustomAction* AddActiveCustomAction(IEntity* pObject, const char* szCustomActionGraphName, ICustomActionListener* pListener = NULL);

    // Called when entity is removed
    void OnEntityRemove(IEntity* pEntity);

private:
    // Library of all defined Actions
    typedef std::map<string, CCustomAction> TCustomActionsLib;
    TCustomActionsLib m_actionsLib;

    // List of all active Actions (including suspended and to be deleted)
    typedef std::list<CCustomAction> TActiveActions;
    TActiveActions m_activeActions;
};

#endif // CRYINCLUDE_CRYACTION_CUSTOMACTIONS_CUSTOMACTIONMANAGER_H
