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

// Description : Object class that stores group of events


#ifndef CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYEVENTTIMELINE_H
#define CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYEVENTTIMELINE_H
#pragma once


//////////////////////////////////////////////////////////////////////////
#include "TelemetryObjects.h"
#include "Util/XmlArchive.h"
#include "Util/Variable.h"
#include "Objects/BaseObject.h"
#include "TelemetryRepository.h"
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////

class CDMLightCache
{
public:
    CDMLightCache();
    ~CDMLightCache();
    void SetLight(float intensity, const Vec3& pos, CObjectLayer* pLayer);
    void ClearLights();

private:
    std::vector<CBaseObject*> m_lights;
    size_t m_lastUsed;
};

//////////////////////////////////////////////////////////////////////////

class CTelemetryEventTimeline
    : public CBaseObject
{
public:
    DECLARE_DYNCREATE(CTelemetryEventTimeline)

    // BaseObject
    virtual void Display(DisplayContext& dc);
    virtual void GetBoundBox(AABB& box);
    virtual void GetLocalBounds(AABB& box);
    virtual bool HitTest(HitContext& hc);
    virtual bool HitTestRect(HitContext& hc);
    virtual void SetSelected(bool bSelect);

    // Own methods
    void FinalConstruct(CTelemetryRepository* repo);

    void SetVariables(const STelemetryEvent* e);

protected:
    CTelemetryEventTimeline();
    ~CTelemetryEventTimeline();
    virtual void DeleteThis();

private:
    typedef std::vector<CSmartVariable<CString> > TVarVector;

    CTelemetryRepository* m_repository;
    CTelemetryRepository::TTelemOctree* m_octree;
    STelemetryEvent* m_highlightedEvent;
    STelemetryEvent* m_selectedEvent;
    CDMLightCache m_lightCache;
    TVarVector m_variables;
};

//////////////////////////////////////////////////////////////////////////

class CTelemetryEventTimelineClassDesc
    : public CObjectClassDesc
{
public:
    REFGUID ClassID()
    {
        // {a00e1307-dbda-4a6c-a8ef-c85c7ae140dc}
        static const GUID guid = {
            0xa00e1307, 0xdbda, 0x4a6c, { 0xa8, 0xef, 0xc8, 0x5c, 0x7a, 0xe1, 0x40, 0xdc }
        };
        return guid;
    }
    ObjectType GetObjectType() { return OBJTYPE_TELEMETRY; };
    const char* ClassName() { return "TelemetryEventTimeline"; };
    const char* Category() { return "Misc"; };
    CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTelemetryEventTimeline); };
    int GameCreationOrder() { return 500; };
    const char* GetTextureIcon() { return "Editor/ObjectIcons/sequence.bmp"; };
};

//////////////////////////////////////////////////////////////////////////

class CMarkerDrawingVisitor
    : public IOctreeVisitor<STelemetryEvent*, STelemOctreeAgent>
{
public:
    CMarkerDrawingVisitor(const Vec3& camera, DisplayContext& dc, int textureId, STelemetryEvent* highlighted, STelemetryEvent* selected);
    virtual bool EnterBranch(branch_type& branch);
    virtual void LeaveBranch(branch_type& branch);
    virtual void VisitLeaf(leaf_type& leaf);
    void DrawEvent(const STelemetryEvent& event);

private:
    Vec3 m_camPos;
    DisplayContext& m_dc;
    COLORREF m_colors[eStatOwner_Num];
    int m_texture;
    size_t m_clampDepth;
    STelemetryEvent* m_selected;
    STelemetryEvent* m_highlighted;
};

//////////////////////////////////////////////////////////////////////////

class CDensityDrawingVisitor
    : public IOctreeVisitor<STelemetryEvent*, STelemOctreeAgent>
{
public:
    CDensityDrawingVisitor(CDMLightCache& lights, size_t maxDensity, CObjectLayer* pLayer);
    virtual bool EnterBranch(branch_type& branch);
    virtual void LeaveBranch(branch_type& branch);
    virtual void VisitLeaf(leaf_type& leaf);

private:
    CDMLightCache& m_lights;
    size_t m_maxDensity;
    CObjectLayer* m_pLayer;
};

//////////////////////////////////////////////////////////////////////////

class COctreeDensityVisitor
    : public IOctreeVisitor<STelemetryEvent*, STelemOctreeAgent>
{
public:
    COctreeDensityVisitor();
    virtual bool EnterBranch(branch_type& branch);
    virtual void LeaveBranch(branch_type& branch);
    virtual void VisitLeaf(leaf_type& leaf);
    size_t getMaxDensity() const { return m_maxDensity; }

private:
    size_t m_maxDensity;
};

//////////////////////////////////////////////////////////////////////////

class COctreeHitTestVisitor
    : public IOctreeVisitor<STelemetryEvent*, STelemOctreeAgent>
{
public:
    COctreeHitTestVisitor(const HitContext& hc);
    virtual bool EnterBranch(branch_type& branch);
    virtual void LeaveBranch(branch_type& branch);
    virtual void VisitLeaf(leaf_type& leaf);
    STelemetryEvent* getHit() const;

private:
    const HitContext& m_hitContext;
    Ray m_ray;
    STelemetryEvent* m_hitEvent;
    float m_cachedDistSqr;
};

//////////////////////////////////////////////////////////////////////////

#endif // CRYINCLUDE_TELEMETRYPLUGIN_TELEMETRYEVENTTIMELINE_H
