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

// Description : Undo/Redo for entity param


#include "StdAfx.h"
#include "UndoEntityParam.h"
#include "EntityObject.h"

SPyWrappedProperty PyGetEntityParam(const char* pObjectName, const char* pVarPath);
void PySetEntityParam(const char* pObjectName, const char* pVarPath, SPyWrappedProperty value);

CUndoEntityParam::CUndoEntityParam(const char* pObjectName, const char* pVarPath, const QString& pUndoDescription)
{
    m_entityName = pObjectName;
    m_paramName = pVarPath;
    m_undo = PyGetEntityParam(pObjectName, pVarPath);
    m_undoDescription = pUndoDescription;
}

int CUndoEntityParam::GetSize()
{
    return sizeof(*this);
}

QString CUndoEntityParam::GetDescription()
{
    return m_undoDescription;
}

void CUndoEntityParam::Undo(bool bUndo)
{
    if (bUndo)
    {
        m_redo = PyGetEntityParam(m_entityName.toUtf8().data(), m_paramName.toUtf8().data());
    }
    PySetEntityParam(m_entityName.toUtf8().data(), m_paramName.toUtf8().data(), m_undo);
}

void CUndoEntityParam::Redo()
{
    PySetEntityParam(m_entityName.toUtf8().data(), m_paramName.toUtf8().data(), m_redo);
}
