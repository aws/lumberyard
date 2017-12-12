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

#ifndef CRYINCLUDE_CRYCOMMON_IAIOBJECTMANAGER_H
#define CRYINCLUDE_CRYCOMMON_IAIOBJECTMANAGER_H
#pragma once



#include <IAIObject.h> // <> required for Interfuscator


/// Filter flags for IAISystem::GetFirstAIObject
enum EGetFirstFilter
{
    OBJFILTER_TYPE,         // Include only objects of specified type (type 0 means all objects).
    OBJFILTER_GROUP,        // Include only objects of specified group id.
    OBJFILTER_FACTION,  // Include only objects of specified faction.
    OBJFILTER_DUMMYOBJECTS, // Include only dummy objects
};

struct IAIObjectManager
{
    // <interfuscator:shuffle>
    virtual ~IAIObjectManager(){}
    virtual IAIObject* CreateAIObject(const AIObjectParams& params) = 0;
    virtual void RemoveObject(tAIObjectID objectID) = 0;//same as destroy??

    virtual IAIObject* GetAIObject(tAIObjectID aiObjectID) = 0;
    virtual IAIObject* GetAIObjectByName(unsigned short type, const char* pName) const = 0;


    // Returns AIObject iterator for first match, see EGetFirstFilter for the filter options.
    // The parameter 'n' specifies the type, group id or species based on the selected filter.
    // It is possible to iterate over all objects by setting the filter to OBJFILTER_TYPE
    // passing zero to 'n'.
    virtual IAIObjectIter* GetFirstAIObject(EGetFirstFilter filter, short n) = 0;
    // Iterates over AI objects within specified range.
    // Parameter 'pos' and 'rad' specify the enclosing sphere, for other parameters see GetFirstAIObject.
    virtual IAIObjectIter* GetFirstAIObjectInRange(EGetFirstFilter filter, short n, const Vec3& pos, float rad, bool check2D) = 0;
    // </interfuscator:shuffle>
};


#endif // CRYINCLUDE_CRYCOMMON_IAIOBJECTMANAGER_H
