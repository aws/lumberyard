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

#include "CryLegacy_precompiled.h"
#include "FlowSerialize.h"
#include <AzCore/Casting/numeric_cast.h>

class CFlowDataReadVisitor
    : public boost::static_visitor<void>
{
public:
    CFlowDataReadVisitor(const char* data)
        : m_data(data)
        , m_ok(false) {}

    void Visit(int& i)
    {
        m_ok = 1 == azsscanf(m_data, "%i", &i);
    }

    void Visit(float& i)
    {
        m_ok = 1 == azsscanf(m_data, "%f", &i);
    }

    void Visit(double& i)
    {
        m_ok = 1 == azsscanf(m_data, "%lf", &i);
    }

    void Visit(EntityId& i)
    {
        m_ok = 1 == azsscanf(m_data, "%u", &i);
    }

    void Visit(FlowEntityId& i)
    {
        FlowEntityId::StorageType id;
        m_ok = 1 == azsscanf(m_data, "%" PRIu64, &id);
        if (m_ok)
        {
            i = FlowEntityId(aznumeric_cast<EntityId>(id));
        }
    }

    void Visit(AZ::Vector3& i)
    {
        float x = i.GetX();
        float y = i.GetY();
        float z = i.GetZ();
        m_ok = 3 == azsscanf(m_data, "%f,%f,%f", &x, &y, &z);
    }

    void Visit(Vec3& i)
    {
        m_ok = 3 == azsscanf(m_data, "%f,%f,%f", &i.x, &i.y, &i.z);
    }

    void Visit(string& i)
    {
        i = m_data;
        m_ok = true;
    }

    void Visit(bool& b)
    {
        int i;
        m_ok = 1 == azsscanf(m_data, "%i", &i);
        b = i != 0;
    }

    void Visit(SFlowSystemVoid&)
    {
    }

    void Visit(FlowCustomData& dest)
    {
        m_ok = dest.SetFrom(string(m_data));
    }

    void Visit(SFlowSystemPointer&)
    {
    }

    bool operator!() const
    {
        return !m_ok;
    }

    template <class T>
    void operator()(T& type)
    {
        Visit(type);
    }

private:
    const char* m_data;
    bool m_ok;
};

class CFlowDataWriteVisitor
    : public boost::static_visitor<void>
{
public:
    CFlowDataWriteVisitor(string& out)
        : m_out(out) {}

    void Visit(int i)
    {
        m_out.Format("%i", i);
    }

    void Visit(float i)
    {
        m_out.Format("%f", i);
    }

    void Visit(double i)
    {
        m_out.Format("%f", i);
    }

    void Visit(EntityId i)
    {
        m_out.Format("%u", i);
    }

    void Visit(FlowEntityId i)
    {
        m_out.Format("%u", i.GetId());
    }

    void Visit(AZ::Vector3 i)
    {
        m_out.Format("%f,%f,%f", i.GetX(), i.GetY(), i.GetZ());
    }

    void Visit(Vec3 i)
    {
        m_out.Format("%f,%f,%f", i.x, i.y, i.z);
    }

    void Visit(const string& i)
    {
        m_out = i;
    }

    void Visit(bool b)
    {
        Visit(int(b));
    }

    void Visit(SFlowSystemVoid)
    {
        m_out.resize(0);
    }

    void Visit(const FlowCustomData& src)
    {
        src.GetAs(m_out);
    }

    void Visit(SFlowSystemPointer)
    {
        m_out.resize(0);
    }

    template <class T>
    void operator()(T& type)
    {
        Visit(type);
    }

private:
    string& m_out;
};

bool SetFromString(TFlowInputData& value, const char* str)
{
    CFlowDataReadVisitor visitor(str);
    value.Visit(visitor);
    return !!visitor;
}

string ConvertToString(const TFlowInputData& value)
{
    string out;
    CFlowDataWriteVisitor visitor(out);
    value.Visit(visitor);
    return out;
}

bool SetAttr(XmlNodeRef node, const char* attr, const TFlowInputData& value)
{
    string out = ConvertToString(value);
    node->setAttr(attr, out.c_str());
    return true;
}
