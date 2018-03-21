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

#include "stdafx.h"
#include "VehicleData.h"

#include "VehicleXMLLoader.h"


IVariable* CVehicleData::m_pDefaults(0);
XmlNodeRef CVehicleData::m_xmlDef(0);


//////////////////////////////////////////////////////////////////////////
const IVariable* CVehicleData::GetDefaultVar()
{
    // force reload of default var if debugdraw is on
    static ICVar* pCVar = gEnv->pConsole->GetCVar("v_debugdraw");
    if ((pCVar->GetIVal() == DEBUGDRAW_VEED) && m_pDefaults != 0)
    {
        m_pDefaults->Release();
        m_pDefaults = 0;
    }

    if (m_pDefaults == 0)
    {
        IVehicleData* pData = VehicleDataLoad(GetXMLDef(), VEED_DEFAULTS.toUtf8().data());
        if (!pData)
        {
            Log("GetDefaultVar: returned zero!");
        }
        else
        {
            m_pDefaults = pData->GetRoot();
        }
    }
    return m_pDefaults;
}


//////////////////////////////////////////////////////////////////////////
const XmlNodeRef& CVehicleData::GetXMLDef()
{
    // Always force a reload
    m_xmlDef = GetISystem()->LoadXmlFromFile(VEHICLE_XML_DEF.toUtf8().data());
    return m_xmlDef;
}

//////////////////////////////////////////////////////////////////////////
IVariable* GetChildVar(const IVariable* array, const char* name, bool recursive /*=false*/)
{
    if (array == 0 || array->IsEmpty())
    {
        return 0;
    }

    // first search top level
    for (int i = 0; i < array->GetNumVariables(); ++i)
    {
        IVariable* var = array->GetVariable(i);
        if (0 == QString::compare(name, var->GetName()))
        {
            return var;
        }
    }
    if (recursive)
    {
        for (int i = 0; i < array->GetNumVariables(); ++i)
        {
            if (IVariable* pVar = GetChildVar(array->GetVariable(i), name, recursive))
            {
                return pVar;
            }
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////////
IVariable* GetOrCreateChildVar(IVariable* array, const char* name, bool searchRec /*=false*/, bool createRec /*=false*/)
{
    IVariable* pRes = GetChildVar(array, name, searchRec);
    if (!pRes)
    {
        if (IVariable* pDef = GetChildVar(CVehicleData::GetDefaultVar(), name, true))
        {
            pRes = pDef->Clone(createRec);

            if (!createRec)
            {
                pRes->DeleteAllVariables(); // VarArray clones children also with non-recursive
            }
            array->AddVariable(pRes);
        }
    }
    return pRes;
}

//////////////////////////////////////////////////////////////////////////
bool HasChildVar(const IVariable* array, const IVariable* child, bool recursive /*= false*/)
{
    if (!array || !child)
    {
        return 0;
    }

    // first search top level
    for (int i = 0; i < array->GetNumVariables(); ++i)
    {
        if (array->GetVariable(i) == child)
        {
            return true;
        }
    }
    if (recursive)
    {
        for (int i = 0; i < array->GetNumVariables(); ++i)
        {
            if (HasChildVar(array->GetVariable(i), child, recursive))
            {
                return true;
            }
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
IVariable* GetParentVar(IVariable* array, const IVariable* child)
{
    if (!array || !child)
    {
        return 0;
    }

    // first search top level
    for (int i = 0; i < array->GetNumVariables(); ++i)
    {
        if (array->GetVariable(i) == child)
        {
            return array;
        }
    }
    // search childs
    for (int i = 0; i < array->GetNumVariables(); ++i)
    {
        if (IVariable* parent = GetParentVar(array->GetVariable(i), child))
        {
            return parent;
        }
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
IVariable* CreateDefaultVar(const char* name, bool rec /*= false*/)
{
    IVariable* pDef = GetChildVar(CVehicleData::GetDefaultVar(), name, true);
    if (!pDef)
    {
        //VeedLog("Default var <%s> not found!", name);
        return 0;
    }
    return pDef->Clone(rec);
}

//////////////////////////////////////////////////////////////////////////
IVariable* CreateDefaultChildOf(const char* name)
{
    IVariable* pParent = GetChildVar(CVehicleData::GetDefaultVar(), name, true);
    if (!pParent)
    {
        Log("Default parent <%s> not found!", name);
        return 0;
    }
    if (pParent->GetNumVariables() > 0)
    {
        return pParent->GetVariable(0)->Clone(true);
    }
    else
    {
        Log("Default Parent <%s> has no children!");
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////
void ReplaceChildVars(IVariable* from, IVariable* to)
{
    if (!from || !to)
    {
        return;
    }

    to->DeleteAllVariables();
    for (int i = 0; i < from->GetNumVariables(); ++i)
    {
        to->AddVariable(from->GetVariable(i));
    }
}

//////////////////////////////////////////////////////////////////////////
void CVehicleData::FillDefaults(IVariable* pVar, const char* defaultVar, const IVariable* pParent /*=GetDefaultVar*/)
{
    IVariable* pDef = GetChildVar(pParent, defaultVar, true);
    if (!pDef)
    {
        Log("Default var <%s> not found!", defaultVar);
        return;
    }

    // if null, just clone
    if (!pVar)
    {
        pVar = pDef->Clone(true);
        return;
    }

    // else rec. compare children and add missing vars
    for (int i = 0; i < pDef->GetNumVariables(); ++i)
    {
        IVariable* pChild = GetChildVar(pVar, pDef->GetVariable(i)->GetName().toUtf8().data());
        if (!pChild)
        {
            // if null, just clone
            IVariable* pClone = pDef->GetVariable(i)->Clone(true);
            pVar->AddVariable(pClone);
        }
        else
        {
            // if present, descent to children
            FillDefaults(pChild, pChild->GetName().toUtf8().data(), pDef);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void DumpVariable(IVariable* pVar, int level /*=0*/)
{
    if (!pVar)
    {
        return;
    }

    string tab("");
    for (int i = 0; i < level; ++i)
    {
        tab.append("\t");
    }

    QString value;
    pVar->Get(value);
    Log("%s<%s> = '%s'", tab.c_str(), pVar->GetName().toUtf8().data(), value.toUtf8().data());

    for (int i = 0; i < pVar->GetNumVariables(); ++i)
    {
        DumpVariable(pVar->GetVariable(0), level++);
    }
}
