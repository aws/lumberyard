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
#ifndef CRYINCLUDE_CRYACTION_PERSISTENTDEBUG_H
#define CRYINCLUDE_CRYACTION_PERSISTENTDEBUG_H
#pragma once

#include <list>
#include <map>

#include "IGameFramework.h"

class CPersistentDebug
    : public IPersistentDebug
    , public ILogCallback
{
public:
    CPersistentDebug();
    ~CPersistentDebug() override {}

    void Begin(const char* name, bool clear) override;
    void AddSphere(const Vec3& pos, float radius, ColorF clr, float timeout) override;
    void AddDirection(const Vec3& pos, float radius, const Vec3& dir, ColorF clr, float timeout) override;
    void AddLine(const Vec3& pos1, const Vec3& pos2, ColorF clr, float timeout) override;
    void AddPlanarDisc(const Vec3& pos, float innerRadius, float outerRadius, ColorF clr, float timeout) override;
    void AddCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, ColorF clr, float timeout) override;
    void AddCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, ColorF clr, float timeout) override;
    void AddQuat(const Vec3& pos, const Quat& q, float r, ColorF clr, float timeout) override;
    void AddAABB(const Vec3& min, const Vec3& max, ColorF clr, float timeout) override;

    void AddText(float x, float y, float fontSize, ColorF clr, float timeout, const char* fmt, ...) override;
    void Add2DText(const char* text, float fontSize, ColorF clr, float timeout) override;
    void Add2DLine(float x1, float y1, float x2, float y2, ColorF clr, float timeout) override;
    void Add2DCircle(float x, float y, float radius, ColorF clr, float timeout) override;
    void Add2DRect(float x, float y, float width, float height, ColorF clr, float timeout) override;

    void AddEntityTag(const SEntityTagParams& params, const char* tagContext = "") override;
    void ClearEntityTags(EntityId entityId) override;
    void ClearStaticTag(EntityId entityId, const char* staticId) override;
    void ClearTagContext(const char* tagContext) override;
    void ClearTagContext(const char* tagContext, EntityId entityId) override;

    void Reset() override;

    bool Init();
    void Update(float frameTime);
    void PostUpdate (float frameTime);


    static void VisualLogCallback(ICVar* pVar);

    //ILogCallback
    virtual void OnWrite(const char* sText, IMiniLog::ELogType type) override {};
    virtual void OnWriteToConsole(const char* sText, bool bNewLine) override;
    virtual void OnWriteToFile(const char* sText, bool bNewLine) override {}
    //~ILogCallback

private:

    IFFont* m_pDefaultFont;

    enum EObjType
    {
        eOT_Sphere,
        eOT_Arrow,
        eOT_Line,
        eOT_Disc,
        eOT_Text,
        eOT_Line2D,
        eOT_Circle2D,
        eOT_Rectangle2D,
        eOT_Quat,
        eOT_EntityTag,
        eOT_Cone,
        eOT_Cylinder,
        eOT_AABB
    };

    struct SEntityTag
    {
        SEntityTagParams params;
        Vec3 vScreenPos;
        float totalTime;
        float totalFadeTime;
    };
    typedef std::list<SEntityTag> TListTag;

    struct SColumn
    {
        float width;
        float height;
    };
    typedef std::vector<SColumn> TColumns;

    struct SObj
    {
        SObj()
            : q(IDENTITY) 
            , clr(0.0f, 0.0f, 0.0f, 0.0f) {}

        EObjType obj;
        ColorF clr;
        float timeRemaining;
        float totalTime;
        float radius;
        float radius2;
        Vec3 pos = ZERO;
        Vec3 dir = ZERO;
        Quat q = IDENTITY;
        string text;

        EntityId entityId;
        float entityHeight;
        TColumns columns;
        TListTag tags;
    };

    struct STextObj2D
    {
        STextObj2D()
            : clr(0.0f, 0.0f, 0.0f, 0.0f) 
            {}
        string text;
        ColorF clr;
        float size;
        float timeRemaining;
        float totalTime;
    };

    struct SObjFinder
    {
        SObjFinder(EntityId _entityId)
            : entityId(_entityId) {}
        bool operator()(const SObj& obj) {return obj.obj == eOT_EntityTag && obj.entityId == entityId; }
        EntityId entityId;
    };

    void UpdateTags(float frameTime, SObj& obj, bool doFirstPass = false);
    void PostUpdateTags(float frameTime, SObj& obj);
    void AddToTagList(TListTag& tagList, SEntityTag& tag);
    SObj* FindObj(EntityId entityId);
    bool GetEntityParams(EntityId entityId, Vec3& baseCenterPos, float& height);

    typedef std::list<SObj> ListObj;
    typedef std::map<string, ListObj> MapListObj;
    typedef std::list<STextObj2D> ListObjText2D;   // 2D objects need a separate pass, so we put it into another list

    static const char* entityTagsContext;
    MapListObj m_objects;
    MapListObj::iterator m_current;
    ListObjText2D m_2DTexts;

    // Pointers to Entity Tag console variables
    ICVar* m_pETLog;
    ICVar* m_pETHideAll;
    ICVar* m_pETHideBehaviour;
    ICVar* m_pETHideReadability;
    ICVar* m_pETHideAIDebug;
    ICVar* m_pETHideFlowgraph;
    ICVar* m_pETHideScriptBind;
    ICVar* m_pETFontSizeMultiplier;
    ICVar* m_pETMaxDisplayDistance;
    ICVar* m_pETColorOverrideEnable;
    ICVar* m_pETColorOverrideR;
    ICVar* m_pETColorOverrideG;
    ICVar* m_pETColorOverrideB;

    ICVar* m_pVisualConsole;
    ICVar* m_pVisualConsoleSubStr;
};

#endif // CRYINCLUDE_CRYACTION_PERSISTENTDEBUG_H
