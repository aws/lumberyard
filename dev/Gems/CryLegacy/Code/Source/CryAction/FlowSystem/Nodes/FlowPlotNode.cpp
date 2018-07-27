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

#include "CryLegacy_precompiled.h"
#include <FlowSystem/Nodes/FlowBaseNode.h>
#include "TimeValue.h"
#include "IRenderAuxGeom.h"

class CFlowPlotNode
    : public CFlowBaseNode<eNCT_Instanced>
{
public:
    enum INPUTS
    {
        PLOTX,
        PLOTY,
        POSX,
        POSY,
        WIDTH,
        HEIGHT,
        MAXPOINTAGE,
        PLOTCOLOR
    };

    CFlowPlotNode(SActivationInfo* pActInfo)
    {
        m_bGotActivation = false;
    }

    IFlowNodePtr Clone(SActivationInfo* pActInfo)
    {
        return new CFlowPlotNode(pActInfo);
    }

    void Serialize(SActivationInfo*, TSerialize ser)
    {
        ser.BeginGroup("Local");
        ser.Value("m_bGotActivation", m_bGotActivation);
        // TODO: ser.Value("m_points", m_points);
        if (ser.IsReading())
        {
            m_points.clear();
        }

        ser.EndGroup();
    }

    virtual void GetMemoryUsage(ICrySizer* s) const
    {
        s->Add(*this);
    }

    void GetConfiguration(SFlowNodeConfig& config)
    {
        static const SInputPortConfig inputs[] = {
            InputPortConfig<float>("plotX", _HELP("x-coordinate to plot")),
            InputPortConfig<float>("plotY", _HELP("y-coordinate to plot")),
            InputPortConfig<float>("posX", 50.0f, _HELP("x-coordinate of graph")),
            InputPortConfig<float>("posY", 100.0f, _HELP("y-coordinate of graph")),
            InputPortConfig<float>("width", 400.0f, _HELP("width of graph")),
            InputPortConfig<float>("height", 300.0f, _HELP("height of graph")),
            InputPortConfig<float>("maxPointAge", 10.0f, _HELP("maximum age of a point on screen in seconds")),
            InputPortConfig<Vec3>("color", Vec3(255.0f, 255.0f, 255.0f), _HELP("Color to plot with")),
            {0}
        };
        config.sDescription = _HELP("Plot x,y coordinate pairs in a graph on screen");
        config.pInputPorts = inputs;
        config.SetCategory(EFLN_ADVANCED);
    }

    void ProcessEvent(EFlowEvent event, SActivationInfo* pActInfo)
    {
        switch (event)
        {
        case eFE_Initialize:
            pActInfo->pGraph->SetRegularlyUpdated(pActInfo->myID, true);
            break;
        case eFE_Activate:
            m_bGotActivation |= IsPortActive(pActInfo, PLOTX);
            m_bGotActivation |= IsPortActive(pActInfo, PLOTY);
            break;
        case eFE_Update:
            Update(pActInfo);
            break;
        }
    }

private:
    struct SPoint
    {
        CTimeValue when;
        Vec2 point;
    };
    std::deque<SPoint> m_points;
    bool m_bGotActivation;

    void Update(SActivationInfo* pActInfo)
    {
        CTimeValue now = gEnv->pTimer->GetFrameStartTime();
        IRenderAuxGeom* pRenderAuxGeom(gEnv->pRenderer->GetIRenderAuxGeom());

        // check for new points
        if (m_bGotActivation)
        {
            SPoint pt;
            pt.when = now;
            pt.point = Vec2(GetPortFloat(pActInfo, PLOTX), GetPortFloat(pActInfo, PLOTY));
            m_points.push_back(pt);
        }
        m_bGotActivation = false;
        // check for too old points
        CTimeValue expired = now - GetPortFloat(pActInfo, MAXPOINTAGE);
        while (!m_points.empty() && m_points.front().when < expired)
        {
            m_points.pop_front();
        }
        // check if we've anything to draw
        if (m_points.size() < 2)
        {
            return;
        }
        // calculate bounds
        Vec2 botLeft, topRight;
        topRight = botLeft = m_points[0].point;
        for (size_t i = 1; i < m_points.size(); i++)
        {
            if (botLeft.x > m_points[i].point.x)
            {
                botLeft.x = m_points[i].point.x;
            }
            if (botLeft.y > m_points[i].point.y)
            {
                botLeft.y = m_points[i].point.y;
            }
            if (topRight.x < m_points[i].point.x)
            {
                topRight.x = m_points[i].point.x;
            }
            if (topRight.y < m_points[i].point.y)
            {
                topRight.y = m_points[i].point.y;
            }
        }
        if (botLeft.x == topRight.x || botLeft.y == topRight.y)
        {
            return;
        }
        // draw the "chart"
        Vec3 colorBase = GetPortVec3(pActInfo, PLOTCOLOR) / 255.0f;
        float maxAge = GetPortFloat(pActInfo, MAXPOINTAGE);
        Vec2 scalePoint = Vec2(GetPortFloat(pActInfo, WIDTH), GetPortFloat(pActInfo, HEIGHT));
        scalePoint.x /= (topRight - botLeft).x;
        scalePoint.y /= (topRight - botLeft).y;
        Vec3 posGraph = Vec3(GetPortFloat(pActInfo, POSX), GetPortFloat(pActInfo, POSY), 0);
        Vec2 p0 = (m_points[0].point - botLeft);
        Vec3 prevPoint(p0.x, p0.y, 0.0f);
        prevPoint.x *= scalePoint.x;
        prevPoint.y *= scalePoint.y;
        prevPoint += posGraph;
        prevPoint.x /= 800.0f;
        prevPoint.y /= 600.0f;
        ColorF prevColor(colorBase.x, colorBase.y, colorBase.z, 1.0f - (now - m_points[0].when).GetSeconds() / maxAge);
        SAuxGeomRenderFlags renderFlags(e_Def2DPublicRenderflags);
        renderFlags.SetAlphaBlendMode(e_AlphaBlended);
        for (size_t i = 1; i < m_points.size(); i++)
        {
            p0 = (m_points[i].point - botLeft);
            Vec3 point(p0.x, p0.y, 0.0f);
            point.x *= scalePoint.x;
            point.y *= scalePoint.y;
            point += posGraph;
            point.x /= 800.0f;
            point.y /= 600.0f;
            ColorF color(colorBase.x, colorBase.y, colorBase.z, 1.0f - (now - m_points[i].when).GetSeconds() / maxAge);
            pRenderAuxGeom->SetRenderFlags(renderFlags);
            pRenderAuxGeom->DrawLine(prevPoint, prevColor, point, color);
            prevPoint = point;
            prevColor = color;
        }
    }
};

FLOW_NODE_BLACKLISTED("Log:Plot", CFlowPlotNode);
