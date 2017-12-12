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

#ifndef CRYINCLUDE_CRY3DENGINE_TIMEOFDAY_H
#define CRYINCLUDE_CRY3DENGINE_TIMEOFDAY_H
#pragma once

#include <ITimeOfDay.h>

class CEnvironmentPreset;
//////////////////////////////////////////////////////////////////////////
// ITimeOfDay interface implementation.
//////////////////////////////////////////////////////////////////////////
class CTimeOfDay
    : public ITimeOfDay
{
public:
    CTimeOfDay();
    ~CTimeOfDay();

    //////////////////////////////////////////////////////////////////////////
    // ITimeOfDay
    //////////////////////////////////////////////////////////////////////////

    int GetPresetCount() const override { return m_presets.size(); }
    bool GetPresetsInfos(SPresetInfo* resultArray, unsigned int arraySize) const override { return false; }
    bool SetCurrentPreset(const char* szPresetName) override { return false; };
    bool AddNewPreset(const char* szPresetName) override { return false; };
    bool RemovePreset(const char* szPresetName) override { return false; };
    bool SavePreset(const char* szPresetName) const override { return false; };
    bool LoadPreset(const char* szFilePath) override { return false; };
    void ResetPreset(const char* szPresetName) override { };

    bool ImportPreset(const char* szPresetName, const char* szFilePath) override { return false; };
    bool ExportPreset(const char* szPresetName, const char* szFilePath) const override { return false; };

    int GetVariableCount() const override { return ITimeOfDay::PARAM_TOTAL; };
    bool GetVariableInfo(int nIndex, SVariableInfo &varInfo) const override;
    void SetVariableValue(int nIndex, float fValue[3]) override;

    bool InterpolateVarInRange(int nIndex, float fMin, float fMax, unsigned int nCount, Vec3* resultArray) const override { return false; };
    uint GetSplineKeysCount(int nIndex, int nSpline) const override { return 0; };
    bool GetSplineKeysForVar(int nIndex, int nSpline, SBezierKey* keysArray, unsigned int keysArraySize) const override { return false; };
    bool SetSplineKeysForVar(int nIndex, int nSpline, const SBezierKey* keysArray, unsigned int keysArraySize) override { return false; };
    bool UpdateSplineKeyForVar(int nIndex, int nSpline, float fTime, float newValue) override { return false; };

    void ResetVariables() override;

    // Time of day is specified in hours.
    void SetTime(float fHour, bool bForceUpdate = false, bool bEnvUpdate = true) override;
    void SetSunPos(float longitude, float latitude) override;
    float GetSunLatitude() override { return m_sunRotationLatitude; }
    float GetSunLongitude() override { return m_sunRotationLongitude; }
    float GetTime() override { return m_fTime; };

    void SetPaused(bool paused) override { m_bPaused = paused; }

    void SetAdvancedInfo(const SAdvancedInfo &advInfo) override;
    void GetAdvancedInfo(SAdvancedInfo &advInfo) override;

    float GetHDRMultiplier() const { return m_fHDRMultiplier; }

    void Update(bool bInterpolate = true, bool bForceUpdate = false, bool bForceEnvUpdate = true) override;
    void SetUpdateCallback(ITimeOfDayUpdateCallback* pCallback) override;

    void Serialize(XmlNodeRef& node, bool bLoading) override;
    void Serialize(TSerialize ser) override;

    void SetTimer(ITimer * pTimer) override;

    void NetSerialize(TSerialize ser, float lag, uint32 flags) override;

    void Tick() override;

    void SetEnvironmentSettings(const SEnvironmentInfo& envInfo) override;
    void LerpWith(const ITimeOfDay& other, float lerpValue, ITimeOfDay& output) const override;


    //////////////////////////////////////////////////////////////////////////

    void BeginEditMode();
    void EndEditMode();

private:
    CTimeOfDay(const CTimeOfDay&);
    CTimeOfDay(const CTimeOfDay&&);
    CTimeOfDay& operator=(const CTimeOfDay&);
    CTimeOfDay& operator=(const CTimeOfDay&&);

    SVariableInfo& GetVar(ETimeOfDayParamID id);
    void UpdateEnvLighting(bool forceUpdate);
    void MigrateLegacyData(bool bSunIntensity, const XmlNodeRef& node);
    void LoadValueFromXmlNode(ETimeOfDayParamID destId, const XmlNodeRef& varNode);

private:
    typedef std::map<string, CEnvironmentPreset>              TPresetsSet;
    TPresetsSet                                               m_presets;
    CEnvironmentPreset*                                       m_pCurrentPreset;
    CEnvironmentPreset*                                       m_defaultPreset;

    SVariableInfo                                             m_vars[ITimeOfDay::PARAM_TOTAL];
    std::map<const char*, int, stl::less_strcmp<const char*> >  m_varsMap;
    float                                                     m_fTime;
    float                                                     m_fEditorTime;
    float                                                     m_sunRotationLatitude;
    float                                                     m_sunRotationLongitude;

    bool                                                      m_bEditMode;
    bool                                                      m_bPaused;
    bool                                                      m_bSunLinkedToTOD;

    SAdvancedInfo                                             m_advancedInfo;
    ITimer*                                                   m_pTimer;
    float                                                     m_fHDRMultiplier;
    ICVar*                                                    m_pTimeOfDaySpeedCVar;
    ITimeOfDayUpdateCallback*                                 m_pUpdateCallback;
};

#endif // CRYINCLUDE_CRY3DENGINE_TIMEOFDAY_H
