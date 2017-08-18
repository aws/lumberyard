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

// Description : TagPoint object definition.


#ifndef CRYINCLUDE_EDITOR_OBJECTS_TAGPOINT_H
#define CRYINCLUDE_EDITOR_OBJECTS_TAGPOINT_H
#pragma once


#include "EntityObject.h"

/*!
 *  CTagPoint is an object that represent named 3d position in world.
 *
 */
class CTagPoint
    : public CEntityObject
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    bool Init(IEditor* ie, CBaseObject* prev, const QString& file);
    void InitVariables();
    void Display(DisplayContext& disp);
    bool IsScalable() { return false; }

    void BeginEditParams(IEditor* ie, int flags);
    void EndEditParams(IEditor* ie);

    //! Called when object is being created.
    int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    bool HitTest(HitContext& hc);

    void GetLocalBounds(AABB& box);
    void GetBoundBox(AABB& box);
    //////////////////////////////////////////////////////////////////////////

    static const GUID& GetClassID()
    {
        // {7826D64A-080E-46cc-8C50-BA6A6CAE5175}
        static const GUID guid = {
            0x7826d64a, 0x80e, 0x46cc, { 0x8c, 0x50, 0xba, 0x6a, 0x6c, 0xae, 0x51, 0x75 }
        };
        return guid;
    }

    CTagPoint();

protected:
    float GetRadius();

    void DeleteThis() { delete this; };
};

/* Respawn point is a special tag point where player will be respawn at beginning or after death.
*/
class CRespawnPoint
    : public CTagPoint
{
    Q_OBJECT
public:
    CRespawnPoint();

    static const GUID& GetClassID()
    {
        // {03A22E8A-0AB8-41fe-8503-75687A8A50BC}
        static const GUID guid = {
            0x3a22e8a, 0xab8, 0x41fe, { 0x85, 0x3, 0x75, 0x68, 0x7a, 0x8a, 0x50, 0xbc }
        };
        return guid;
    }
};

class CSpawnPoint
    : public CTagPoint
{
    Q_OBJECT
public:
    CSpawnPoint();

    static const GUID& GetClassID()
    {
        // {03A22E8A-0AB8-41fe-8503-75687A8A50BC}
        // {888401F3-FD0E-4087-906A-27CF9E071999}
        static const GUID guid = {
            0x888401f3, 0xfd0e, 0x4087, { 0x90, 0x6a, 0x27, 0xcf, 0x9e, 0x7, 0x19, 0x99 }
        };
        return guid;
    }
};

/*
    NavigationSeedPoint is a special tag point used to generate the reachable
    part of a navigation mesh starting from his position
*/
class CNavigationSeedPoint
    : public CTagPoint
{
    Q_OBJECT
public:
    //////////////////////////////////////////////////////////////////////////
    // Overrides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    virtual void Display(DisplayContext& dc);
    virtual int MouseCreateCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags);
    virtual void Done();
    virtual void SetModified(bool boModifiedTransformOnly);

    //////////////////////////////////////////////////////////////////////////
    CNavigationSeedPoint();

    static const GUID& GetClassID()
    {
        // {6768DC77-9928-48FD-AB43-52CFF156CFE0}
        // {888401F3-FD0E-4087-906A-27CF9E071999}
        static const GUID guid = {
            0x6768dc77, 0x9928, 0x48fd, { 0xab, 0x43, 0x52, 0xcf, 0xf1, 0x56, 0xcf, 0xe0 }
        };
        return guid;
    }
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_TAGPOINT_H
