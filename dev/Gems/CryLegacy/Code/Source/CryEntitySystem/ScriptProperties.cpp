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
#include "ScriptProperties.h"
#include <IScriptSystem.h>

//////////////////////////////////////////////////////////////////////////
bool CScriptProperties::SetProperties(XmlNodeRef& entityNode, IScriptTable* pEntityTable)
{
    XmlNodeRef propsNode = entityNode->findChild("Properties");
    if (propsNode)
    {
        SmartScriptTable pPropsTable;
        if (pEntityTable->GetValue("Properties", pPropsTable))
        {
            Assign(propsNode, pPropsTable);
        }
    }

    propsNode = entityNode->findChild("Properties2");
    if (propsNode)
    {
        SmartScriptTable pPropsTable;
        if (pEntityTable->GetValue("PropertiesInstance", pPropsTable))
        {
            Assign(propsNode, pPropsTable);
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CScriptProperties::Assign(XmlNodeRef& propsNode, IScriptTable* pPropsTable)
{
    const char* key = "";
    const char* value = "";
    int nAttrs = propsNode->getNumAttributes();
    for (int attr = 0; attr < nAttrs; attr++)
    {
        if (!propsNode->getAttributeByIndex(attr, &key, &value))
        {
            continue;
        }

        ScriptVarType varType = pPropsTable->GetValueType(key);
        switch (varType)
        {
        case svtNull:
            break;
        case svtString:
            pPropsTable->SetValue(key, value);
            break;
        case svtNumber:
        {
            float fValue = (float)atof(value);
            pPropsTable->SetValue(key, fValue);
        }
        break;
        case svtBool:
        {
            bool const bValue = (_stricmp(value, "true") == 0) || (_stricmp(value, "1") == 0);
            pPropsTable->SetValue(key, bValue);
        }
        break;
        case svtObject:
        {
            Vec3 vec;
            propsNode->getAttr(key, vec);
            CScriptVector vecTable;
            pPropsTable->GetValue(key, vecTable);
            vecTable.Set(vec);
            //pPropsTable->SetValue( key,vec );
        }
        break;
        case svtPointer:
        case svtUserData:
        case svtFunction:
            // Ignore invalid property types.
            break;
        }
    }

    for (int i = 0; i < propsNode->getChildCount(); i++)
    {
        XmlNodeRef childNode = propsNode->getChild(i);
        SmartScriptTable pChildPropTable;
        if (pPropsTable->GetValue(childNode->getTag(), pChildPropTable))
        {
            // Recurse.
            Assign(childNode, pChildPropTable);
        }
    }
}