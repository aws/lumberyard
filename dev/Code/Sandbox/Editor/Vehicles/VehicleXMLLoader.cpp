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
#include "VehicleXMLLoader.h"

#include <stack>
#include <IReadWriteXMLSink.h>

#include "VehicleData.h"
#include "VehicleXMLHelper.h"


/*
* Implementation of IReadXMLSink, creates IVehicleData structure
*/
class CVehicleDataLoader
    : public IReadXMLSink
{
public:
    CVehicleDataLoader();

    IVehicleData* GetVehicleData() { return m_pData; }

    // IReadXMLSink
    virtual void AddRef();
    virtual void Release();

    virtual IReadXMLSinkPtr BeginTable(const char* name, const XmlNodeRef& definition);
    virtual IReadXMLSinkPtr BeginTableAt(int elem, const XmlNodeRef& definition);
    virtual bool SetValue(const char* name, const TValue& value, const XmlNodeRef& definition);
    virtual bool EndTableAt(int elem);
    virtual bool EndTable(const char* name);

    virtual IReadXMLSinkPtr BeginArray(const char* name, const XmlNodeRef& definition);
    virtual bool SetAt(int elem, const TValue& value, const XmlNodeRef& definition);
    virtual bool EndArray(const char* name);

    virtual bool Complete();

    virtual bool IsCreationMode(){ return m_creationNode != 0; }
    virtual XmlNodeRef GetCreationNode(){ return m_creationNode; }
    virtual void SetCreationNode(XmlNodeRef definition){ m_creationNode = definition; }
    // ~IReadXMLSink

private:
    int m_nRefs;
    IVehicleData* m_pData;
    std::stack<IVariablePtr> m_nodeStack;
    XmlNodeRef m_creationNode;

    IVariablePtr CurNode() { assert(!m_nodeStack.empty()); return m_nodeStack.top(); }

    class SSetValueVisitor
        : public boost::static_visitor<void>
    {
    public:
        SSetValueVisitor(IVariablePtr parent, const char* name, const XmlNodeRef& definition, IReadXMLSink* sink)
            : m_parent(parent)
            , m_name(name)
            , m_def(definition)
            , m_sink(sink)
        {}

        template <class T>
        void Visit(const T& value)
        {
            IVariablePtr node = VehicleXml::CreateVar(value, m_def);
            DoVisit(value, node);
        }

        template <class T>
        void operator()(const T& type)
        {
            Visit(type);
        }

    private:
        template <class T>
        void DoVisit(const T& value, IVariablePtr node)
        {
            node->SetName(m_name);
            node->Set(value);

            VehicleXml::SetExtendedVarProperties(node, m_def);

            m_parent->AddVariable(node);
        }

        IVariablePtr m_parent;
        const char* m_name;
        const XmlNodeRef& m_def;
        IReadXMLSink* m_sink;
    };

    class SSetValueAtVisitor
        : public boost::static_visitor<void>
    {
    public:
        SSetValueAtVisitor(IVariablePtr parent, int elem, const XmlNodeRef& definition, IReadXMLSink* sink)
            : m_parent(parent)
            , m_elem(elem)
            , m_def(definition)
            , m_sink(sink)
        {}

        template <class T>
        void Visit(const T& value)
        {
            IVariablePtr node = VehicleXml::CreateVar(value, m_def);
            DoVisit(value, node);
        }

        template <class T>
        void operator()(const T& type)
        {
            Visit(type);
        }

    private:
        template <class T>
        void DoVisit(const T& value, IVariablePtr node)
        {
            QString s;
            //s.Format(_T("%d"), m_elem);
            s = m_def->getAttr("elementName");

            node->SetName(s);
            node->Set(value);

            VehicleXml::SetExtendedVarProperties(node, m_def);

            m_parent->AddVariable(node);
        }

        IVariablePtr m_parent;
        int m_elem;
        const XmlNodeRef& m_def;
        IReadXMLSink* m_sink;
    };
};

TYPEDEF_AUTOPTR(CVehicleDataLoader);
typedef CVehicleDataLoader_AutoPtr CVehicleDataLoaderPtr;


CVehicleDataLoader::CVehicleDataLoader()
    : m_nRefs(0)
    , m_creationNode(0)
{
    m_pData = new CVehicleData;
    m_nodeStack.push(m_pData->GetRoot());
}

void CVehicleDataLoader::AddRef()
{
    ++m_nRefs;
}

void CVehicleDataLoader::Release()
{
    if (0 == --m_nRefs)
    {
        delete this;
    }
}

IReadXMLSinkPtr CVehicleDataLoader::BeginTable(const char* name, const XmlNodeRef& definition)
{
    CVariableArray* arr = new CVariableArray();
    arr->SetName(name);
    m_nodeStack.push(arr);

    return this;
}

IReadXMLSinkPtr CVehicleDataLoader::BeginTableAt(int elem, const XmlNodeRef& definition)
{
    CVariableArray* arr = new CVariableArray();

    QString s;
    //s.Format(_T("%d"), elem);
    s = definition->getAttr("elementName");

    arr->SetName(s);
    m_nodeStack.push(arr);

    return this;
}

bool CVehicleDataLoader::SetValue(const char* name, const TValue& value, const XmlNodeRef& definition)
{
    SSetValueVisitor visitor(CurNode(), name, definition, this);
    boost::apply_visitor(visitor, value);
    return true;
}

bool CVehicleDataLoader::EndTable(const char* name)
{
    IVariablePtr newNode = CurNode();
    m_nodeStack.pop();
    CurNode()->AddVariable(newNode);   // add child
    return true;
}

bool CVehicleDataLoader::EndTableAt(int elem)
{
    return EndTable(0);
}

IReadXMLSinkPtr CVehicleDataLoader::BeginArray(const char* name, const XmlNodeRef& definition)
{
    IVariablePtr arr = new CVariableArray();
    arr->SetName(name);

    VehicleXml::SetExtendedVarProperties(arr, definition);

    m_nodeStack.push(arr);
    return this;
}

bool CVehicleDataLoader::SetAt(int elem, const TValue& value, const XmlNodeRef& definition)
{
    SSetValueAtVisitor visitor(CurNode(), elem, definition, this);
    boost::apply_visitor(visitor, value);
    return true;
}

bool CVehicleDataLoader::EndArray(const char* name)
{
    return EndTable(name);
}

bool CVehicleDataLoader::Complete()
{
    return true;
}



IVehicleData* VehicleDataLoad(const char* definitionFile, const char* dataFile, bool bFillDefaults)
{
    XmlNodeRef definition = GetISystem()->LoadXmlFromFile(definitionFile);
    if (!definition)
    {
        Warning("VehicleDataLoad: unable to load definition file %s", definitionFile);
        return 0;
    }
    return VehicleDataLoad(definition, dataFile, bFillDefaults);
}

IVehicleData* VehicleDataLoad(const XmlNodeRef& definition, const char* dataFile, bool bFillDefaults)
{
    CVehicleDataLoaderPtr pLoader(new CVehicleDataLoader);
    if (GetISystem()->GetXmlUtils()->GetIReadWriteXMLSink()->ReadXML(definition, dataFile, &*pLoader))
    {
        return pLoader->GetVehicleData();
    }
    else
    {
        return NULL;
    }
}

IVehicleData* VehicleDataLoad(const XmlNodeRef& definition, const XmlNodeRef& data, bool bFillDefaults)
{
    CVehicleDataLoaderPtr pLoader(new CVehicleDataLoader);
    if (GetISystem()->GetXmlUtils()->GetIReadWriteXMLSink()->ReadXML(definition, data, &*pLoader))
    {
        return pLoader->GetVehicleData();
    }
    else
    {
        return NULL;
    }
}

//
// end of CVehicleDataLoader implementation
