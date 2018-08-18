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

#ifndef CRYINCLUDE_CRYCOMMON_SCREENPLOTTER_H
#define CRYINCLUDE_CRYCOMMON_SCREENPLOTTER_H
#pragma once


#include "IRenderer.h"
#include <IRenderAuxGeom.h> // <> required for Interfuscator
#include <ITimer.h> // <> required for Interfuscator
#include <deque>

class CScreenPlotter
{
    struct PlotValue
    {
        float               m_value;
        CTimeValue  m_timestamp;

        PlotValue(float value)
        {
            m_value = value;
            if (gEnv->pTimer)
            {
                m_timestamp = gEnv->pTimer->GetFrameStartTime();
            }
        }
    };

public:

    enum PlotMode
    {
        ALL,
        ONE_PER_FRAME,
        TIME_FRAME
    };

    //just initialize the plotter with screen coordinates (pixels) and optional labeling information
    CScreenPlotter(float x1, float x2, float y1, float y2, int32 minValue = 0, int32 maxValue = 100, char* xLabel = "X", char* yLabel = "Y")
        : m_posX1(x1)
        , m_posX2(x2)
        , m_posY1(y1)
        , m_posY2(y2)
        , m_minValue(minValue)
        , m_maxValue(maxValue)
        , m_xAxisLabel(xLabel)
        , m_yAxisLabel(yLabel)
        , m_graphColor(0, 220, 220)
    {
        m_pRenderer = NULL;
        m_valueCount = 0;
        m_screenWidth = int32(320);
        m_screenHeight = int32(240);
        m_fTimeFrame = 10000.0f;
    }

    //this function adds a value to the plotter's stack
    void AddValue(float val)
    {
        if (val > m_maxValue)
        {
            val = (float)m_maxValue;
        }
        else if (val < m_minValue)
        {
            val = (float)m_minValue;
        }

        val = (val + m_minValue) * m_height / (float)(m_maxValue - m_minValue);

        m_values.push_back(PlotValue(val));
        if ((m_valueCount > 0 && m_values.size() > m_valueCount) || (m_valueCount == 0 && (m_values.size() > m_width * m_screenWidth)))
        {
            m_values.pop_front();
        }
    }

    //render graph with chosen mode (ALL, ONCE_PER_FRAME (interpolated) ...)
    void Render(PlotMode mode = ALL)
    {
        if (m_pRenderer == NULL)
        {
            m_pRenderer = gEnv->pRenderer;
            m_pRAG = gEnv->pRenderer->GetIRenderAuxGeom();
            m_screenWidth = (float)m_pRenderer->GetWidth();
            m_screenHeight = (float)m_pRenderer->GetHeight();
            m_posX1 /= m_screenWidth;
            m_posX2 /= m_screenWidth;
            m_posY1 /= m_screenHeight;
            m_posY2 /= m_screenHeight;
            m_height = m_posY2 - m_posY1;
            m_width = m_posX2 - m_posX1;
            m_textScale = m_height / 20.0f;
        }

        m_pRAG->SetRenderFlags(SAuxGeomRenderFlags(e_Def2DPublicRenderflags));

        RenderLine(m_posX1, m_posX2, m_posY1, m_posY1);
        RenderLine(m_posX1, m_posX1, m_posY1, m_posY2);

        m_pRAG->DrawPoint(Vec3(m_posX2, m_posY1, 0), ColorB(255, 0, 0), 3);
        m_pRAG->DrawPoint(Vec3(m_posX1, m_posY2, 0), ColorB(255, 0, 0), 3);

        if (m_values.size() < 2)
        {
            return;
        }

        switch (mode)
        {
        case ALL:
            RenderAll();
            break;
        case ONE_PER_FRAME:
            RenderOPF();
            break;
        case TIME_FRAME:
            RenderTimeFrame();
            break;
        default:
            break;
        }

        RenderLabel(m_posX2, m_posY1, m_xAxisLabel.c_str());
        RenderLabel(m_posX1, m_posY2, m_yAxisLabel.c_str());
        char num[] = {' ', ' ', ' ', ' '};
        azitoa(m_maxValue, num, AZ_ARRAY_SIZE(num), 10);
        RenderLabel(m_posX1, m_posY2 + m_textScale, num);
        azitoa(m_minValue, num, AZ_ARRAY_SIZE(num), 10);
        RenderLabel(m_posX1 - m_textScale, m_posY1, num);
    }

    ILINE void SetGraphColor(ColorB color)
    {
        m_graphColor = color;
    }

    //set the maximum amount of values (overwrites pre-definition)
    ILINE void SetValueCount(uint32 amount)
    {
        m_valueCount = (int32)amount;
    }

    //move and scale the plot
    ILINE void SetGeometry(float x1, float x2, float y1, float y2)
    {
        m_posX1 = x1;
        m_posX2 = x2;
        m_posY1 = y1;
        m_posY2 = y2;
        m_height = m_posY2 - m_posY1;
        m_width = m_posX2 - m_posX1;
        m_textScale = m_height / 20.0f;
        m_pRenderer = NULL;
    }

    //set the time window for TimeFrame rendering
    ILINE void SetTimeFrame(float milliseconds)
    {
        m_fTimeFrame = milliseconds;
    }

private:

    void RenderLine(float x1, float x2, float y1, float y2, const ColorB& col = ColorB(0, 0, 0), float thickness = 1.f)
    {
        m_pRAG->DrawLine(Vec3(x1, y1, 0), col, Vec3(x2, y2, 0), col, thickness);
    }

    void RenderLabel(float posX, float posY, const char* msg, float fScale = 1.2f, ColorB col = ColorB(0, 0, 255))
    {
        float pX = floorf(m_pRenderer->GetWidth() * posX);
        float pY = floorf(m_pRenderer->GetHeight() * posY);
        float textCol[] = {col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, 1};
        m_pRenderer->Draw2dLabel(pX, pY, fScale, textCol, false, "%s", msg);
    }

    //render every single value
    void RenderAll()
    {
        if (m_values.size() < 2)
        {
            return;
        }
        std::deque<PlotValue>::iterator it;
        std::deque<PlotValue>::iterator it2;
        int32 valNum = 1;
        float dX = m_width / (float)m_values.size();
        for (it2 = m_values.begin() + 1; it2 != m_values.end(); it2++)
        {
            it = it2 - 1;
            RenderLine(m_posX1 + dX * valNum, m_posX1 + dX * (valNum + 1), m_posY1 + (*it).m_value, m_posY1 + (*it2).m_value, m_graphColor);
            valNum++;
        }
    }

    //render time frame
    void RenderTimeFrame()
    {
        if (m_values.size() < 2)
        {
            return;
        }

        std::deque<PlotValue>::iterator it;
        std::deque<PlotValue>::iterator it2;

        float now = gEnv->pTimer->GetAsyncTime().GetMilliSeconds();

        it2 = m_values.begin(); //get first value
        float timeDiff = (*it2).m_timestamp.GetMilliSeconds() - now;
        while (timeDiff > m_fTimeFrame)
        {
            if (it2 == m_values.end())
            {
                break;
            }
            it2++;
            timeDiff = (*it2).m_timestamp.GetMilliSeconds() - now;
        }

        float time1, time2;
        time1 = time2 = 0;
        for (; it2 != m_values.end(); it2++)
        {
            it = it2 - 1;
            time1 = (*it).m_timestamp.GetMilliSeconds() - now;
            time2 = (*it2).m_timestamp.GetMilliSeconds() - now;
            float posX1 = (time1 / m_fTimeFrame) * m_width;
            float posX2 = (time2 / m_fTimeFrame) * m_width;
            RenderLine(m_posX1 + posX1, m_posX1 + posX2, m_posY1 + (*it).m_value, m_posY1 + (*it2).m_value, m_graphColor);
        }
    }

    //render only once per frame (interpolated values)
    void RenderOPF()
    {
        if (m_values.size() < 2)
        {
            return;
        }
        std::deque<PlotValue>::iterator it;
        std::deque<PlotValue>::iterator it2;
        int32 differentValues = 0;
        CTimeValue lastTime;
        float val = 0;
        int32 count = 0;
        std::deque<PlotValue> int32erpolated;
        for (it = m_values.begin();; it++)
        {
            it2 = (it + 1);
            if (it2 == m_values.end())
            {
                if (count > 0)
                {
                    int32erpolated.push_back(PlotValue(val / float(count)));
                }
                break;
            }
            val += (*it).m_value;
            count++;
            lastTime = (*it).m_timestamp;

            if (lastTime != (*it2).m_timestamp)
            {
                int32erpolated.push_back(PlotValue(val / float(count)));
                differentValues++;
                val = 0.0f;
                count = 0;
            }
        }

        differentValues--;
        if (differentValues < 1)
        {
            return;
        }

        float dX = m_width / (float)differentValues;
        int32 valNum = 0;
        for (it2 = int32erpolated.begin() + 1; it2 != int32erpolated.end(); it2++)
        {
            it = it2 - 1;
            RenderLine(m_posX1 + dX * valNum, m_posX1 + dX * (valNum + 1), m_posY1 + (*it).m_value, m_posY1 + (*it2).m_value, m_graphColor);
            valNum++;
        }
    }

    IRenderer* m_pRenderer;
    IRenderAuxGeom* m_pRAG;

    std::deque<PlotValue> m_values;

    float                           m_posX1, m_posX2, m_posY1, m_posY2, m_height, m_width, m_textScale, m_fTimeFrame;
    float                           m_screenWidth, m_screenHeight;
    int32                           m_minValue, m_maxValue;
    uint32                      m_valueCount;
    string                      m_xAxisLabel, m_yAxisLabel;
    ColorB                      m_graphColor;
};

#endif // CRYINCLUDE_CRYCOMMON_SCREENPLOTTER_H
