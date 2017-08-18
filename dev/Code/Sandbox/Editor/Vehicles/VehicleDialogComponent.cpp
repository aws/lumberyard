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

#include "StdAfx.h"
#include "VehicleDialogComponent.h"

#include "Objects/BaseObject.h"
#include "VehicleData.h"
#include "VehiclePrototype.h"
#include "VehiclePart.h"
#include "VehicleHelperObject.h"
#include "VehicleSeat.h"
#include "VehicleWeapon.h"
#include "VehicleComp.h"

IVeedObject* IVeedObject::GetVeedObject(CBaseObject* pObj)
{
    assert(pObj);
    if (!pObj)
    {
        return NULL;
    }

    if (qobject_cast<CVehiclePart*>(pObj))
    {
        return (CVehiclePart*)pObj;
    }

    else if (qobject_cast<CVehicleHelper*>(pObj))
    {
        return (CVehicleHelper*)pObj;
    }

    else if (qobject_cast<CVehicleSeat*>(pObj))
    {
        return (CVehicleSeat*)pObj;
    }

    else if (qobject_cast<CVehicleWeapon*>(pObj))
    {
        return (CVehicleWeapon*)pObj;
    }

    else if (qobject_cast<CVehicleComponent*>(pObj))
    {
        return (CVehicleComponent*)pObj;
    }

    else if (qobject_cast<CVehiclePrototype*>(pObj))
    {
        return (CVehiclePrototype*)pObj;
    }

    return 0;
}

void VeedLog(const char* s, ...)
{
    static ICVar* pVar = gEnv->pConsole->GetCVar("v_debugdraw");

    if (pVar && pVar->GetIVal() == DEBUGDRAW_VEED)
    {
        char buffer[1024];
        va_list arg;
        va_start(arg, s);
        vsprintf_s(buffer, s, arg);
        va_end(arg);

        Log(buffer);
    }
}



void CVeedObject::UpdateObjectOnVarChangeCallback(IVariable* var)
{
    if (m_ignoreObjectUpdateCallback)
    {
        return;
    }

    m_ignoreOnTransformCallback = true;
    UpdateObjectFromVar();
    m_ignoreOnTransformCallback = false;
}



void CVeedObject::EnableUpdateObjectOnVarChange(const string& childVarName)
{
    IVariable* pVar = GetVariable();
    IVariable* pChildVar = GetChildVar(pVar, childVarName);
    if (pChildVar == NULL)
    {
        return;
    }

    pChildVar->AddOnSetCallback(functor(*this, &CVeedObject::UpdateObjectOnVarChangeCallback));
}



void CVeedObject::DisableUpdateObjectOnVarChange(const string& childVarName)
{
    IVariable* pVar = GetVariable();
    IVariable* pChildVar = GetChildVar(pVar, childVarName);
    if (pChildVar == NULL)
    {
        return;
    }

    pChildVar->RemoveOnSetCallback(functor(*this, &CVeedObject::UpdateObjectOnVarChangeCallback));
}


void CVeedObject::InitOnTransformCallback(CBaseObject* pObject)
{
    assert(pObject != NULL);
    if (pObject == NULL)
    {
        return;
    }

    pObject->AddEventListener(functor(*this, &CVeedObject::OnObjectEventCallback));
}


void CVeedObject::OnObjectEventCallback(CBaseObject* pObject, int eventId)
{
    if (eventId == CBaseObject::ON_TRANSFORM)
    {
        if (m_ignoreOnTransformCallback)
        {
            return;
        }
        m_ignoreObjectUpdateCallback = true;
        m_ignoreOnTransformCallback = true;
        OnTransform();
        m_ignoreObjectUpdateCallback = false;
        m_ignoreOnTransformCallback = false;
    }
}