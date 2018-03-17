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

// Description : Undo/Redo for entity property


#include "StdAfx.h"
#include "UndoEntityProperty.h"
#include "EntityObject.h"

SPyWrappedProperty PyGetEntityProperty(const char* entityName, const char* propName);
void PySetEntityProperty(const char* entityName, const char* propName, SPyWrappedProperty value);

CUndoEntityProperty::CUndoEntityProperty(const char* pEntityName, const char* pPropertyName, const QString& pUndoDescription)
{
    m_entityName = pEntityName;
    m_propertyName = pPropertyName;
    m_undoDescription = pUndoDescription;
    m_undo = PyGetEntityProperty(pEntityName, pPropertyName);
}

int CUndoEntityProperty::GetSize()
{
    return sizeof(*this);
}

QString CUndoEntityProperty::GetDescription()
{
    return m_undoDescription;
}



void CUndoEntityProperty::Undo(bool bUndo)
{
    if (bUndo)
    {
        m_redo = PyGetEntityProperty(m_entityName.toUtf8().data(), m_propertyName.toUtf8().data());
    }
    PySetEntityProperty(m_entityName.toUtf8().data(), m_propertyName.toUtf8().data(), m_undo);
}

void CUndoEntityProperty::Redo()
{
    PySetEntityProperty(m_entityName.toUtf8().data(), m_propertyName.toUtf8().data(), m_redo);
}
