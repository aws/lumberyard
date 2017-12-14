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

#pragma once

struct LightningStats
{
    struct Stat
    {
        Stat()
            : m_current(0)
            , m_peak(0) {}
        void SetCurrent(int current)
        {
            m_current = current;
            m_peak = max(m_peak, current);
        }
        void Increment(int plus = 1) { SetCurrent(m_current + plus); }
        int GetCurrent() const { return m_current; }
        int GetPeak() const { return m_peak; }

    private:
        int m_current;
        int m_peak;
    };

    void Restart()
    {
        m_activeSparks.SetCurrent(0);
        m_memory.SetCurrent(0);
        m_triCount.SetCurrent(0);
        m_branches.SetCurrent(0);
    }

    void Draw();

    Stat m_activeSparks;
    Stat m_memory;
    Stat m_triCount;
    Stat m_branches;
};

struct LightningArcParams
{
    AZ_TYPE_INFO(LightningArcParams, "{F638407F-CB8C-4CE4-9571-2583508FCBAA}");

    LightningArcParams();

    void Reset(XmlNodeRef node);

    uint32 m_nameCRC; //Legacy
    AZ::Crc32 m_azCrc;

    static void Reflect(AZ::ReflectContext* context);
    static void EditorReflect(AZ::ReflectContext* context);

    float m_strikeTimeMin;
    float m_strikeTimeMax;
    float m_strikeFadeOut;
    AZ::u32 m_strikeNumSegments = 1;
    AZ::u32 m_strikeNumPoints = 1;

    float m_lightningDeviation;
    float m_lightningFuzziness;
    float m_lightningVelocity;

    float m_branchProbability;
    AZ::u32 m_branchMaxLevel;
    AZ::u32 m_maxNumStrikes;

    float m_beamSize;
    float m_beamTexTiling;
    float m_beamTexShift;
    float m_beamTexFrames;
    float m_beamTexFPS;
};