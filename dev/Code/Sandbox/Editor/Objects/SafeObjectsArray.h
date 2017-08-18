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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_SAFEOBJECTSARRAY_H
#define CRYINCLUDE_EDITOR_OBJECTS_SAFEOBJECTSARRAY_H
#pragma once

#include "BaseObject.h"

/** This class used as a safe collction of references to CBaseObject instances.
        Target objects in this collection can be safely removed or added,
        This object makes references to added objects and recieve back event when theose objects are deleted.
*/
class CSafeObjectsArray
{
public:
    CSafeObjectsArray() {};
    ~CSafeObjectsArray();

    void Add(CBaseObject* obj);
    void Remove(CBaseObject* obj);

    bool IsEmpty() const { return m_objects.empty(); }
    int GetCount() const { return m_objects.size(); }
    CBaseObject* Get(int index) const;

    // Clear array.
    void Clear();

private:
    void OnTargetEvent(CBaseObject* target, int event);

    //////////////////////////////////////////////////////////////////////////
    std::vector<CBaseObjectPtr> m_objects;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_SAFEOBJECTSARRAY_H
