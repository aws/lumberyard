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

#include <ITimeOfDay.h>
#include <AzTest/AzTest.h>

class ITimeOfDayMock
    : public ITimeOfDay
{
public:
    MOCK_CONST_METHOD0(GetPresetCount,
        int());
    MOCK_CONST_METHOD2(GetPresetsInfos,
        bool(SPresetInfo* resultArray, unsigned int arraySize));
    MOCK_METHOD1(SetCurrentPreset,
        bool(const char* szPresetName));
    MOCK_METHOD1(AddNewPreset,
        bool(const char* szPresetName));
    MOCK_METHOD1(RemovePreset,
        bool(const char* szPresetName));
    MOCK_CONST_METHOD1(SavePreset,
        bool(const char* szPresetName));
    MOCK_METHOD1(LoadPreset,
        bool(const char* szFilePath));
    MOCK_METHOD1(ResetPreset,
        void(const char* szPresetName));
    MOCK_METHOD2(ImportPreset,
        bool(const char* szPresetName, const char* szFilePath));
    MOCK_CONST_METHOD2(ExportPreset,
        bool(const char* szPresetName, const char* szFilePath));
    MOCK_CONST_METHOD0(GetVariableCount,
        int());
    MOCK_CONST_METHOD2(GetVariableInfo,
        bool(int nIndex, SVariableInfo& varInfo));
    MOCK_METHOD2(SetVariableValue,
        void(int nIndex, float fValue[3]));
    MOCK_CONST_METHOD5(InterpolateVarInRange,
        bool(int nIndex, float fMin, float fMax, unsigned int nCount, Vec3* resultArray));
    MOCK_CONST_METHOD2(GetSplineKeysCount,
        uint(int nIndex, int nSpline));
    MOCK_CONST_METHOD4(GetSplineKeysForVar,
        bool(int nIndex, int nSpline, SBezierKey* keysArray, unsigned int keysArraySize));
    MOCK_METHOD4(SetSplineKeysForVar,
        bool(int nIndex, int nSpline, const SBezierKey* keysArray, unsigned int keysArraySize));
    MOCK_METHOD4(UpdateSplineKeyForVar,
        bool(int nIndex, int nSpline, float fTime, float newValue));
    MOCK_METHOD0(ResetVariables,
        void());
    MOCK_METHOD3(SetTime,
        void(float, bool, bool));
    MOCK_METHOD0(GetTime,
        float());
    MOCK_METHOD2(SetSunPos,
        void(float longitude, float latitude));
    MOCK_METHOD0(GetSunLatitude,
        float());
    MOCK_METHOD0(GetSunLongitude,
        float());
    MOCK_METHOD0(Tick,
        void());
    MOCK_METHOD1(SetPaused,
        void(bool paused));
    MOCK_METHOD0(MakeCurrentPresetFromCurrentParams,
        void());
    MOCK_METHOD1(SetAdvancedInfo,
        void(const SAdvancedInfo& advInfo));
    MOCK_METHOD1(GetAdvancedInfo,
        void(SAdvancedInfo& advInfo));
    MOCK_METHOD3(Update,
        void(bool, bool, bool));
    MOCK_METHOD0(UpdateValues,
        void());
    MOCK_METHOD1(UpdateEnvLighting,
        void(bool));
    MOCK_METHOD1(SetUpdateCallback,
        void(ITimeOfDayUpdateCallback* pCallback));
    MOCK_METHOD0(BeginEditMode,
        void());
    MOCK_METHOD0(EndEditMode,
        void());
    MOCK_METHOD2(Serialize,
        void(XmlNodeRef& node, bool bLoading));
    MOCK_METHOD1(Serialize,
        void(TSerialize ser));
    MOCK_METHOD1(SetTimer,
        void(ITimer* pTimer));
    MOCK_METHOD1(SetEnvironmentSettings,
        void(const SEnvironmentInfo& envInfo));
    MOCK_CONST_METHOD3(LerpWith,
        void(const ITimeOfDay& other, float lerpValue, ITimeOfDay& output));
    MOCK_METHOD3(NetSerialize,
        void(TSerialize ser, float lag, uint32 flags));
};

class ITimeOfDayUpdateCallbackMock : public ITimeOfDayUpdateCallback {
public:
    MOCK_METHOD0(BeginUpdate,
        void());
    MOCK_METHOD4(GetCustomValue,
        bool(ITimeOfDay::ETimeOfDayParamID paramID, int dim, float* pValues, float& blendWeight));
    MOCK_METHOD0(EndUpdate,
        void());
};
