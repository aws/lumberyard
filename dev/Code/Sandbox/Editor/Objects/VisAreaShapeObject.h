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

#ifndef CRYINCLUDE_EDITOR_OBJECTS_VISAREASHAPEOBJECT_H
#define CRYINCLUDE_EDITOR_OBJECTS_VISAREASHAPEOBJECT_H
#pragma once

#include "ShapeObject.h"

//////////////////////////////////////////////////////////////////////////
class C3DEngineAreaObjectBase
    : public CShapeObject
{
    Q_OBJECT
public:
    C3DEngineAreaObjectBase();
    virtual void Done();
    virtual XmlNodeRef Export(const QString& levelPath, XmlNodeRef& xmlNode);
    virtual bool CreateGameObject();
    virtual void Validate(CErrorReport* report)
    {
        CBaseObject::Validate(report);
    }

protected:
    virtual bool IsOnlyUpdateOnUnselect() const { return false; }

    struct IVisArea* m_area;
    struct IRamArea* m_RamArea;
};

/** Represent Visibility Area, visibility areas can be connected with portals.
*/
class CVisAreaShapeObject
    : public C3DEngineAreaObjectBase
{
    Q_OBJECT
public:
    CVisAreaShapeObject();

    static const GUID& GetClassID()
    {
        // {31B912A0-D351-4abc-AA8F-B819CED21231}
        static const GUID guid = {
            0x31b912a0, 0xd351, 0x4abc, { 0xaa, 0x8f, 0xb8, 0x19, 0xce, 0xd2, 0x12, 0x31 }
        };
        return guid;
    }

    //////////////////////////////////////////////////////////////////////////
    // Ovverides from CBaseObject.
    //////////////////////////////////////////////////////////////////////////
    virtual void InitVariables();
    //////////////////////////////////////////////////////////////////////////

protected:
    virtual void UpdateGameArea(bool bRemove = false);

    // Visibility area in 3d engine.
    CVariable<Vec3>  mv_vAmbientColor;
    CVariable<bool>  mv_bAffectedBySun;
    CVariable<bool>  mv_bIgnoreSkyColor;
    CVariable<float> mv_fViewDistRatio;
    CVariable<bool>  mv_bSkyOnly;
    CVariable<bool>  mv_bOceanIsVisible;
    CVariable<bool>  mv_bIgnoreGI;
};

/** Represent Portal Area.
        Portal connect visibility areas, visibility between vis areas are only done with portals.
*/
class CPortalShapeObject
    : public CVisAreaShapeObject
{
    Q_OBJECT
public:
    CPortalShapeObject();
    static const GUID& GetClassID()
    {
        // {D8665C26-AE2D-482b-9574-B6E2C9253E40}
        static const GUID guid = {
            0xd8665c26, 0xae2d, 0x482b, { 0x95, 0x74, 0xb6, 0xe2, 0xc9, 0x25, 0x3e, 0x40 }
        };
        return guid;
    }
    void InitVariables();

protected:
    virtual int GetMaxPoints() const { return 4; };
    virtual void UpdateGameArea(bool bRemove = false);

    CVariable<bool>  mv_bUseDeepness;
    CVariable<bool>  mv_bDoubleSide;
    CVariable<bool>  mv_bPortalBlending;
    CVariable<float> mv_fPortalBlendValue;
};

/** Represent Occluder Area.
Areas that occlude objects behind it.
*/
class COccluderPlaneObject
    : public C3DEngineAreaObjectBase
{
    Q_OBJECT
public:
    COccluderPlaneObject();

    static const GUID& GetClassID()
    {
        // {76D000C3-47E8-420a-B4C9-17F698C1A607}
        static const GUID guid = {
            0x76d000c3, 0x47e8, 0x420a, { 0xb4, 0xc9, 0x17, 0xf6, 0x98, 0xc1, 0xa6, 0x7 }
        };
        return guid;
    }

    void InitVariables();
    void PostLoad(CObjectArchive& ar) override;

protected:
    virtual bool IsOnlyUpdateOnUnselect() const { return false; }
    virtual int GetMinPoints() const { return 2; };
    virtual int GetMaxPoints() const { return 2; };
    virtual void UpdateGameArea(bool bRemove = false);

    CVariable<bool>  mv_bUseInIndoors;
    CVariable<bool>  mv_bDoubleSide;
    CVariable<float> mv_fViewDistRatio;
};

/** Represent Occluder Plane.
Areas that occlude objects behind it.
*/
class COccluderAreaObject
    : public C3DEngineAreaObjectBase
{
    Q_OBJECT
public:
    COccluderAreaObject();

    static const GUID& GetClassID()
    {
        // {ca242d20-f233-48f5-9d10-b51194bbdf7e}
        static const GUID guid = {
            0xca242d20, 0xf233, 0x48f5, { 0x9d, 0x10, 0xb5, 0x11, 0x94, 0xbb, 0xdf, 0x7e }
        };
        return guid;
    }

    void InitVariables();
    void PostLoad(CObjectArchive& ar) override;

protected:
    virtual bool IsOnlyUpdateOnUnselect() const { return false; }
    virtual int GetMinPoints() const { return 4; };
    virtual int GetMaxPoints() const { return 4; };
    virtual void UpdateGameArea(bool bRemove = false);

    CVariable<bool>  mv_bUseInIndoors;
    CVariable<float> mv_fViewDistRatio;
};

#endif // CRYINCLUDE_EDITOR_OBJECTS_VISAREASHAPEOBJECT_H
