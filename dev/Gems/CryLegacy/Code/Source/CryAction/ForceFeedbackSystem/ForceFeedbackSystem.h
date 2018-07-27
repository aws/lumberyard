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

// Description : Implementation for force feedback system
//
//               Effect definition (shape, time, ...) are defined in xml
//               Effects are invoked by name, and updated here internally, feeding
//               input system in a per frame basis


#ifndef CRYINCLUDE_CRYACTION_FORCEFEEDBACKSYSTEM_FORCEFEEDBACKSYSTEM_H
#define CRYINCLUDE_CRYACTION_FORCEFEEDBACKSYSTEM_FORCEFEEDBACKSYSTEM_H
#pragma once

#include "IForceFeedbackSystem.h"

#define FFSYSTEM_MAX_PATTERN_SAMPLES 32
#define FFSYSTEM_MAX_PATTERN_SAMPLES_FLOAT 32.0f
#define FFSYSTEM_PATTERN_SAMPLE_STEP 0.03125f

#define FFSYSTEM_MAX_ENVELOPE_SAMPLES 16
#define FFSYSTEM_MAX_ENVELOPE_SAMPLES_FLOAT 16.0f
#define FFSYSTEM_ENVELOPE_SAMPLE_STEP 0.0625f

#define FFSYSTEM_UINT16_TO_FLOAT 0.0000152590218967f

struct SForceFeedbackSystemCVars
{
    SForceFeedbackSystemCVars();
    ~SForceFeedbackSystemCVars();

    int ffs_debug;
};

class CForceFeedBackSystem
    : public IForceFeedbackSystem
{
private:

    struct FFBInternalId
    {
        FFBInternalId()
            : id (0)
        {
        }

        FFBInternalId(uint32 _id)
            : id (_id)
        {
        }

        FFBInternalId(const FFBInternalId& _otherId)
        {
#if defined(_DEBUG)
            name = _otherId.name;
#endif
            id = _otherId.id;
        }

        void Set(const char* _name);

        ILINE bool operator == (const FFBInternalId& rhs) const
        {
            return (id == rhs.id);
        }

        ILINE bool operator != (const FFBInternalId& rhs) const
        {
            return (id != rhs.id);
        }

        ILINE bool operator < (const FFBInternalId& rhs) const
        {
            return (id < rhs.id);
        }

        ILINE bool operator >= (const FFBInternalId& rhs) const
        {
            return (id >= rhs.id);
        }

        const char* GetDebugName() const;

        //This is useful for look ups in debug, so we don't allocate strings
        static FFBInternalId& GetIdForName(const char* name);

#if defined(_DEBUG)
        string name;
#endif
        uint32 id;
    };

    struct SPattern
    {
        float SamplePattern(float time) const
        {
            assert((time >= 0.0f) && (time <= 1.0f));

            const float fSampleIdx = floor_tpl(time * (float)fres(FFSYSTEM_PATTERN_SAMPLE_STEP));
            int sampleIdx1 = (int)fSampleIdx;
            assert((sampleIdx1 >= 0) && (sampleIdx1 < FFSYSTEM_MAX_PATTERN_SAMPLES));
            int sampleIdx2 = (sampleIdx1 >= (FFSYSTEM_MAX_PATTERN_SAMPLES - 1)) ?  0 : sampleIdx1 + 1;
            const float delta = clamp_tpl((FFSYSTEM_PATTERN_SAMPLE_STEP - (time - (FFSYSTEM_PATTERN_SAMPLE_STEP * fSampleIdx))) * FFSYSTEM_MAX_PATTERN_SAMPLES_FLOAT, 0.0f, 1.0f);

            return ((SampleToFloat(sampleIdx1) * delta) + (SampleToFloat(sampleIdx2) * (1.0f - delta)));
        }

        ILINE float SampleToFloat(int idx) const
        {
            return ((float)m_patternSamples[idx] * FFSYSTEM_UINT16_TO_FLOAT);
        }

        ILINE void ResetToDefault()
        {
            memset(m_patternSamples, 0, sizeof(uint16) * FFSYSTEM_MAX_PATTERN_SAMPLES);
        }

        ILINE bool operator < (const SPattern& rhs) const
        {
            return (m_patternId < rhs.m_patternId);
        }

        uint16 m_patternSamples[FFSYSTEM_MAX_PATTERN_SAMPLES];
        FFBInternalId m_patternId;
    };

    typedef std::vector<SPattern> TPatternsVector;

    struct SEnvelope
    {
        float SampleEnvelope(float time) const
        {
            assert((time >= 0.0f) && (time <= 1.0f));

            const float fSampleIdx = floor_tpl(time * (float)fres(FFSYSTEM_ENVELOPE_SAMPLE_STEP));
            int sampleIdx1 = (int)fSampleIdx;
            sampleIdx1 = (sampleIdx1 >= (FFSYSTEM_MAX_ENVELOPE_SAMPLES - 1)) ? (FFSYSTEM_MAX_ENVELOPE_SAMPLES - 2) : sampleIdx1;
            int sampleIdx2 = sampleIdx1 + 1;
            const float delta = clamp_tpl((FFSYSTEM_ENVELOPE_SAMPLE_STEP - (time - (FFSYSTEM_ENVELOPE_SAMPLE_STEP * fSampleIdx))) * FFSYSTEM_MAX_ENVELOPE_SAMPLES_FLOAT, 0.0f, 1.0f);

            return ((SampleToFloat(sampleIdx1) * delta) + (SampleToFloat(sampleIdx2) * (1.0f - delta)));
        }

        ILINE float SampleToFloat(int idx) const
        {
            return ((float)m_envelopeSamples[idx] * FFSYSTEM_UINT16_TO_FLOAT);
        }

        ILINE void ResetToDefault()
        {
            memset(m_envelopeSamples, 0, sizeof(uint16) * FFSYSTEM_MAX_ENVELOPE_SAMPLES);
        }

        ILINE bool operator < (const SEnvelope& rhs) const
        {
            return (m_envelopeId < rhs.m_envelopeId);
        }

        uint16 m_envelopeSamples[FFSYSTEM_MAX_ENVELOPE_SAMPLES];
        FFBInternalId m_envelopeId;
    };

    typedef std::vector<SEnvelope> TEnvelopesVector;

    struct SEffect
    {
        SEffect()
            : time(1.0f)
            , frequencyA(1.0f)
            , frequencyB(1.0f)
        {
        }

        FFBInternalId patternA;
        FFBInternalId envelopeA;
        FFBInternalId patternB;
        FFBInternalId envelopeB;

        float frequencyA;
        float frequencyB;
        float time;
    };

    typedef VectorMap<FFBInternalId, ForceFeedbackFxId> TEffectToIndexMap;
    typedef std::vector<SEffect>    TEffectArray;

    struct SFFOutput
    {
        SFFOutput()
            : forceFeedbackA(0.0f)
            , forceFeedbackB(0.0f)
        {
        }

        void operator += (const SFFOutput& operand2)
        {
            forceFeedbackA += operand2.forceFeedbackA;
            forceFeedbackB += operand2.forceFeedbackB;
        }

        ILINE float GetClampedFFA() const
        {
            return clamp_tpl(forceFeedbackA, 0.0f, 1.0f);
        }

        ILINE float GetClampedFFB() const
        {
            return clamp_tpl(forceFeedbackB, 0.0f, 1.0f);
        }

        ILINE void ZeroIt()
        {
            forceFeedbackA = forceFeedbackB = 0.0f;
        }

        float forceFeedbackA;
        float forceFeedbackB;
    };

    struct SActiveEffect
    {
        SActiveEffect()
            : effectId(InvalidForceFeedbackFxId)
            , effectTime(1.0f)
            , runningTime(0.0f)
            , frequencyA(1.0f)
            , frequencyB(1.0f)
            , m_inputDeviceType(eIDT_Gamepad)
        {
        }

        ILINE bool HasFinished() const
        {
            return (effectTime > 0.0f) ? (runningTime > effectTime) : false;
        }

        SFFOutput Update(float frameTime);

        ForceFeedbackFxId effectId;
        SForceFeedbackRuntimeParams runtimeParams;
        float effectTime;
        float runningTime;
        float frequencyA;
        float frequencyB;
        SPattern m_patternA;
        SEnvelope m_envelopeA;
        SPattern m_patternB;
        SEnvelope m_envelopeB;
        EInputDeviceType m_inputDeviceType;
    };

    typedef std::vector<SActiveEffect> TActiveEffectsArray;
    typedef std::vector<string> TEffectNamesArray;
    typedef std::map<EInputDeviceType, SFFOutput> TDeviceSFFOutputMap;
    typedef std::map<EInputDeviceType, SFFTriggerOutputData> TDeviceSFFTriggerOutputDataMap;

    typedef CryFixedStringT<256>    TSamplesBuffer;

public:

    CForceFeedBackSystem();
    ~CForceFeedBackSystem();

    //IForceFeedbackSystem
    void PlayForceFeedbackEffect(ForceFeedbackFxId id, const SForceFeedbackRuntimeParams& runtimeParams, EInputDeviceType inputDeviceType) override;
    void StopForceFeedbackEffect(ForceFeedbackFxId id) override;
    void StopAllEffects() override;

    ForceFeedbackFxId GetEffectIdByName(const char* effectName) const override;

    void AddFrameCustomForceFeedback(const float amplifierA, const float amplifierB, EInputDeviceType inputDeviceType) override;
    void AddCustomTriggerForceFeedback(const SFFTriggerOutputData& triggersData, EInputDeviceType inputDeviceType) override;

    void EnumerateEffects(IFFSPopulateCallBack* pCallBack) override;    // intended to be used only from the editor
    int GetEffectNamesCount() const override;
    void SuppressEffects(bool bSuppressEffects) override;
    //~IForceFeedbackSystem

    void Initialize();
    void Reload();

    void Update(float frameTime);

private:

    bool TryToRestartEffectIfAlreadyRunning(ForceFeedbackFxId id, const SForceFeedbackRuntimeParams& runtimeParams);

    void LoadXmlData();

    void LoadPatters(XmlNodeRef& patternsNode);
    void LoadEnvelopes(XmlNodeRef& envelopesNode);
    void LoadEffects(XmlNodeRef& effectsNode);

    int ParseSampleBuffer(const TSamplesBuffer& buffer, float* outputValues, const int maxOutputValues);
    void DistributeSamples(const float* sampleInput, const int sampleInputCount, uint16* sampleOutput, const int sampleOutputCount);

    void UpdateInputSystem(const float amplifierA, const float amplifierB, const SFFTriggerOutputData& triggers, EInputDeviceType inputDeviceType);

    const SPattern& FindPattern(const FFBInternalId& id) const;
    const SEnvelope& FindEnvelope(const FFBInternalId& id) const;

    SPattern    m_defaultPattern;
    SEnvelope m_defaultEnvelope;

    TPatternsVector m_patters;
    TEnvelopesVector m_envelopes;

    TEffectToIndexMap m_effectToIndexMap;
    TEffectArray            m_effects;
    TActiveEffectsArray m_activeEffects;

    TEffectNamesArray m_effectNames;        //Stored list of names for Flow graph system UI

    TDeviceSFFOutputMap m_frameCustomForceFeedbackMap;
    TDeviceSFFTriggerOutputDataMap m_triggerCustomForceFeedBackMap;

    SForceFeedbackSystemCVars m_cvars;
    int         m_effectLock;
};
#endif // CRYINCLUDE_CRYACTION_FORCEFEEDBACKSYSTEM_FORCEFEEDBACKSYSTEM_H
