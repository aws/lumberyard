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

// Description : Variadic parameters class.


#ifndef CRYINCLUDE_CRYCOMMON_VARIADICPARAMS_H
#define CRYINCLUDE_CRYCOMMON_VARIADICPARAMS_H
#pragma once


template <typename PARAM>
class CVariadicParams
{
public:

    typedef PARAM TParam;

    enum
    {
        MaxParamCount = 8
    };

    inline CVariadicParams()
        : m_paramCount(0) {}

    inline CVariadicParams(const TParam& param1)
        : m_paramCount(1)
    {
        m_params[0] = param1;
    }

    inline CVariadicParams(const TParam& param1, const TParam& param2)
        : m_paramCount(2)
    {
        m_params[0] = param1;
        m_params[1] = param2;
    }

    inline CVariadicParams(const TParam& param1, const TParam& param2, const TParam& param3)
        : m_paramCount(3)
    {
        m_params[0] = param1;
        m_params[1] = param2;
        m_params[2] = param3;
    }

    inline CVariadicParams(const TParam& param1, const TParam& param2, const TParam& param3, const TParam& param4)
        : m_paramCount(4)
    {
        m_params[0] = param1;
        m_params[1] = param2;
        m_params[2] = param3;
        m_params[3] = param4;
    }

    inline CVariadicParams(const TParam& param1, const TParam& param2, const TParam& param3, const TParam& param4, const TParam& param5)
        : m_paramCount(5)
    {
        m_params[0] = param1;
        m_params[1] = param2;
        m_params[2] = param3;
        m_params[3] = param4;
        m_params[4] = param5;
    }

    inline CVariadicParams(const TParam& param1, const TParam& param2, const TParam& param3, const TParam& param4, const TParam& param5, const TParam& param6)
        : m_paramCount(6)
    {
        m_params[0] = param1;
        m_params[1] = param2;
        m_params[2] = param3;
        m_params[3] = param4;
        m_params[4] = param5;
        m_params[5] = param6;
    }

    inline CVariadicParams(const TParam& param1, const TParam& param2, const TParam& param3, const TParam& param4, const TParam& param5, const TParam& param6, const TParam& param7)
        : m_paramCount(7)
    {
        m_params[0] = param1;
        m_params[1] = param2;
        m_params[2] = param3;
        m_params[3] = param4;
        m_params[4] = param5;
        m_params[5] = param6;
        m_params[6] = param7;
    }

    inline CVariadicParams(const TParam& param1, const TParam& param2, const TParam& param3, const TParam& param4, const TParam& param5, const TParam& param6, const TParam& param7, const TParam& param8)
        : m_paramCount(8)
    {
        m_params[0] = param1;
        m_params[1] = param2;
        m_params[2] = param3;
        m_params[3] = param4;
        m_params[4] = param5;
        m_params[5] = param6;
        m_params[6] = param7;
        m_params[7] = param8;
    }

    inline CVariadicParams(const CVariadicParams& rhs)
        : m_paramCount(rhs.m_paramCount)
    {
        for (size_t iParam = 0; iParam < m_paramCount; ++iParam)
        {
            m_params[iParam] = rhs.m_params[iParam];
        }
    }

    inline bool PushParam(const TParam& param)
    {
#if TELEMETRY_DEBUG
        CRY_ASSERT(m_paramCount < MaxParamCount);
#endif //TELEMETRY_DEBUG

        m_params[m_paramCount++] = param;

        return true;
    }

    inline size_t ParamCount() const
    {
        return m_paramCount;
    }

    inline const TParam& operator [] (size_t i) const
    {
#if TELEMETRY_DEBUG
        CRY_ASSERT(i < m_paramCount);
#endif //TELEMETRY_DEBUG

        return m_params[i];
    }

private:

    TParam  m_params[MaxParamCount];

    size_t  m_paramCount;
};

#endif // CRYINCLUDE_CRYCOMMON_VARIADICPARAMS_H
