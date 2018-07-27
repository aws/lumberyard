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

#ifndef CRYINCLUDE_CRYENTITYSYSTEM_SCRIPTPROPERTIES_H
#define CRYINCLUDE_CRYENTITYSYSTEM_SCRIPTPROPERTIES_H
#pragma once

struct IScriptObject;

//////////////////////////////////////////////////////////////////////////
// This class handles assignment of entity script properties from XML nodes
// to the script tables.
//////////////////////////////////////////////////////////////////////////
class CScriptProperties
{
public:
    bool SetProperties(XmlNodeRef& entityNode, IScriptTable* pEntityTable);
    void Assign(XmlNodeRef& propsNode, IScriptTable* pPropsTable);
};

#endif // CRYINCLUDE_CRYENTITYSYSTEM_SCRIPTPROPERTIES_H
