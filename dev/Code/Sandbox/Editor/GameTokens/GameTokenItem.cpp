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

#include "StdAfx.h"
#include "GameTokenItem.h"
#include "GameEngine.h"
#include "GameTokenLibrary.h"
#include "GameTokenManager.h"
#include <IGameTokens.h>

#define FTN_INT "Int"
#define FTN_FLOAT "Float"
#define FTN_DOUBLE "Double"
#define FTN_VEC3 "Vec3"
#define FTN_STRING "String"
#define FTN_BOOL "Bool"
#define FTN_CUSTOM_DATA "CustomData"

//////////////////////////////////////////////////////////////////////////
inline EFlowDataTypes FlowNameToType(const char* typeName)
{
    EFlowDataTypes tokenType = eFDT_Any;
    if (0 == strcmp(typeName, FTN_INT))
    {
        tokenType = eFDT_Int;
    }
    else if (0 == strcmp(typeName, FTN_FLOAT))
    {
        tokenType = eFDT_Float;
    }
    else if (0 == strcmp(typeName, FTN_DOUBLE))
    {
        tokenType = eFDT_Double;
    }
    else if (0 == strcmp(typeName, FTN_VEC3))
    {
        tokenType = eFDT_Vec3;
    }
    else if (0 == strcmp(typeName, FTN_STRING))
    {
        tokenType = eFDT_String;
    }
    else if (0 == strcmp(typeName, FTN_BOOL))
    {
        tokenType = eFDT_Bool;
    }
    else if (0 == strcmp(typeName, FTN_CUSTOM_DATA))
    {
        tokenType = eFDT_CustomData;
    }

    return tokenType;
}

//////////////////////////////////////////////////////////////////////////
inline const char* FlowTypeToName(EFlowDataTypes tokenType)
{
    // Saving.
    switch (tokenType)
    {
    case eFDT_Int:
        return FTN_INT;
    case eFDT_Float:
        return FTN_FLOAT;
    case eFDT_Double:
        return FTN_DOUBLE;
    case eFDT_Vec3:
        return FTN_VEC3;
    case eFDT_String:
        return FTN_STRING;
    case eFDT_Bool:
        return FTN_BOOL;
    case eFDT_CustomData:
        return FTN_CUSTOM_DATA;
    }
    return "";
}

class CFlowDataReadVisitorEditor
    : public boost::static_visitor<void>
{
public:
    CFlowDataReadVisitorEditor(const char* data)
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
        FlowEntityId* scannedEntityPtr;
        m_ok = 1 == azsscanf(m_data, "%p", &scannedEntityPtr);
        i = *scannedEntityPtr;
    }

    void Visit(AZ::Vector3& v)
    {
        float x(0.0f), y(0.0f), z(0.0f);
        m_ok = 3 == azsscanf(m_data, "%g,%g,%g", &x, &y, &z);
        v.SetX(x);
        v.SetY(y);
        v.SetZ(z);
    }

    void Visit(Vec3& i)
    {
        m_ok = 3 == azsscanf(m_data, "%g,%g,%g", &i.x, &i.y, &i.z);
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
        if (m_ok)
        {
            b = (i != 0);
        }
        else
        {
            if (azstricmp(m_data, "true") == 0)
            {
                m_ok = b = true;
            }
            else if (azstricmp(m_data, "false") == 0)
            {
                m_ok = true;
                b = false;
            }
        }
    }

    void Visit(SFlowSystemVoid&)
    {
    }

    void Visit(FlowCustomData&)
    {
        // currently not supported
    }

    void Visit(SFlowSystemPointer&)
    {
    }

    template <class T>
    void operator()(T& type)
    {
        Visit(type);
    }

    bool operator!() const
    {
        return !m_ok;
    }

private:
    const char* m_data;
    bool m_ok;
};

//////////////////////////////////////////////////////////////////////////
class CFlowDataWriteVisitorEditor
    : public boost::static_visitor<void>
{
public:
    CFlowDataWriteVisitorEditor(string& out)
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
        m_out.Format("%u", i);
    }

    void Visit(AZ::Vector3 i)
    {
        m_out.Format("%g,%g,%g", i.GetX(), i.GetY(), i.GetZ());
    }

    void Visit(Vec3 i)
    {
        m_out.Format("%g,%g,%g", i.x, i.y, i.z);
    }

    void Visit(const string& i)
    {
        m_out = i;
    }

    void Visit(bool b)
    {
        m_out = (b) ? "true" : "false";
    }

    void Visit(SFlowSystemVoid)
    {
        m_out = "";
    }

    void Visit(FlowCustomData)
    {
        // not currently supported
        m_out = "";
    }

    void Visit(SFlowSystemPointer)
    {
        m_out = "";
    }

    template <class T>
    void operator()(T& type)
    {
        Visit(type);
    }

private:
    string& m_out;
};

//////////////////////////////////////////////////////////////////////////
inline string ConvertFlowDataToString(const TFlowInputData& value)
{
    string out;
    CFlowDataWriteVisitorEditor writer(out);
    value.Visit(writer);
    return out;
}

//////////////////////////////////////////////////////////////////////////
inline bool SetFlowDataFromString(TFlowInputData& value, const char* str)
{
    CFlowDataReadVisitorEditor visitor(str);
    value.Visit(visitor);
    return !!visitor;
}



//////////////////////////////////////////////////////////////////////////
CGameTokenItem::CGameTokenItem()
{
    m_pTokenSystem =  GetIEditor()->GetGameEngine()->GetIGameTokenSystem();
    m_value = TFlowInputData((bool)0, true);
}

//////////////////////////////////////////////////////////////////////////
CGameTokenItem::~CGameTokenItem()
{
    if (m_pTokenSystem)
    {
        IGameToken* pToken = m_pTokenSystem->FindToken(m_cachedFullName.toUtf8().data());
        if (pToken)
        {
            m_pTokenSystem->DeleteToken(pToken);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenItem::SetName(const QString& name)
{
    IGameToken* pToken = NULL;
    if (m_pTokenSystem)
    {
        m_pTokenSystem->FindToken(GetFullName().toUtf8().data());
    }
    CBaseLibraryItem::SetName(name);
    if (m_pTokenSystem)
    {
        if (pToken)
        {
            m_pTokenSystem->RenameToken(pToken, GetFullName().toUtf8().data());
        }
        else
        {
            m_pTokenSystem->SetOrCreateToken(GetFullName().toUtf8().data(), m_value);
        }
    }
    m_cachedFullName = GetFullName();
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenItem::Serialize(SerializeContext& ctx)
{
    //CBaseLibraryItem::Serialize( ctx );
    XmlNodeRef node = ctx.node;
    if (ctx.bLoading)
    {
        const char* sTypeName = node->getAttr("Type");
        const char* sDefaultValue = node->getAttr("Value");

        if (!SetTypeName(sTypeName))
        {
            return;
        }
        SetFlowDataFromString(m_value, sDefaultValue);

        QString name = m_name;
        // Loading
        node->getAttr("Name", name);

        // SetName will also set new value
        if (!ctx.bUniqName)
        {
            SetName(name);
        }
        else
        {
            SetName(GetLibrary()->GetManager()->MakeUniqueItemName(name));
        }
        node->getAttr("Description", m_description);

        int localOnly = 0;
        node->getAttr("LocalOnly", localOnly);
        SetLocalOnly(localOnly != 0);
    }
    else
    {
        node->setAttr("Name", m_name.toUtf8().data());
        // Saving.
        const char* sTypeName = FlowTypeToName((EFlowDataTypes)m_value.GetType());
        if (*sTypeName != 0)
        {
            node->setAttr("Type", sTypeName);
        }
        string sValue = ConvertFlowDataToString(m_value);
        node->setAttr("Value", sValue);
        if (!m_description.isEmpty())
        {
            node->setAttr("Description", m_description.toUtf8().data());
        }

        int localOnly = m_localOnly ? 1 : 0;
        node->setAttr("LocalOnly", localOnly);
    }
    m_bModified = false;
}

//////////////////////////////////////////////////////////////////////////
QString CGameTokenItem::GetTypeName() const
{
    return FlowTypeToName((EFlowDataTypes)m_value.GetType());
}

//////////////////////////////////////////////////////////////////////////
QString CGameTokenItem::GetValueString() const
{
    return (const char*)ConvertFlowDataToString(m_value);
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenItem::SetValueString(const char* sValue)
{
    // Skip if the same type already.
    if (0 == QString::compare(GetValueString(), sValue))
    {
        return;
    }

    SetFlowDataFromString(m_value, sValue);
}

//////////////////////////////////////////////////////////////////////////
bool CGameTokenItem::GetValue(TFlowInputData& data) const
{
    data = m_value;
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenItem::SetValue(const TFlowInputData& data, bool bUpdateGTS)
{
    m_value = data;
    if (bUpdateGTS && m_pTokenSystem)
    {
        IGameToken* pToken = m_pTokenSystem->FindToken(m_cachedFullName.toUtf8().data());
        if (pToken)
        {
            pToken->SetValue(m_value);
        }
    }
}
//////////////////////////////////////////////////////////////////////////
bool CGameTokenItem::SetTypeName(const char* typeName)
{
    EFlowDataTypes tokenType = FlowNameToType(typeName);

    // Skip if the same type already.
    if (tokenType == m_value.GetType())
    {
        return true;
    }

    QString prevVal = GetValueString();
    switch (tokenType)
    {
    case eFDT_Int:
        m_value = TFlowInputData((int)0, true);
        break;
    case eFDT_Float:
        m_value = TFlowInputData((float)0, true);
        break;
    case eFDT_Double:
        m_value = TFlowInputData((double)0, true);
        break;
    case eFDT_Vec3:
        m_value = TFlowInputData(Vec3(0, 0, 0), true);
        break;
    case eFDT_String:
        m_value = TFlowInputData(string(""), true);
        break;
    case eFDT_Bool:
        m_value = TFlowInputData((bool)false, true);
        break;
    default:
        // Unknown type.
        return false;
    }
    SetValueString(prevVal.toUtf8().data());
    return true;
}

//////////////////////////////////////////////////////////////////////////
void CGameTokenItem::Update()
{
    // Mark library as modified.
    SetModified();

    if (m_pTokenSystem)
    {
        // Recreate the game token with new default value, and set flags
        if (IGameToken* pToken = m_pTokenSystem->SetOrCreateToken(GetFullName().toUtf8().data(), m_value))
        {
            if (m_localOnly)
            {
                pToken->SetFlags(pToken->GetFlags() | EGAME_TOKEN_LOCALONLY);
            }
            else
            {
                pToken->SetFlags(pToken->GetFlags() & ~EGAME_TOKEN_LOCALONLY);
            }
        }
    }
}
