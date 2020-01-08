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
#ifndef CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTSCRIPT_H
#define CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTSCRIPT_H
#pragma once

#include <IComponent.h>
#include <ComponentType.h>
#include <SerializeFwd.h>

struct IEntityScript;
struct IScriptTable;

//! Interface for the script component.
//! This component handles interaction between the entity and its script.
struct IComponentScript
    : public IComponent
{
    // <interfuscator:shuffle>
    DECLARE_COMPONENT_TYPE("ComponentScript", 0xE1341BAF5E714E1B, 0xB65E170811023EE0)

    IComponentScript() {}

    ComponentEventPriority GetEventPriority(const int eventID) const override { return EntityEventPriority::Script; }

    virtual void SetScriptUpdateRate(float fUpdateEveryNSeconds) = 0;
    virtual IScriptTable* GetScriptTable() const = 0;
    virtual void CallEvent(const char* sEvent) = 0;
    virtual void CallEvent(const char* sEvent, float fValue) = 0;
    virtual void CallEvent(const char* sEvent, double fValue) = 0;
    virtual void CallEvent(const char* sEvent, bool bValue) = 0;
    virtual void CallEvent(const char* sEvent, const char* sValue) = 0;
    virtual void CallEvent(const char* sEvent, const Vec3& vValue) = 0;
    virtual void CallEvent(const char* sEvent, EntityId nEntityId) = 0;
    virtual void CallInitEvent(bool bFromReload) = 0;

    // Description:
    //    Change current state of the entity script.
    // Return:
    //    If state was successfully set.
    virtual bool GotoState(const char* sStateName) = 0;

    // Description:
    //    Change current state of the entity script.
    // Return:
    //    If state was successfully set.
    virtual bool GotoStateId(int nStateId) = 0;

    // Description:
    //    Check if entity is in specified state.
    // Arguments:
    //    sStateName - Name of state table within entity script (case sensitive).
    // Return:
    //    If entity script is in specified state.
    virtual bool IsInState(const char* sStateName) = 0;

    // Description:
    //    Retrieves name of the currently active entity script state.
    // Return:
    //    Name of current state.
    virtual const char* GetState() = 0;

    // Description:
    //    Retrieves the id of the currently active entity script state.
    // Return:
    //    Index of current state.
    virtual int GetStateId() = 0;

    // Description:
    //     Fires an event in the entity script.
    //     This will call OnEvent(id,param) Lua function in entity script, so that script can handle this event.
    // See Also:
    //     EScriptEventId, IScriptObject
    virtual void SendScriptEvent(int Event, IScriptTable* pParamters, bool* pRet = NULL) = 0;
    virtual void SendScriptEvent(int Event, const char* str, bool* pRet = NULL) = 0;
    virtual void SendScriptEvent(int Event, int nParam, bool* pRet = NULL) = 0;

    // Description:
    //    Change the Entity Script used by the Script Component.
    //    Caller is responsible for making sure new script is initialized and script bound as required
    // Arguments:
    //    pScript - an entity script object that has already been loaded with the new script
    //    params - parameters used to set the properties table if required
    virtual void ChangeScript(IEntityScript* pScript, SEntitySpawnParams* params) = 0;

    // Note:
    //    Promoted functions to IComponentScript for access by CScriptBind_Entity.
    virtual void RegisterForAreaEvents(bool bEnable) = 0;

    // Description:
    //    Called from physics events.
    // Note:
    //    Promoted function to IComponentScript for access by CComponentPhysics
    virtual void OnCollision(IEntity* pTarget, int matId, const Vec3& pt, const Vec3& n, const Vec3& vel, const Vec3& targetVel, int partId, float mass) = 0;

    // Description:
    //    Callback inovked when this script component is prepared from a pool.
    virtual void OnPreparedFromPool() = 0;

    // Description:
    //    Serialize the script component's properties.
    virtual void SerializeProperties(TSerialize ser) = 0;

    // </interfuscator:shuffle>
};

DECLARE_SMART_POINTERS(IComponentScript);


#endif // CRYINCLUDE_CRYCOMMON_COMPONENTS_ICOMPONENTSCRIPT_H