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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_NORMALFROMHEIGHT_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_NORMALFROMHEIGHT_H
#pragma once
#include "../Filtering/FIR-Windows.h"

#include <QWidget>

class CBumpProperties
{
public:
    // Arguments:
    //   bAlphaAsBump true=m_AlphaAsBump, false=m_BumpToNormal
    CBumpProperties(const bool bAlphaAsBump)
        : m_pCC(0)
        , m_bAlphaAsBump(bAlphaAsBump)
    {
    }

    // Arguments:
    //  pCC - can be 0
    void SetCC(ConvertContext* pCC)
    {
        m_pCC = pCC;
    }

    void setKeyValue(const char* key, const char* value)
    {
        m_pCC->multiConfig->setKeyValue(eCP_PriorityFile, key, value);
    }

    // ----------------------------------------

    // Arguments:
    //  Value - filename without path and special characters like space, quotes, .. (dot is ok)
    void SetBumpmapName(const string Value)
    {
        if (GetBumpmapName() != Value)
        {
            setKeyValue("bumpname", Value);
        }
    }

    // Returns:
    //  filename without path and special characters like space, quotes, .. (dot is ok)
    string GetBumpmapName(int platform = -1) const
    {
        if (platform < 0)
        {
            platform = m_pCC->platform;
        }
        return m_pCC->multiConfig->getConfig(platform).GetAsString("bumpname", "", "");
    }

    // ----------------------------------------

    // Arguments:
    //  fValue - 0 .. 10.0
    void SetBumpBlurAmount(const float fValue)
    {
        if (GetBumpBlurAmount() != fValue)
        {
            char str[80];
            sprintf_s(str, "%f", fValue);
            setKeyValue(m_bAlphaAsBump ? "bumpblur_a" : "bumpblur", str);
        }
    }

    // Returns:
    //  0 .. 10.0
    float GetBumpBlurAmount(int platform = -1) const
    {
        if (platform < 0)
        {
            platform = m_pCC->platform;
        }
        return Util::getMax(m_pCC->multiConfig->getConfig(platform).GetAsFloat(m_bAlphaAsBump ? "bumpblur_a" : "bumpblur", 0.0f, 0.0f), 0.0f);
    }

    void EnableWindowBumpBlurAmount(QWidget* hWnd) const
    {
        const string Value = m_pCC->multiConfig->getConfig().GetAsString(m_bAlphaAsBump ? "bumpblur_a" : "bumpblur", "", "", eCP_PriorityAll & (~eCP_PriorityFile));
        hWnd->setEnabled(Value.empty() || (GetBumpToNormalFilterIndex() > 0));
    }

    // ----------------------------------------

    // Arguments:
    //  fValue - -1000.0 .. 1000.0
    void SetBumpStrengthAmount(const float fValue)
    {
        if (GetBumpStrengthAmount() != fValue)
        {
            char str[80];
            sprintf_s(str, "%f", fValue);
            setKeyValue(m_bAlphaAsBump ? "bumpstrength_a" : "bumpstrength", str);
        }
    }

    // Returns:
    //  -1000.0 .. 1000.0
    float GetBumpStrengthAmount(int platform = -1) const
    {
        if (platform < 0)
        {
            platform = m_pCC->platform;
        }
        const float defaultVal = 5.0f;  // default like NVidia plugin
        return m_pCC->multiConfig->getConfig(platform).GetAsFloat(m_bAlphaAsBump ? "bumpstrength_a" : "bumpstrength", defaultVal, defaultVal);
    }

    void EnableWindowBumpStrengthAmount(QWidget* hWnd) const
    {
        const string Value = m_pCC->multiConfig->getConfig().GetAsString(m_bAlphaAsBump ? "bumpstrength_a" : "bumpstrength", "", "", eCP_PriorityAll & (~eCP_PriorityFile));
        hWnd->setEnabled(Value.empty() || (GetBumpToNormalFilterIndex() > 0));
    }

    // ----------------------------------------

    static int GetBumpToNormalFilterDefault()
    {
        return 0;
    }

    static int GetBumpToNormalFilterCount()
    {
        return GetFilterFunctionCount();
    }

    static const char* GetBumpToNormalFilterName(int idx)
    {
        if (idx < 0 || idx >= GetBumpToNormalFilterCount())
        {
            idx = GetBumpToNormalFilterDefault();
        }

        return GetFilterFunctionName(idx);
    }

    const int GetBumpToNormalFilterIndex(int platform = -1) const
    {
        if (platform < 0)
        {
            platform = m_pCC->platform;
        }

        const int defaultIndex = GetBumpToNormalFilterDefault();
        const char* const pDefaultName = GetBumpToNormalFilterName(defaultIndex);
        const int n = GetBumpToNormalFilterCount();

        const string name = m_pCC->multiConfig->getConfig(platform).GetAsString(m_bAlphaAsBump ? "bumptype_a" : "bumptype", pDefaultName, pDefaultName);

        int index = GetFilterFunctionIndex(name.c_str(), -1, "0");
        if (index == -1)
        {
            index = defaultIndex;

            RCLogWarning("Unknown filtering window: '%s'. Using '%s'.", name.c_str(), GetFilterFunctionName(defaultIndex));
        }

        return index;
    }

    void SetBumpToNormalFilterIndex(const int idx)
    {
        const int curIdx = GetBumpToNormalFilterIndex();
        if (curIdx == idx)
        {
            return;
        }

        setKeyValue(m_bAlphaAsBump ? "bumptype_a" : "bumptype", GetBumpToNormalFilterName(idx));
    }

    void EnableWindowBumpToNormalFilter(QWidget* hWnd) const
    {
        const string Value = m_pCC->multiConfig->getConfig().GetAsString(m_bAlphaAsBump ? "bumptype_a" : "bumptype", "", "", eCP_PriorityAll & (~eCP_PriorityFile));
        hWnd->setEnabled(Value.empty());
    }

private: // -------------------------------------------------------

    bool m_bAlphaAsBump;   // true: m_AlphaAsBump, false: m_BumpToNormal

protected: // -------------------------------------------------------

    ConvertContext* m_pCC; // can be 0
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILERIMAGE_CONVERTERS_NORMALFROMHEIGHT_H
