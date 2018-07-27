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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHVARIABLES_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHVARIABLES_H
#pragma once

#include <Util/Variable.h>

#define FGVARIABLES_REAL_CLONE
// #undef  FGVARIABLES_REAL_CLONE  // !!! Not working without real cloning currently !!!

// REMARKS:
// FlowGraph uses its own set of CVariables
// These Variables handle the IGetCustomItems stored in the UserData of the
// Variables
// If FGVARIABLES_REAL_CLONE is defined every CVariableFlowNode uses its own instance of CFlowNodeGetCustomItemBase
// If FGVARIABLES_REAL_CLONE is not set, there is only one instance of CFlowNodeGetCustomItemBase per port type
// which is shared and pointers are adjusted in CFlowNode::GetInputsVarBlock()
// The later is not working correctly with Undo/Redo.

class CFlowNode;
class CFlowNodeGetCustomItemsBase;

namespace FlowGraphVariables
{
    template <typename T>
    CFlowNodeGetCustomItemsBase* CreateIt() { return new T(); };

    // A simple struct to map prefixes to data types
    struct MapEntry
    {
        const char*                                 sPrefix;    // prefix string (including '_' character)
        IVariable::EDataType                eDataType;
        CFlowNodeGetCustomItemsBase* (* pGetCustomItemsCreator) ();
    };

    const MapEntry* FindEntry(const char* sPrefix);
};

//////////////////////////////////////////////////////////////////////////
class CFlowNodeGetCustomItemsBase
    : public IVariable::IGetCustomItems
{
public:
    virtual CFlowNodeGetCustomItemsBase* Clone() const
    {
#ifdef FGVARIABLES_REAL_CLONE
        CFlowNodeGetCustomItemsBase* pNew = new CFlowNodeGetCustomItemsBase();
        pNew->m_pFlowNode = m_pFlowNode;
        pNew->m_config = m_config;
        return pNew;
#else
        return const_cast<CFlowNodeGetCustomItemsBase*> (this);
#endif
    }

    CFlowNodeGetCustomItemsBase()
        : m_pFlowNode(0)
    {
        // CryLogAlways("CFlowNodeGetCustomItemsBase::ctor 0x%p", this);
    }

    virtual ~CFlowNodeGetCustomItemsBase()
    {
        // CryLogAlways("CFlowNodeGetCustomItemsBase::dtor 0x%p", this);
    }

    virtual bool GetItems (IVariable* /* pVar */, std::vector<SItem>& /* items */, QString& /* outDialogTitle */)
    {
        return false;
    }

    virtual bool UseTree()
    {
        return false;
    }

    virtual const char* GetTreeSeparator()
    {
        return 0;
    }

    void SetFlowNode(CFlowNode* pFlowNode)
    {
        m_pFlowNode = pFlowNode;
    }

    CFlowNode* GetFlowNode() const
    {
        // assert (m_pFlowNode != 0);
        return m_pFlowNode;
    }

    void SetUIConfig(const char* sUIConfig)
    {
        m_config = sUIConfig;
    }

    const QString& GetUIConfig() const
    {
        return m_config;
    }
protected:
    CFlowNode* m_pFlowNode;
    QString m_config;
private:
    CFlowNodeGetCustomItemsBase(const CFlowNodeGetCustomItemsBase&) {};
};

//////////////////////////////////////////////////////////////////////////
template<class T>
class CVariableFlowNode
    : public CVariable<T>
{
public:
    typedef CVariableFlowNode<T> Self;

    CVariableFlowNode(){}

    IVariable* Clone(bool bRecursive) const
    {
        Self* var = new Self(*this);
        return var;
    }

protected:
    using CVariable<T>::m_userData;
    
    CVariableFlowNode(const Self& other)
        : CVariable<T> (other)
    {
        if (static_cast<int>(other.m_userData.type()) == QMetaType::VoidStar)
        {
            CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (qvariant_cast<void *>(other.m_userData));
            if (pGetCustomItems)
            {
                pGetCustomItems = pGetCustomItems->Clone();
                pGetCustomItems->AddRef();
                m_userData = QVariant::fromValue<void *>(pGetCustomItems);
            }
        }
    }

    virtual ~CVariableFlowNode()
    {
        if (static_cast<int>(m_userData.type()) == QMetaType::VoidStar)
        {
            CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (qvariant_cast<void *>(m_userData));
            if (pGetCustomItems)
            {
                pGetCustomItems->Release();
            }
        }
    }

    virtual QString GetDisplayValue() const
    {
        return CVariable<T>::GetDisplayValue();
    }

    virtual void SetDisplayValue(const QString& value)
    {
        CVariable<T>::SetDisplayValue(value);
    }
};

// explicit member template specialization in case of Vec3
// if it is a color, return values in range [0,255] instead of [0,1] (just like PropertyItem)
template<>
inline QString CVariableFlowNode<Vec3>::GetDisplayValue() const
{
    if (m_dataType != DT_COLOR)
    {
        return CVariable<Vec3>::GetDisplayValue();
    }
    else
    {
        Vec3 v;
        Get(v);
        int r = v.x * 255;
        int g = v.y * 255;
        int b = v.z * 255;
        r = max(0, min(r, 255));
        g = max(0, min(g, 255));
        b = max(0, min(b, 255));
        return QStringLiteral("%1,%2,%3").arg(r).arg(g).arg(b);
    }
}

template<>
inline void CVariableFlowNode<Vec3>::SetDisplayValue(const QString& value)
{
    if (m_dataType != DT_COLOR)
    {
        CVariable<Vec3>::Set(value);
    }
    else
    {
        float r, g, b;
        int res = azsscanf(value.toUtf8().data(), "%f,%f,%f", &r, &g, &b);
        if (res == 3)
        {
            r /= 255.0f;
            g /= 255.0f;
            b /= 255.0f;
            Set(Vec3(r, g, b));
        }
        else
        {
            CVariable<Vec3>::Set(value);
        }
    }
}


template<class T>
class CVariableFlowNodeEnum
    : public CVariableEnum<T>
{
public:
    typedef CVariableFlowNodeEnum<T> Self;
    CVariableFlowNodeEnum(){}

    IVariable* Clone(bool bRecursive) const
    {
        Self* var = new Self(*this);
        return var;
    }

protected:
    using CVariableEnum<T>::m_userData;

    CVariableFlowNodeEnum(const Self& other)
        : CVariableEnum<T> (other)
    {
        if (static_cast<int>(other.m_userData.type()) == QMetaType::VoidStar)
        {
            CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (qvariant_cast<void *>(other.m_userData));
            if (pGetCustomItems)
            {
                pGetCustomItems = pGetCustomItems->Clone();
                pGetCustomItems->AddRef();
                m_userData = QVariant::fromValue<void *>(pGetCustomItems);
            }
        }
    }

    virtual ~CVariableFlowNodeEnum()
    {
        if (static_cast<int>(m_userData.type()) == QMetaType::VoidStar)
        {
            CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (qvariant_cast<void *>(m_userData));
            if (pGetCustomItems)
            {
                pGetCustomItems->Release();
            }
        }
    }
};

class CVariableFlowNodeVoid
    : public CVariableVoid
{
public:
    typedef CVariableFlowNodeVoid Self;
    CVariableFlowNodeVoid(){}

    IVariable* Clone(bool bRecursive) const
    {
        Self* var = new Self(*this);
        return var;
    }

protected:
    using CVariableVoid::m_userData;
    
    CVariableFlowNodeVoid(const Self& other)
        : CVariableVoid (other)
    {
        if (static_cast<int>(other.m_userData.type()) == QMetaType::VoidStar)
        {
            CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (qvariant_cast<void *>(other.m_userData));
            if (pGetCustomItems)
            {
                pGetCustomItems = pGetCustomItems->Clone();
                pGetCustomItems->AddRef();
                m_userData = QVariant::fromValue<void *>(pGetCustomItems);
            }
        }
    }

    virtual ~CVariableFlowNodeVoid()
    {
        if (static_cast<int>(m_userData.type()) == QMetaType::VoidStar)
        {
            CFlowNodeGetCustomItemsBase* pGetCustomItems = static_cast<CFlowNodeGetCustomItemsBase*> (qvariant_cast<void *>(m_userData));
            if (pGetCustomItems)
            {
                pGetCustomItems->Release();
            }
        }
    }
};

class CVariableFlowNodeCustomData
    : public CVariableBase
{
public:

    CVariableFlowNodeCustomData()
    {
    }

    EType GetType() const override
    {
        return IVariable::FLOW_CUSTOM_DATA;
    };

    IVariable* Clone(bool bRecursive) const override
    {
        return new CVariableFlowNodeCustomData(*this);
    }

    void CopyValue(IVariable* fromVar) override
    {
        // required by CVariableBase, not used for custom flow node data
        assert(false);
    };

    bool HasDefaultValue() const override
    {
        return false;
    }

    void ResetToDefault() override
    {
        // required by CVariableBase, not implemented for custom flow node data
        AZ_Assert(false, "Required by CVariableBase, not implemented for custom flow node data");
    }

    void SetDisplayValue(const QString& value) override
    {
        m_displayValue = value;
    }

    virtual QString GetDisplayValue() const override
    {
        return m_displayValue;
    }

    int GetFlags() const override
    {
        return UI_DISABLED;
    }

    void Serialize(XmlNodeRef node, bool load) override
    {
        // Custom port data values are not serialized into the
        // flow graph xml. Port values initially take on the
        // default value as provided by the input port
        // definition.
    }

protected:

    CVariableFlowNodeCustomData(const CVariableFlowNodeCustomData& other)
        : CVariableBase(other)
        , m_displayValue(other.m_displayValue)
    {
    }

    virtual ~CVariableFlowNodeCustomData()
    {
    }

private:

    QString m_displayValue;
};

struct CUIEnumsDatabase_SEnum;

class CVariableFlowNodeCustomEnumBase
    : public CVariableFlowNode<QString>
{
public:
    CVariableFlowNodeCustomEnumBase()
    {
        m_pDynEnumList = new CDynamicEnumList(this);
    }

    IVarEnumList* GetEnumList() const
    {
        return m_pDynEnumList;
    }

    QString GetDisplayValue() const
    {
        if (m_pDynEnumList)
        {
            return m_pDynEnumList->ValueToName(m_valueDef);
        }
        else
        {
            return CVariableFlowNode<QString>::GetDisplayValue();
        }
    }

    void SetDisplayValue(const QString& value)
    {
        if (m_pDynEnumList)
        {
            CVariableFlowNode<QString>::Set(m_pDynEnumList->NameToValue(value));
        }
        else
        {
            CVariableFlowNode<QString>::Set(value);
        }
    }

protected:
    CVariableFlowNodeCustomEnumBase(const CVariableFlowNodeCustomEnumBase& other)
        : CVariableFlowNode<QString> (other)
    {
        m_pDynEnumList = new CDynamicEnumList(this);
    }

    virtual CUIEnumsDatabase_SEnum* GetSEnum() = 0;

    // some inner class
    class CDynamicEnumList
        : public CVarEnumListBase<QString>
    {
    public:
        CDynamicEnumList(CVariableFlowNodeCustomEnumBase* pDynEnum);
        virtual ~CDynamicEnumList();

        //! Get the name of specified value in enumeration, or NULL if out of range.
        virtual QString GetItemName(uint index);

        virtual QString NameToValue(const QString& name);
        virtual QString ValueToName(const QString& value);

        //! Don't add anything to a global enum database
        virtual void AddItem(const QString& name, const QString& value) {}

    private:
        CVariableFlowNodeCustomEnumBase* m_pDynEnum;
    };

protected:
    TSmartPtr<CDynamicEnumList> m_pDynEnumList;
    QString m_refPort;
    QString m_refFormatString;
};

// looks up value in a different variable[port] and provides enum accordingly
class CVariableFlowNodeDynamicEnum
    : public CVariableFlowNodeCustomEnumBase
{
public:
    CVariableFlowNodeDynamicEnum(const QString& refFormatString, const QString& refPort)
    {
        m_refPort = refPort;
        m_refFormatString = refFormatString;
    }

    IVariable* Clone(bool bRecursive) const
    {
        CVariableFlowNodeDynamicEnum* var = new CVariableFlowNodeDynamicEnum(*this);
        return var;
    }

protected:
    CVariableFlowNodeDynamicEnum(const CVariableFlowNodeDynamicEnum& other)
        : CVariableFlowNodeCustomEnumBase (other)
    {
        m_refPort = other.m_refPort;
        m_refFormatString = other.m_refFormatString;
    }

    virtual CUIEnumsDatabase_SEnum* GetSEnum();

protected:
    QString m_refPort;
    QString m_refFormatString;
};

// looks up value via a define given through a callback and provides enum accordingly
class CVariableFlowNodeDefinedEnum
    : public CVariableFlowNodeCustomEnumBase
{
public:
    CVariableFlowNodeDefinedEnum(const QString& refFormatString, uint32 portId)
    {
        m_refFormatString = refFormatString;
        m_portId = portId;
    }

    IVariable* Clone(bool bRecursive) const
    {
        CVariableFlowNodeDefinedEnum* var = new CVariableFlowNodeDefinedEnum(*this);
        return var;
    }

protected:
    CVariableFlowNodeDefinedEnum(const CVariableFlowNodeDefinedEnum& other)
        : CVariableFlowNodeCustomEnumBase (other)
    {
        m_refFormatString = other.m_refFormatString;
        m_portId = other.m_portId;
    }

    virtual CUIEnumsDatabase_SEnum* GetSEnum();

protected:
    QString m_refFormatString;
    uint32 m_portId;
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_FLOWGRAPHVARIABLES_H
