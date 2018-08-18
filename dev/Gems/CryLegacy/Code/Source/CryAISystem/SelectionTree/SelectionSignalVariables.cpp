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
#include "SelectionSignalVariables.h"

#include "CryCrc32.h"


bool SelectionSignalVariables::LoadFromXML(const BlockyXmlBlocks::Ptr& blocks,
    const SelectionVariableDeclarations& variableDecls, const XmlNodeRef& rootNode,
    const char* scopeName, const char* fileName)
{
    BlockyXmlNodeRef blockyNode(blocks, scopeName, rootNode, fileName);
    while (XmlNodeRef childNode = blockyNode.next())
    {
        if (!_stricmp(childNode->getTag(), "Signal"))
        {
            const char* signalName = 0;
            if (childNode->haveAttr("name"))
            {
                childNode->getAttr("name", &signalName);
            }
            else
            {
                AIWarning("Missing 'name' attribute for tag '%s' in file '%s' at line %d.",
                    childNode->getTag(), fileName, childNode->getLine());

                return false;
            }

            const char* variableName = 0;
            if (childNode->haveAttr("variable"))
            {
                childNode->getAttr("variable", &variableName);
            }
            else
            {
                AIWarning("Missing 'variable' attribute for tag '%s' in file '%s' at line %d.",
                    childNode->getTag(), fileName, childNode->getLine());

                return false;
            }

            const char* value = 0;
            if (childNode->haveAttr("value"))
            {
                childNode->getAttr("value", &value);
            }
            else
            {
                AIWarning("Missing 'value' attribute for tag '%s' in file '%s' at line %d.",
                    childNode->getTag(), fileName, childNode->getLine());

                return false;
            }

            SelectionVariableID variableID = variableDecls.GetVariableID(variableName);
            if (variableDecls.IsDeclared(variableID))
            {
                SignalVariables::iterator it = m_signalVariables.insert(
                        SignalVariables::value_type(CCrc32::Compute(signalName), SignalVariable()));

                SignalVariable& signalVariable = it->second;

                signalVariable.valueExpr = SelectionCondition(value, variableDecls);
                signalVariable.variableID = variableID;

                if (!signalVariable.valueExpr.Valid())
                {
                    AIWarning("Failed to compile expression '%s' in file '%s' at line %d.",
                        value, fileName, childNode->getLine());

                    return false;
                }
            }
            else
            {
                AIWarning("Unknown variable '%s' used for signal variable in file '%s' at line '%d'.",
                    variableName, fileName, childNode->getLine());

                return false;
            }
        }
        else
        {
            AIWarning("Unexpected tag '%s' in file '%s' at line %d. 'Signal' expected.",
                childNode->getTag(), fileName, childNode->getLine());

            return false;
        }
    }

    return true;
}

bool SelectionSignalVariables::ProcessSignal(const char* signalName, uint32 signalCRC, SelectionVariables& variables) const
{
    SignalVariables::const_iterator it = m_signalVariables.find(signalCRC);
    if (it == m_signalVariables.end())
    {
        return false;
    }

    while ((it != m_signalVariables.end()) && (it->first == signalCRC))
    {
        const SignalVariable& signalVariable = it->second;
        variables.SetVariable(signalVariable.variableID, signalVariable.valueExpr.Evaluate(variables));

        ++it;
    }

    return true;
}