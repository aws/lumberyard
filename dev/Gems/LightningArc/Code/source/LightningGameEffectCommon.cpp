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

#include "LightningArc_precompiled.h"

#include "LightningGameEffectCommon.h"

#include "GameXmlParamReader.h"

void LightningStats::Draw()
{
    Vec2 textPos = Vec2(400.0f, 10.0f);
    float red[4] = { 1, 0, 0, 1 };
    float location = 50.0f;
    const float step = 12.0f;
    gEnv->pRenderer->Draw2dLabel(textPos.x, textPos.y, 1.4f, red, false, "Lightning Stats: # (peak #)");
    textPos.y += step;
    gEnv->pRenderer->Draw2dLabel(textPos.x, textPos.y, 1.4f, red, false, "Active Sparks: %d (%d)",
        m_activeSparks.GetCurrent(), m_activeSparks.GetPeak());
    textPos.y += step;
    gEnv->pRenderer->Draw2dLabel(textPos.x, textPos.y, 1.4f, red, false, "Memory: %d (%d) bytes",
        m_memory.GetCurrent(), m_memory.GetPeak());
    textPos.y += step;
    gEnv->pRenderer->Draw2dLabel(textPos.x, textPos.y, 1.4f, red, false, "Tri Count: %d (%d)",
        m_triCount.GetCurrent(), m_triCount.GetPeak());
    textPos.y += step;
    gEnv->pRenderer->Draw2dLabel(textPos.x, textPos.y, 1.4f, red, false, "Branches: %d (%d)",
        m_branches.GetCurrent(), m_branches.GetPeak());
}

LightningArcParams::LightningArcParams()
    : m_lightningDeviation(0.05f)
    , m_lightningFuzziness(0.01f)
    , m_branchMaxLevel(1)
    , m_branchProbability(2.0f)
    , m_lightningVelocity(0.5f)
    , m_strikeTimeMin(0.25f)
    , m_strikeTimeMax(0.25f)
    , m_strikeFadeOut(1.0f)
    , m_strikeNumSegments(6)
    , m_strikeNumPoints(8)
    , m_maxNumStrikes(6)
    , m_beamSize(0.15f)
    , m_beamTexTiling(0.75f)
    , m_beamTexShift(0.2f)
    , m_beamTexFrames(4.0f)
    , m_beamTexFPS(15.0f)
{
}

void LightningArcParams::Reset(XmlNodeRef node)
{
    CGameXmlParamReader reader(node);
    reader.ReadParamValue("lightningDeviation", m_lightningDeviation);
    reader.ReadParamValue("lightningFuzziness", m_lightningFuzziness);
    reader.ReadParamValue("branchMaxLevel", m_branchMaxLevel);
    reader.ReadParamValue("branchProbability", m_branchProbability);
    reader.ReadParamValue("lightningVelocity", m_lightningVelocity);
    reader.ReadParamValue("strikeTimeMin", m_strikeTimeMin);
    reader.ReadParamValue("strikeTimeMax", m_strikeTimeMax);
    reader.ReadParamValue("strikeFadeOut", m_strikeFadeOut);
    reader.ReadParamValue("strikeNumSegments", m_strikeNumSegments);
    reader.ReadParamValue("strikeNumPoints", m_strikeNumPoints);
    reader.ReadParamValue("maxNumStrikes", m_maxNumStrikes);
    reader.ReadParamValue("beamSize", m_beamSize);
    reader.ReadParamValue("beamTexTiling", m_beamTexTiling);
    reader.ReadParamValue("beamTexShift", m_beamTexShift);
    reader.ReadParamValue("beamTexFrames", m_beamTexFrames);
    reader.ReadParamValue("beamTexFPS", m_beamTexFPS);
}
