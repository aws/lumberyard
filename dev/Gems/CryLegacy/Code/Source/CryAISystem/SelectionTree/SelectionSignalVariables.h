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

#ifndef CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONSIGNALVARIABLES_H
#define CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONSIGNALVARIABLES_H
#pragma once

/*
    A simple mechanism to flip variable values based on signal fire and conditions.
    A signal is mapped to an expression which will evaluate to the value of the variable,
    shall the signal be fired.
*/

#include "BlockyXml.h"
#include "SelectionVariables.h"
#include "SelectionCondition.h"


class SelectionSignalVariables
{
public:
    bool LoadFromXML(const BlockyXmlBlocks::Ptr& blocks, const SelectionVariableDeclarations& variableDecls,
        const XmlNodeRef& rootNode, const char* scopeName, const char* fileName);

    bool ProcessSignal(const char* signalName, uint32 signalCRC, SelectionVariables& variables) const;

private:
    struct SignalVariable
    {
        SelectionCondition valueExpr;
        SelectionVariableID variableID;
    };

    typedef std::multimap<uint32, SignalVariable> SignalVariables;
    SignalVariables m_signalVariables;
};


#endif // CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_SELECTIONSIGNALVARIABLES_H
