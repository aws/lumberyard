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
#include "SafeObjectsArray.h"

//////////////////////////////////////////////////////////////////////////
CSafeObjectsArray::~CSafeObjectsArray()
{
    Clear();
}

//////////////////////////////////////////////////////////////////////////
void CSafeObjectsArray::Clear()
{
    int num = m_objects.size();
    for (int i = 0; i < num; i++)
    {
        m_objects[i]->RemoveEventListener(functor(*this, &CSafeObjectsArray::OnTargetEvent));
    }
    m_objects.clear();
}

//////////////////////////////////////////////////////////////////////////
void CSafeObjectsArray::Add(CBaseObject* obj)
{
    // Not add NULL object.
    if (!obj)
    {
        return;
    }

    // Check if object is unique in array.
    if (!stl::find(m_objects, obj))
    {
        m_objects.push_back(obj);
        // Make reference on this object.
        obj->AddEventListener(functor(*this, &CSafeObjectsArray::OnTargetEvent));
    }
}

//////////////////////////////////////////////////////////////////////////
void CSafeObjectsArray::Remove(CBaseObject* obj)
{
    // Find this object.
    if (stl::find_and_erase(m_objects, obj))
    {
        obj->RemoveEventListener(functor(*this, &CSafeObjectsArray::OnTargetEvent));
    }
}

//////////////////////////////////////////////////////////////////////////
CBaseObject* CSafeObjectsArray::Get(int index) const
{
    assert(index >= 0 && index < m_objects.size());
    return m_objects[index];
}

//////////////////////////////////////////////////////////////////////////
void CSafeObjectsArray::OnTargetEvent(CBaseObject* target, int event)
{
    if (event == CBaseObject::ON_DELETE)
    {
        Remove(target);
    }
}