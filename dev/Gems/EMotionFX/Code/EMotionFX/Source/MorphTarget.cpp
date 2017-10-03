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

// include the required headers
#include "EMotionFXConfig.h"
#include "Node.h"
#include "MorphTarget.h"


namespace EMotionFX
{
    // the constructor
    MorphTarget::MorphTarget(const char* name)
        : BaseObject()
    {
        mRangeMin       = 0.0f;
        mRangeMax       = 1.0f;
        mPhonemeSets    = PHONEMESET_NONE;

        // set the name
        SetName(name);
    }


    // destructor
    MorphTarget::~MorphTarget()
    {
    }


    // convert the given phoneme name to a phoneme set
    MorphTarget::EPhonemeSet MorphTarget::FindPhonemeSet(const MCore::String& phonemeName)
    {
        // return neutral pose if the phoneme name is empty
        if (phonemeName.GetIsEmpty() || phonemeName == "x")
        {
            return PHONEMESET_NEUTRAL_POSE;
        }

        // AW
        if (phonemeName.CheckIfIsEqualNoCase("AW"))
        {
            return PHONEMESET_AW;
        }

        // UW_UH_OY
        if (phonemeName.CheckIfIsEqualNoCase("UW") || phonemeName.CheckIfIsEqualNoCase("UH") || phonemeName.CheckIfIsEqualNoCase("OY") || phonemeName.CheckIfIsEqualNoCase("UW_UH_OY"))
        {
            return PHONEMESET_UW_UH_OY;
        }

        // AA_AO_OW
        if (phonemeName.CheckIfIsEqualNoCase("AA") || phonemeName.CheckIfIsEqualNoCase("AO") || phonemeName.CheckIfIsEqualNoCase("OW") || phonemeName.CheckIfIsEqualNoCase("AA_AO_OW"))
        {
            return PHONEMESET_AA_AO_OW;
        }

        // IH_AE_AH_EY_AY_H
        if (phonemeName.CheckIfIsEqualNoCase("IH") || phonemeName.CheckIfIsEqualNoCase("AE") || phonemeName.CheckIfIsEqualNoCase("AH") || phonemeName.CheckIfIsEqualNoCase("EY") || phonemeName.CheckIfIsEqualNoCase("AY") || phonemeName.CheckIfIsEqualNoCase("H") || phonemeName.CheckIfIsEqualNoCase("IH_AE_AH_EY_AY_H"))
        {
            return PHONEMESET_IH_AE_AH_EY_AY_H;
        }

        // IY_EH_Y
        if (phonemeName.CheckIfIsEqualNoCase("IY") || phonemeName.CheckIfIsEqualNoCase("EH") || phonemeName.CheckIfIsEqualNoCase("Y") || phonemeName.CheckIfIsEqualNoCase("IY_EH_Y"))
        {
            return PHONEMESET_IY_EH_Y;
        }

        // L_EL
        if (phonemeName.CheckIfIsEqualNoCase("") || phonemeName.CheckIfIsEqualNoCase("E") || phonemeName.CheckIfIsEqualNoCase("L_E"))
        {
            return PHONEMESET_L_EL;
        }

        // N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH
        if (phonemeName.CheckIfIsEqualNoCase("N") || phonemeName.CheckIfIsEqualNoCase("NG") || phonemeName.CheckIfIsEqualNoCase("CH") || phonemeName.CheckIfIsEqualNoCase("J")  || phonemeName.CheckIfIsEqualNoCase("DH") || phonemeName.CheckIfIsEqualNoCase("D") || phonemeName.CheckIfIsEqualNoCase("G") || phonemeName.CheckIfIsEqualNoCase("T") || phonemeName.CheckIfIsEqualNoCase("K") || phonemeName.CheckIfIsEqualNoCase("Z")  || phonemeName.CheckIfIsEqualNoCase("ZH") || phonemeName.CheckIfIsEqualNoCase("TH") || phonemeName.CheckIfIsEqualNoCase("S") || phonemeName.CheckIfIsEqualNoCase("SH") || phonemeName.CheckIfIsEqualNoCase("N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH"))
        {
            return PHONEMESET_N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH;
        }

        // R_ER
        if (phonemeName.CheckIfIsEqualNoCase("R") || phonemeName.CheckIfIsEqualNoCase("ER") || phonemeName.CheckIfIsEqualNoCase("R_ER"))
        {
            return PHONEMESET_R_ER;
        }

        // M_B_P_X
        if (phonemeName.CheckIfIsEqualNoCase("M") || phonemeName.CheckIfIsEqualNoCase("B") || phonemeName.CheckIfIsEqualNoCase("P") || phonemeName.CheckIfIsEqualNoCase("X") || phonemeName.CheckIfIsEqualNoCase("M_B_P_X"))
        {
            return PHONEMESET_M_B_P_X;
        }

        // F_V
        if (phonemeName.CheckIfIsEqualNoCase("F") || phonemeName.CheckIfIsEqualNoCase("V") || phonemeName.CheckIfIsEqualNoCase("F_V"))
        {
            return PHONEMESET_F_V;
        }

        // W
        if (phonemeName.CheckIfIsEqualNoCase("W"))
        {
            return PHONEMESET_W;
        }

        return PHONEMESET_NONE;
    }


    // get the string/name for a given phoneme set
    MCore::String MorphTarget::GetPhonemeSetString(const EPhonemeSet phonemeSet)
    {
        MCore::String result;

        // build the string
        if (phonemeSet & PHONEMESET_NEUTRAL_POSE)
        {
            result += ",NEUTRAL_POSE";
        }
        if (phonemeSet & PHONEMESET_M_B_P_X)
        {
            result += ",M_B_P_X";
        }
        if (phonemeSet & PHONEMESET_AA_AO_OW)
        {
            result += ",AA_AO_OW";
        }
        if (phonemeSet & PHONEMESET_IH_AE_AH_EY_AY_H)
        {
            result += ",IH_AE_AH_EY_AY_H";
        }
        if (phonemeSet & PHONEMESET_AW)
        {
            result += ",AW";
        }
        if (phonemeSet & PHONEMESET_N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH)
        {
            result += ",N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH";
        }
        if (phonemeSet & PHONEMESET_IY_EH_Y)
        {
            result += ",IY_EH_Y";
        }
        if (phonemeSet & PHONEMESET_UW_UH_OY)
        {
            result += ",UW_UH_OY";
        }
        if (phonemeSet & PHONEMESET_F_V)
        {
            result += ",F_V";
        }
        if (phonemeSet & PHONEMESET_L_EL)
        {
            result += ",L_E";
        }
        if (phonemeSet & PHONEMESET_W)
        {
            result += ",W";
        }
        if (phonemeSet & PHONEMESET_R_ER)
        {
            result += ",R_ER";
        }

        // remove any leading commas
        result.Trim(MCore::UnicodeCharacter::comma);

        // return the resulting string
        return result;
    }


    // calculate the weight in range of 0..1
    float MorphTarget::CalcNormalizedWeight(float rangedWeight) const
    {
        const float range = mRangeMax - mRangeMin;
        if (MCore::Math::Abs(range) > 0.0f)
        {
            return (rangedWeight - mRangeMin) / range;
        }
        else
        {
            return 0.0f;
        }
    }


    // there are 12 possible phoneme sets
    uint32 MorphTarget::GetNumAvailablePhonemeSets()
    {
        return 12;
    }


    // enable or disable a given phoneme set
    void MorphTarget::EnablePhonemeSet(EPhonemeSet set, bool enabled)
    {
        if (enabled)
        {
            mPhonemeSets = (EPhonemeSet)((uint32)mPhonemeSets | (uint32)set);
        }
        else
        {
            mPhonemeSets = (EPhonemeSet)((uint32)mPhonemeSets & (uint32) ~set);
        }
    }


    // copy the base class members to the target class
    void MorphTarget::CopyBaseClassMemberValues(MorphTarget* target)
    {
        target->mNameID         = mNameID;
        target->mRangeMin       = mRangeMin;
        target->mRangeMax       = mRangeMax;
        target->mPhonemeSets    = mPhonemeSets;
    }


    const char* MorphTarget::GetName() const
    {
        return MCore::GetStringIDGenerator().GetName(mNameID).AsChar();
    }


    const MCore::String& MorphTarget::GetNameString() const
    {
        return MCore::GetStringIDGenerator().GetName(mNameID);
    }


    void MorphTarget::SetRangeMin(float rangeMin)
    {
        mRangeMin = rangeMin;
    }


    void MorphTarget::SetRangeMax(float rangeMax)
    {
        mRangeMax = rangeMax;
    }


    float MorphTarget::GetRangeMin() const
    {
        return mRangeMin;
    }


    float MorphTarget::GetRangeMax() const
    {
        return mRangeMax;
    }


    void MorphTarget::SetName(const char* name)
    {
        mNameID = MCore::GetStringIDGenerator().GenerateIDForString(name);
    }


    void MorphTarget::SetPhonemeSets(EPhonemeSet phonemeSets)
    {
        mPhonemeSets = phonemeSets;
    }


    MorphTarget::EPhonemeSet MorphTarget::GetPhonemeSets() const
    {
        return mPhonemeSets;
    }


    bool MorphTarget::GetIsPhonemeSetEnabled(EPhonemeSet set) const
    {
        return (mPhonemeSets & set) != 0;
    }


    float MorphTarget::CalcRangedWeight(float weight) const
    {
        return mRangeMin + (weight * (mRangeMax - mRangeMin));
    }


    float MorphTarget::CalcZeroInfluenceWeight() const
    {
        return MCore::Math::Abs(mRangeMin) / MCore::Math::Abs(mRangeMax - mRangeMin);
    }


    bool MorphTarget::GetIsPhoneme() const
    {
        return (mPhonemeSets != PHONEMESET_NONE);
    }
} // namespace EMotionFX
