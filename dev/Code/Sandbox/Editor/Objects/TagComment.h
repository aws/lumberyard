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

// Description : Special tag point for comment.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_TAGCOMMENT_H
#define CRYINCLUDE_EDITOR_OBJECTS_TAGCOMMENT_H
#pragma once

#include "EntityObject.h"

/*!
 *  CTagComment is an object that represent text commentary added to named 3d position in world.
 *
 */
class CTagComment
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CEntityObject.
    //////////////////////////////////////////////////////////////////////////
    virtual void InitVariables() {}
    bool IsScalable() { return false; }
    virtual void Serialize(CObjectArchive& ar);


    CTagComment();

    static const GUID& GetClassID()	
    {
        // {FAAA3955-EFE0-4888-85E8-C5481DC16FA5}
        static const GUID guid = {
            0xfaaa3955, 0xefe0, 0x4888, { 0x85, 0xe8, 0xc5, 0x48, 0x1d, 0xc1, 0x6f, 0xa5 }
        };
        return guid;
    }
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_TAGCOMMENT_H
