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


//! Manages parameter info for a TOD node
class CTODNodeDescription
    : public _i_reference_target_t
{
public:
    //-----------------------------------------------------------------------------
    //! Base Class for referencing parameters by ITimeOfDay::ETimeOfDayParamID
    class CControlParamBase
        : public _i_reference_target_t
    {
    public:
        CControlParamBase();

        virtual void SetDefault(float val) = 0;
        virtual void SetDefault(bool val) = 0;
        virtual void SetDefault(Vec4 val) = 0;
        virtual void SetDefault(Vec3 val) = 0;

        virtual void GetDefault(float& val) const = 0;
        virtual void GetDefault(bool& val) const = 0;
        virtual void GetDefault(Vec4& val) const = 0;
        virtual void GetDefault(Vec3& val) const = 0;

        ITimeOfDay::ETimeOfDayParamID m_paramId;

    protected:
        virtual ~CControlParamBase();
    };

    //-----------------------------------------------------------------------------
    //! Holds default value for TOD parameter
    template<typename T>
    class TControlParam
        : public CControlParamBase
    {
    public:
        virtual void SetDefault(float val);
        virtual void SetDefault(bool val);
        virtual void SetDefault(Vec4 val);
        virtual void SetDefault(Vec3 val);

        virtual void GetDefault(float& val) const;
        virtual void GetDefault(bool& val) const;
        virtual void GetDefault(Vec4& val) const;
        virtual void GetDefault(Vec3& val) const;
    protected:
        virtual ~TControlParam();

        T m_defaultValue;
    };

    //-----------------------------------------------------------------------------
    //!
    CTODNodeDescription();

    //-----------------------------------------------------------------------------
    //!
    template<typename T>
    void AddSupportedParam(const char* name, AnimValueType valueType, ITimeOfDay::ETimeOfDayParamID paramId, T defaultValue);
    
    //-----------------------------------------------------------------------------
    //! Contains generic type info for a parameters
    std::vector<CAnimNode::SParamInfo> m_nodeParams;
    //! Contains default values and ITimeOfDay::ETimeOfDayParamID mapping for parameters
    std::vector<_smart_ptr<CControlParamBase>> m_controlParams;
};
