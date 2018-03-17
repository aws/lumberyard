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

// Description : Undo for Python function (PySetCVar)


#include "StdAfx.h"
#include "UndoCVar.h"

CUndoCVar::CUndoCVar(const char* pCVarName, const QString& pUndoDescription)
{
    m_CVarName = pCVarName;
    m_undoDescription = pUndoDescription;

    ICVar* pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(m_CVarName);
    if (pCVar)
    {
        if (pCVar->GetType() == CVAR_INT)
        {
            m_undo.type = SPyWrappedProperty::eType_Int;
            m_undo.property.intValue = pCVar->GetIVal();
        }
        else if (pCVar->GetType() == CVAR_FLOAT)
        {
            m_undo.type = SPyWrappedProperty::eType_Float;
            m_undo.property.floatValue = pCVar->GetFVal();
        }
        else if (pCVar->GetType() == CVAR_STRING)
        {
            m_undo.type = SPyWrappedProperty::eType_String;
            m_undo.stringValue = pCVar->GetString();
        }
    }
}

int CUndoCVar::GetSize()
{
    return sizeof(*this);
}

QString CUndoCVar::GetDescription()
{
    return m_undoDescription;
}

void CUndoCVar::Undo(bool bUndo)
{
    ICVar* pTempCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(m_CVarName);
    if (pTempCVar)
    {
        if (bUndo)
        {
            if (pTempCVar->GetType() == CVAR_INT && m_undo.type == SPyWrappedProperty::eType_Int)
            {
                m_redo.type = SPyWrappedProperty::eType_Int;
                m_redo.property.intValue = pTempCVar->GetIVal();
            }
            else if (pTempCVar->GetType() == CVAR_FLOAT && m_undo.type == SPyWrappedProperty::eType_Float)
            {
                m_redo.type = SPyWrappedProperty::eType_Float;
                m_redo.property.intValue = pTempCVar->GetFVal();
            }
            else if (pTempCVar->GetType() == CVAR_STRING && m_undo.type == SPyWrappedProperty::eType_String)
            {
                m_redo.type = SPyWrappedProperty::eType_String;
                m_redo.stringValue = pTempCVar->GetString();
            }
        }

        if (pTempCVar->GetType() == CVAR_INT && m_undo.type == SPyWrappedProperty::eType_Int)
        {
            pTempCVar->Set(m_undo.property.intValue);
        }
        else if (pTempCVar->GetType() == CVAR_FLOAT && m_undo.type == SPyWrappedProperty::eType_Float)
        {
            pTempCVar->Set(m_undo.property.floatValue);
        }
        else if (pTempCVar->GetType() == CVAR_STRING && m_undo.type == SPyWrappedProperty::eType_String)
        {
            pTempCVar->Set(m_undo.stringValue.toUtf8().data());
        }
    }
}

void CUndoCVar::Redo()
{
    ICVar* pTempCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(m_CVarName);
    if (pTempCVar)
    {
        if (pTempCVar->GetType() == CVAR_INT && m_redo.type == SPyWrappedProperty::eType_Int)
        {
            pTempCVar->Set(m_redo.property.intValue);
        }
        else if (pTempCVar->GetType() == CVAR_FLOAT && m_undo.type == SPyWrappedProperty::eType_Float)
        {
            pTempCVar->Set(m_redo.property.floatValue);
        }
        else if (pTempCVar->GetType() && m_undo.type == SPyWrappedProperty::eType_String)
        {
            pTempCVar->Set(m_redo.stringValue.toUtf8().data());
        }
    }
}