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

#ifndef CRYINCLUDE_CRYANIMATION_FACIALANIMATION_EYEMOVEMENTFACEANIM_H
#define CRYINCLUDE_CRYANIMATION_FACIALANIMATION_EYEMOVEMENTFACEANIM_H
#pragma once

class CFacialInstance;
class CFacialEffectorsLibrary;
class CFaceState;
class CFaceIdentifierHandle;
class CCharInstance;

#define EYEMOVEMENT_EFFECTOR_AMOUNT 16

class CEyeMovementFaceAnim
    : public _i_reference_target_t
{
public:
    CEyeMovementFaceAnim(CFacialInstance* pInstance);

    void Update(float fDeltaTimeSec, const QuatTS& rAnimLocationNext, CCharInstance* pCharacter, CFacialEffectorsLibrary* pEffectorsLibrary, CFaceState* pFaceState);
    void OnExpressionLibraryLoad();

    void GetMemoryUsage(ICrySizer* pSizer) const;

private:
    enum DirectionID
    {
        DirectionNONE = -1,

        DirectionEyeUp = 0,
        DirectionEyeUpRight,
        DirectionEyeRight,
        DirectionEyeDownRight,
        DirectionEyeDown,
        DirectionEyeDownLeft,
        DirectionEyeLeft,
        DirectionEyeUpLeft,

        DirectionCOUNT
    };

    enum EyeID
    {
        EyeLeft,
        EyeRight,

        EyeCOUNT
    };

    enum EffectorID
    {
        EffectorEyeLeftDirectionEyeUp,
        EffectorEyeLeftDirectionEyeUpRight,
        EffectorEyeLeftDirectionEyeRight,
        EffectorEyeLeftDirectionEyeDownRight,
        EffectorEyeLeftDirectionEyeDown,
        EffectorEyeLeftDirectionEyeDownLeft,
        EffectorEyeLeftDirectionEyeLeft,
        EffectorEyeLeftDirectionEyeUpLeft,
        EffectorEyeRightDirectionEyeUp,
        EffectorEyeRightDirectionEyeUpRight,
        EffectorEyeRightDirectionEyeRight,
        EffectorEyeRightDirectionEyeDownRight,
        EffectorEyeRightDirectionEyeDown,
        EffectorEyeRightDirectionEyeDownLeft,
        EffectorEyeRightDirectionEyeLeft,
        EffectorEyeRightDirectionEyeUpLeft,

        EffectorCOUNT
    };

    EffectorID EffectorFromEyeAndDirection(EyeID eye, DirectionID direction)
    {
        return static_cast<EffectorID>(eye * DirectionCOUNT + direction);
    }

    void InitialiseChannels();
    uint32 GetChannelForEffector(EffectorID effector);
    uint32 CreateChannelForEffector(EffectorID effector);
    void UpdateEye(const QuatTS& rAnimLocationNext, EyeID eye, const QuatT& additionalRotation);
    DirectionID FindEyeDirection(EyeID eye);
    void InitialiseBoneIDs();
    void FindLookAngleAndStrength(EyeID eye, float& angle, float& strength, const QuatT& additionalRotation);
    void DisplayDebugInfoForEye(const QuatTS& rAnimLocationNext, EyeID eye, const string& text);
    void CalculateEyeAdditionalRotation(CCharInstance * pCharacter, CFaceState * pFaceState, CFacialEffectorsLibrary * pEffectorsLibrary, QuatT additionalRotation[EyeCOUNT]);
    const CFaceIdentifierHandle* RetrieveEffectorIdentifiers() const;

    CFacialInstance* m_pInstance;
    static const char* szEyeBoneNames[EyeCOUNT];
    static uint32 ms_eyeBoneNamesCrc[EyeCOUNT];

    class CChannelEntry
    {
    public:
        CChannelEntry()
            : id(~0)
            , loadingAttempted(false) {}
        int id;
        bool loadingAttempted;
    };

    CChannelEntry m_channelEntries[EffectorCOUNT];
    int m_eyeBoneIDs[EyeCOUNT];
    bool m_bInitialized;
};

#endif // CRYINCLUDE_CRYANIMATION_FACIALANIMATION_EYEMOVEMENTFACEANIM_H
