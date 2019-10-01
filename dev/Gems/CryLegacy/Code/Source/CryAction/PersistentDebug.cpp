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
#include "CryLegacy_precompiled.h"
#include "PersistentDebug.h"
#include "CryAction.h"
#include <IRenderAuxGeom.h>
#include <LyShine/IDraw2d.h>
#include <ILocalizationManager.h>


////////////////////////////////////////////////////////////////////////////////////////////////////
// LOCAL STATIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
// Generates a circle texture
static ITexture* GenerateCircleTexture()
{
    const uint32 blank = 0x00000000;
    const uint32 white = 0xffffffff;

    const int texSize = 128;
    const int sizeHalf = texSize / 2;
    const int maxDistance = sizeHalf * sizeHalf;

    uint32 data[texSize * texSize];

    for (int y = 0; y < texSize; ++y)
    {
        int posY = y * texSize;
        int diffY = (sizeHalf - y) * (sizeHalf - y);

        for (int x = 0; x < texSize; ++x)
        {
            int index = posY + x;

            int diffX = (sizeHalf - x) * (sizeHalf - x);
            int lengthSquared = diffX + diffY;

            data[index] = ((lengthSquared > maxDistance) ? blank : white);
        }
    }

    IRenderer* renderer = gEnv->pRenderer;
    int textureId = renderer->DownLoadToVideoMemory((uint8*)data, texSize, texSize, eTF_R8G8B8A8, eTF_R8G8B8A8, 1);
    return gEnv->pRenderer->EF_GetTextureByID(textureId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Gets the code generated circle texture
static ITexture* GetCircleTexture()
{
    static ITexture* s_circleTexture = nullptr;
    if (!s_circleTexture)
    {
        s_circleTexture = GenerateCircleTexture();
    }
    return s_circleTexture;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Generates a square texture
static ITexture* GenerateSquareTexture()
{
    const uint32 white = 0xffffffff;

    const int texSize = 64;
    const int texSizeTotal = texSize * texSize;
    uint32 data[texSizeTotal];

    for (int index = 0; index < texSizeTotal; ++index)
    {
        data[index] = white;
    }

    IRenderer* renderer = gEnv->pRenderer;
    int textureId = renderer->DownLoadToVideoMemory((uint8*)data, texSize, texSize, eTF_R8G8B8A8, eTF_R8G8B8A8, 1);
    return gEnv->pRenderer->EF_GetTextureByID(textureId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Gets the code generated square texture
static ITexture* GetSquareTexture()
{
    static ITexture* s_squareTexture = nullptr;
    if (!s_squareTexture)
    {
        s_squareTexture = GenerateSquareTexture();
    }
    return s_squareTexture;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// CPersistentDebug FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////
CPersistentDebug::CPersistentDebug()
{
    if (gEnv->pCryFont)
    {
        m_pDefaultFont = gEnv->pCryFont->GetFont("default");
        CRY_ASSERT(m_pDefaultFont);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::Begin(const char* name, bool clear)
{
    if (clear)
    {
        m_objects[name].clear();
    }
    else
    {
        m_objects[name];
    }
    m_current = m_objects.find(name);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::AddSphere(const Vec3& pos, float radius, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Sphere;
    obj.clr = clr;
    obj.pos = pos;
    obj.radius = radius;
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::AddDirection(const Vec3& pos, float radius, const Vec3& dir, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Arrow;
    obj.clr = clr;
    obj.pos = pos;
    obj.radius = radius;
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    obj.dir = dir.GetNormalizedSafe();
    if (obj.dir.GetLengthSquared() > 0.001f)
    {
        m_current->second.push_back(obj);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::AddLine(const Vec3& pos1, const Vec3& pos2, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Line;
    obj.clr = clr;
    obj.pos = pos1;
    obj.dir = pos2 - pos1;
    obj.radius = 0.0f;
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::AddPlanarDisc(const Vec3& pos, float innerRadius, float outerRadius, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Disc;
    obj.clr = clr;
    obj.pos = pos;
    obj.radius = std::max(0.0f, std::min(innerRadius, outerRadius));
    obj.radius2 = std::min(100.0f, std::max(0.0f, std::max(innerRadius, outerRadius)));
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    if (obj.radius2)
    {
        m_current->second.push_back(obj);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::AddCone(const Vec3& pos, const Vec3& dir, float baseRadius, float height, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Cone;
    obj.clr = clr;
    obj.pos = pos;
    obj.dir = dir;
    obj.radius = std::max(0.001f, baseRadius);
    obj.radius2 = std::max(0.001f, height);
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::AddCylinder(const Vec3& pos, const Vec3& dir, float radius, float height, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Cylinder;
    obj.clr = clr;
    obj.pos = pos;
    obj.dir = dir;
    obj.radius = std::max(0.001f, radius);
    obj.radius2 = std::max(0.001f, height);
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::AddQuat(const Vec3& pos, const Quat& q, float r, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Quat;
    obj.clr = clr;
    obj.pos = pos;
    obj.q = q;
    obj.radius = r;
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::AddAABB(const Vec3& min, const Vec3& max, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_AABB;
    obj.clr = clr;
    obj.pos = min;
    obj.dir = max;
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::AddText(float x, float y, float fontSize, ColorF clr, float timeout, const char* fmt, ...)
{
    char buffer[4096];
    va_list args;
    va_start(args, fmt);
    vsprintf_s(buffer, fmt, args);
    va_end(args);

    SObj obj;
    obj.obj = eOT_Text;
    obj.clr = clr;
    obj.pos.x = x;
    obj.pos.y = y;
    obj.radius = fontSize;
    obj.text = buffer;
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::Add2DText(const char* text, float fontSize, ColorF clr, float timeout)
{
    if (text == 0 || *text == '\0')
    {
        return;
    }

    STextObj2D obj;
    obj.clr = clr;
    obj.text = text;
    obj.size = fontSize;
    obj.timeRemaining = timeout > 0.0f ? timeout : .1f;
    obj.totalTime = obj.timeRemaining;
    m_2DTexts.push_front(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::Add2DLine(float x1, float y1, float x2, float y2, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Line2D;
    obj.pos.x = x1;
    obj.pos.y = y1;
    obj.pos.z = 0;
    obj.dir.x = x2;
    obj.dir.y = y2;
    obj.dir.z = 0;
    obj.clr = clr;
    obj.timeRemaining = timeout > 0.0f ? timeout : .1f;
    obj.totalTime = obj.timeRemaining;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::Add2DCircle(float x, float y, float radius, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Circle2D;
    obj.clr = clr;
    obj.radius = obj.radius2 = std::max(0.001f, radius);
    obj.pos.x = x - obj.radius;
    obj.pos.y = y - obj.radius;
    obj.pos.z = 0;
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::Add2DRect(float x, float y, float width, float height, ColorF clr, float timeout)
{
    SObj obj;
    obj.obj = eOT_Rectangle2D;
    obj.clr = clr;
    obj.pos.x = x;
    obj.pos.y = y;
    obj.pos.z = 0;
    obj.radius = std::max(0.001f, width);
    obj.radius2 = std::max(0.001f, height);
    obj.timeRemaining = timeout;
    obj.totalTime = timeout;
    m_current->second.push_back(obj);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::Reset()
{
    m_2DTexts.clear();
    m_objects.clear();
    Begin("default", true);   // Safety context if first caller forgets to use Begin(), so we don't crash!
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool CPersistentDebug::Init()
{
    // Init CVars
    m_pETLog = REGISTER_INT("cl_ETLog", 0, VF_DUMPTODISK, "Logging (0=off, 1=editor.log, 2=editor.log + AIlog.log)");
    m_pETHideAll = REGISTER_INT("cl_ETHideAll", 0, VF_DUMPTODISK, "Hide all tags (overrides all other options)");
    m_pETHideBehaviour = REGISTER_INT("cl_ETHideBehaviour", 0, VF_DUMPTODISK, "Hide AI behavior tags");
    m_pETHideReadability = REGISTER_INT("cl_ETHideReadability", 0, VF_DUMPTODISK, "Hide AI readability tags");
    m_pETHideAIDebug = REGISTER_INT("cl_ETHideAIDebug", 0, VF_DUMPTODISK, "Hide AI debug tags");
    m_pETHideFlowgraph = REGISTER_INT("cl_ETHideFlowgraph", 0, VF_DUMPTODISK, "Hide tags created by flowgraph");
    m_pETHideScriptBind = REGISTER_INT("cl_ETHideScriptBind", 0, VF_DUMPTODISK, "Hide tags created by Lua script");
    m_pETFontSizeMultiplier = REGISTER_FLOAT("cl_ETFontSizeMultiplier", 1.0f, VF_DUMPTODISK, "Global font size multiplier");
    m_pETMaxDisplayDistance = REGISTER_FLOAT("cl_ETMaxDisplayDistance", -2.0f, VF_DUMPTODISK, "Max display distance");
    m_pETColorOverrideEnable = REGISTER_INT("cl_ETColorOverrideEnable", 0, VF_DUMPTODISK, "Global color override");
    m_pETColorOverrideR = REGISTER_FLOAT("cl_ETColorOverrideR", 1.0f, VF_DUMPTODISK, "Global color override (RED)");
    m_pETColorOverrideG = REGISTER_FLOAT("cl_ETColorOverrideG", 1.0f, VF_DUMPTODISK, "Global color override (GREEN)");
    m_pETColorOverrideB = REGISTER_FLOAT("cl_ETColorOverrideB", 1.0f, VF_DUMPTODISK, "Global color override (BLUE)");

    m_pVisualConsole = REGISTER_INT_CB("VisualConsole", 0, VF_NULL, "writes console to screen", VisualLogCallback);
    if (m_pVisualConsole)
    {
        m_pVisualConsoleSubStr = REGISTER_STRING("VisualConsoleSubStr", "", VF_NULL, "writes console to screen if it matches substr");
    }

    Reset();

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::Update(float frameTime)
{
    if (m_objects.empty())
    {
        return;
    }

    IRenderAuxGeom* pAux = gEnv->pRenderer->GetIRenderAuxGeom();
    static const int flags3D = e_Mode3D | e_AlphaBlended | e_DrawInFrontOff | e_FillModeSolid | e_CullModeBack | e_DepthWriteOn | e_DepthTestOn;

    IDraw2d* draw2d = Draw2dHelper::GetDraw2d();
    if (!draw2d)
    {
        return;
    }

    IDraw2d::TextOptions textOptions = draw2d->GetDefaultTextOptions();
    IDraw2d::ImageOptions imageOptions = draw2d->GetDefaultImageOptions();

    std::vector<ListObj::iterator> toClear;
    std::vector<MapListObj::iterator> toClearMap;
    for (MapListObj::iterator iterMap = m_objects.begin(); iterMap != m_objects.end(); ++iterMap)
    {
        toClear.resize(0);
        for (ListObj::iterator iterList = iterMap->second.begin(); iterList != iterMap->second.end(); ++iterList)
        {
            iterList->timeRemaining -= frameTime;
            if (iterList->timeRemaining <= 0.0f && !(iterList->obj == eOT_EntityTag && iterList->columns.size() > 1))
            {
                toClear.push_back(iterList);
            }
            else
            {
                ColorF clr = iterList->clr;
                imageOptions.color = textOptions.color = AZ::Vector3(clr.r, clr.g, clr.b);
                clr.a *= iterList->timeRemaining / iterList->totalTime;

                switch (iterList->obj)
                {
                case eOT_Sphere:
                    pAux->SetRenderFlags(flags3D);
                    pAux->DrawSphere(iterList->pos, iterList->radius, clr);
                    break;
                case eOT_Quat:
                    pAux->SetRenderFlags(flags3D);
                    {
                        float r = iterList->radius;
                        Vec3 x = r * iterList->q.GetColumn0();
                        Vec3 y = r * iterList->q.GetColumn1();
                        Vec3 z = r * iterList->q.GetColumn2();
                        Vec3 p = iterList->pos;
                        OBB obb = OBB::CreateOBB(Matrix33::CreateIdentity(), Vec3(0.05f, 0.05f, 0.05f), ZERO);
                        pAux->DrawOBB(obb, p, false, clr, eBBD_Extremes_Color_Encoded);
                        pAux->DrawLine(p, ColorF(1, 0, 0, clr.a), p + x, ColorF(1, 0, 0, clr.a));
                        pAux->DrawLine(p, ColorF(0, 1, 0, clr.a), p + y, ColorF(0, 1, 0, clr.a));
                        pAux->DrawLine(p, ColorF(0, 0, 1, clr.a), p + z, ColorF(0, 0, 1, clr.a));
                    }
                    break;
                case eOT_Arrow:
                    pAux->SetRenderFlags(flags3D);
                    pAux->DrawLine(iterList->pos - iterList->dir * iterList->radius, clr, iterList->pos + iterList->dir * iterList->radius, clr);
                    pAux->DrawCone(iterList->pos + iterList->dir * iterList->radius, iterList->dir, 0.1f * iterList->radius, 0.3f * iterList->radius, clr);
                    break;
                case eOT_Line:
                    pAux->SetRenderFlags(flags3D);
                    pAux->DrawLine(iterList->pos, clr, iterList->pos + iterList->dir, clr);
                    break;
                case eOT_Cone:
                    pAux->SetRenderFlags(flags3D);
                    pAux->DrawCone(iterList->pos, iterList->dir, iterList->radius, iterList->radius2, clr);
                    break;
                case eOT_Cylinder:
                    pAux->SetRenderFlags(flags3D);
                    pAux->DrawCylinder(iterList->pos, iterList->dir, iterList->radius, iterList->radius2, clr);
                    break;
                case eOT_AABB:
                    pAux->SetRenderFlags(flags3D);
                    pAux->DrawAABB(AABB(iterList->pos, iterList->dir), Matrix34(IDENTITY), false, clr, eBBD_Faceted);
                    break;
                case eOT_Disc:
                {
                    pAux->SetRenderFlags(flags3D);
                    vtx_idx indTriQuad[ 6 ] =
                    {
                        0, 2, 1,
                        0, 3, 2
                    };
                    vtx_idx indTriTri[ 3 ] =
                    {
                        0, 1, 2
                    };

                    int steps = (int)(10 * iterList->radius2);
                    steps = std::max(steps, 10);
                    float angStep = gf_PI2 / steps;
                    for (int i = 0; i < steps; i++)
                    {
                        float a0 = angStep * i;
                        float a1 = angStep * (i + 1);
                        float c0 = cosf(a0);
                        float c1 = cosf(a1);
                        float s0 = sinf(a0);
                        float s1 = sinf(a1);
                        Vec3 pts[4];
                        int n, n2;
                        vtx_idx* indTri;
                        if (iterList->radius)
                        {
                            n = 4;
                            n2 = 6;
                            pts[0] = iterList->pos + iterList->radius * Vec3(c0, s0, 0);
                            pts[1] = iterList->pos + iterList->radius * Vec3(c1, s1, 0);
                            pts[2] = iterList->pos + iterList->radius2 * Vec3(c1, s1, 0);
                            pts[3] = iterList->pos + iterList->radius2 * Vec3(c0, s0, 0);
                            indTri = indTriQuad;
                        }
                        else
                        {
                            n = 3;
                            n2 = 3;
                            pts[0] = iterList->pos;
                            pts[1] = pts[0] + iterList->radius2 * Vec3(c0, s0, 0);
                            pts[2] = pts[0] + iterList->radius2 * Vec3(c1, s1, 0);
                            indTri = indTriTri;
                        }
                        pAux->DrawTriangles(pts, n, indTri, n2, clr);
                    }
                }
                break;
                case eOT_Text:
                    draw2d->BeginDraw2d(true);
                    {
                        textOptions.horizontalAlignment = IDraw2d::HAlign::Left;
                        textOptions.verticalAlignment = IDraw2d::VAlign::Bottom;

                        float fontSize = iterList->radius;
                        const string& textLabel = iterList->text;

                        // localize the string
                        if (!textLabel.empty() && textLabel[0] == '@')
                        {
                            string localizedString;
                            LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::LocalizeString_s, textLabel, localizedString, false);
                            draw2d->DrawText(localizedString.c_str(), AZ::Vector2(iterList->pos.x, iterList->pos.y), fontSize, clr.a, &textOptions);
                        }
                        else
                        {
                            draw2d->DrawText(textLabel.c_str(), AZ::Vector2(iterList->pos.x, iterList->pos.y), fontSize, clr.a, &textOptions);
                        }
                    }
                    draw2d->EndDraw2d();
                    break;
                case eOT_Line2D:
                    draw2d->BeginDraw2d(true);
                    {
                        draw2d->DrawLine(AZ::Vector2(iterList->pos.x, iterList->pos.y), AZ::Vector2(iterList->dir.x, iterList->dir.y), AZ::Color(clr.r, clr.g, clr.b, clr.a));
                    }
                    draw2d->EndDraw2d();
                    break;
                case eOT_Circle2D:
                    draw2d->BeginDraw2d(true);
                    {
                        ITexture* texture = GetCircleTexture();
                        draw2d->DrawImage(texture->GetTextureID(), AZ::Vector2(iterList->pos.x, iterList->pos.y),
                            AZ::Vector2(iterList->radius, iterList->radius2) * 2.0f, clr.a, 0.0f, nullptr, nullptr, &imageOptions);
                    }
                    draw2d->EndDraw2d();
                    break;
                case eOT_Rectangle2D:
                    draw2d->BeginDraw2d(true);
                    {
                        ITexture* texture = GetSquareTexture();
                        draw2d->DrawImage(texture->GetTextureID(), AZ::Vector2(iterList->pos.x, iterList->pos.y),
                            AZ::Vector2(iterList->radius, iterList->radius2), clr.a, 0.0f, nullptr, nullptr, &imageOptions);
                    }
                    draw2d->EndDraw2d();
                    break;
                case eOT_EntityTag:
                {
                    UpdateTags(frameTime, *iterList);
                }
                break;
                }
            }
        }
        while (!toClear.empty())
        {
            iterMap->second.erase(toClear.back());
            toClear.pop_back();
        }
        if (iterMap->second.empty())
        {
            toClearMap.push_back(iterMap);
        }
    }
    while (!toClearMap.empty())
    {
        m_objects.erase(toClearMap.back());
        toClearMap.pop_back();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::PostUpdate(float frameTime)
{
    IDraw2d* draw2d = Draw2dHelper::GetDraw2d();
    if (!draw2d)
    {
        m_2DTexts.clear();
        return;
    }

    // Post update entity tags
    for (MapListObj::iterator iterMap = m_objects.begin(); iterMap != m_objects.end(); ++iterMap)
    {
        for (ListObj::iterator iterList = iterMap->second.begin(); iterList != iterMap->second.end(); ++iterList)
        {
            if (eOT_EntityTag == iterList->obj)
            {
                PostUpdateTags(frameTime, *iterList);
            }
        }
    }

    // fixed placement 2d text rendering
    draw2d->BeginDraw2d();
    {
        IDraw2d::TextOptions textOptions = draw2d->GetDefaultTextOptions();
        IDraw2d::ImageOptions imageOptions = draw2d->GetDefaultImageOptions();

        textOptions.horizontalAlignment = IDraw2d::HAlign::Center;
        textOptions.verticalAlignment = IDraw2d::VAlign::Top;

        AZ::Vector2 textPos(draw2d->GetViewportWidth() * 0.5f, draw2d->GetViewportHeight() * 0.6667f); // matches the same placement as the previous renderer

        const float textSpaceing = 2.0f;

        // now draw 2D texts overlay
        for (ListObjText2D::iterator iter = m_2DTexts.begin(); iter != m_2DTexts.end(); )
        {
            STextObj2D& textObj = *iter;

            textOptions.color = AZ::Vector3(textObj.clr.r, textObj.clr.g, textObj.clr.b);
            float alpha = textObj.clr.a * (textObj.timeRemaining / textObj.totalTime);

            float fontSize = textObj.size;
            AZ::Vector2 textSize(0.0f, 0.0f);
            const string& textLabel = textObj.text;

            // localize the string
            if (!textLabel.empty() && textLabel[0] == '@')
            {
                string localizedString;
                LocalizationManagerRequestBus::Broadcast(&LocalizationManagerRequestBus::Events::LocalizeString_s, textLabel, localizedString, false);

                textSize = draw2d->GetTextSize(localizedString.c_str(), fontSize, &textOptions);
                draw2d->DrawText(localizedString.c_str(), textPos, fontSize, alpha, &textOptions);
            }
            else
            {
                textSize = draw2d->GetTextSize(textLabel.c_str(), fontSize, &textOptions);
                draw2d->DrawText(textLabel.c_str(), textPos, fontSize, alpha, &textOptions);
            }

            textPos.SetY(textPos.GetY() + textSize.GetY() + textSpaceing);

            textObj.timeRemaining -= frameTime;
            const bool bDelete = textObj.timeRemaining <= 0.0f;
            if (bDelete)
            {
                ListObjText2D::iterator toDelete = iter;
                ++iter;
                m_2DTexts.erase(toDelete);
            }
            else
            {
                ++iter;
            }
        }
    }
    draw2d->EndDraw2d();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//static
void CPersistentDebug::VisualLogCallback(ICVar* pVar)
{
    CPersistentDebug* pPersistentDebug = CCryAction::GetCryAction()->GetPersistentDebug();
    if (pPersistentDebug && gEnv->pLog)
    {
        if (pVar->GetIVal())
        {
            gEnv->pLog->AddCallback(pPersistentDebug);
        }
        else
        {
            if (pPersistentDebug->m_pVisualConsoleSubStr)
            {
                pPersistentDebug->m_pVisualConsoleSubStr->Set("");
            }
            gEnv->pLog->RemoveCallback(pPersistentDebug);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CPersistentDebug::OnWriteToConsole(const char* sText, bool bNewLine)
{
    if (sText)
    {
        bool display = true;
        if (m_pVisualConsoleSubStr && m_pVisualConsoleSubStr->GetString())
        {
            if (!strstr(sText, m_pVisualConsoleSubStr->GetString()))
            {
                display = false;
            }
        }

        if (display)
        {
            Add2DText(sText, 1.0f, ColorF(1.0f, 1.0f, 1.0f), 20.0f);
        }
    }
}

