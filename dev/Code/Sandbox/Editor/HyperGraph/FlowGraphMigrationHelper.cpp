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
#include "FlowGraphMigrationHelper.h"

#define DEPRECATIONS_FILE_PATH "Libs/FlowNodes/Substitutions.XML"

namespace
{
    const QString DISCARD = "__discard__";
};

struct CInputValueCondition
    : public CFlowGraphMigrationHelper::ICondition
{
    CInputValueCondition(const QString& port, const QString& val)
        : m_port(port)
    {
        m_vals = val.split(",", QString::SkipEmptyParts);
    }

    bool Fulfilled(XmlNodeRef node, CFlowGraphMigrationHelper::NodeEntry& entry)
    {
        if (m_vals.isEmpty())
        {
            return true;
        }

        XmlNodeRef inputsXml = node->findChild("Inputs");
        if (!inputsXml)
        {
            return false;
        }

        // go over all inputs and check them
        for (int j = 0; j < inputsXml->getNumAttributes(); j++)
        {
            const char* key;
            const char* value;
            bool success = inputsXml->getAttributeByIndex(j, &key, &value);
            if (!success)
            {
                return false;
            }
            if (m_port.compare(key, Qt::CaseInsensitive) == 0)
            {
                for (int i = 0; i < m_vals.size(); ++i)
                {
                    const QString& refVal = m_vals[i];
                    if (refVal.compare(value, Qt::CaseInsensitive) == 0)
                    {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    QString m_port;
    QStringList m_vals;
};


CFlowGraphMigrationHelper::CFlowGraphMigrationHelper()
    : m_bLoaded(false)
    , m_bInitialized(false)
{
}

CFlowGraphMigrationHelper::~CFlowGraphMigrationHelper()
{
    ReleaseLUAFuncs();
}

void CFlowGraphMigrationHelper::ReleaseLUAFuncs()
{
    if (m_transformFuncs.empty())
    {
        return;
    }

    // get ptr to script system
    IScriptSystem* pScriptSystem = gEnv->pScriptSystem;
    std::vector<HSCRIPTFUNCTION>::iterator iter = m_transformFuncs.begin();
    std::vector<HSCRIPTFUNCTION>::iterator end = m_transformFuncs.end();
    while (iter != m_transformFuncs.end())
    {
        HSCRIPTFUNCTION func = *iter;
        pScriptSystem->ReleaseFunc(func);
        ++iter;
    }
    m_transformFuncs.resize(0);
}

bool
CFlowGraphMigrationHelper::Substitute(XmlNodeRef node)
{
    if (!m_bLoaded)
    {
        m_bInitialized = LoadSubstitutions();
        if (m_bInitialized == false)
        {
            QString filename = DEPRECATIONS_FILE_PATH;
            Error(QObject::tr("Error while loading FlowGraph Substitution File: %1. Substitution will NOT work!").arg(filename).toUtf8().data());
        }
        m_bLoaded = true;
    }

    if (!m_bInitialized)
    {
        return false;
    }

    // get ptr to script system
    IScriptSystem* pScriptSystem = gEnv->pScriptSystem;

    // clear report
    m_report.resize(0);

    // we remember the old class name with a map
    // NodeId -> OldName
    std::map<HyperNodeID, QString> nodeToNameMap;

    XmlNodeRef nodesXml = node->findChild("Nodes");
    if (nodesXml)
    {
        HyperNodeID nodeId;
        QString nodeclass;
        for (int i = 0; i < nodesXml->getChildCount(); i++)
        {
            XmlNodeRef nodeXml = nodesXml->getChild(i);
            if (!nodeXml->getAttr("Class", nodeclass))
            {
                continue;
            }
            if (!nodeXml->getAttr("Id", nodeId))
            {
                continue;
            }

            NodeMappings::iterator iter = m_nodeMappings.find(nodeclass);
            if (iter != m_nodeMappings.end() && EvalConditions(nodeXml, iter->second) == true)
            {
                NodeEntry& nodeEntry = iter->second;

                // store old name
                nodeToNameMap[nodeId] = nodeclass;
                // store new class name in XML node
                nodeXml->setAttr("Class", nodeEntry.newName.toUtf8().data());

                // report
                if (nodeclass.compare(nodeEntry.newName, Qt::CaseInsensitive) != 0)
                {
                    ReportEntry reportEntry;
                    reportEntry.nodeId = nodeId;
                    reportEntry.description = QStringLiteral("NodeID %1: NodeClass '%2' replaced by '%3'")
                        .arg(nodeId).arg(nodeclass, nodeEntry.newName);
                    m_report.push_back(reportEntry);
                }

                // value re-mapping
                XmlNodeRef inputsXml = nodeXml->findChild("Inputs");

                // assign new default input values
                if (nodeEntry.inputValues.empty() == false)
                {
                    if (inputsXml == 0)
                    {
                        inputsXml = nodeXml->newChild("Inputs");
                    }
                    InputValues::iterator valIter = nodeEntry.inputValues.begin();
                    while (valIter != nodeEntry.inputValues.end())
                    {
                        inputsXml->setAttr(valIter->first.toUtf8().data(), valIter->second.toUtf8().data());
                        ReportEntry reportEntry;
                        reportEntry.nodeId = nodeId;
                        reportEntry.description = QStringLiteral("NodeID %1: NodeClass '%2': Setting input value '%3' to '%4'")
                            .arg(nodeId).arg(nodeclass, valIter->first, valIter->second);
                        m_report.push_back(reportEntry);
                        ++valIter;
                    }
                }

                if (inputsXml)
                {
                    // go over all inputs and if we find a port mapping add this attribute
                    for (int j = 0; j < inputsXml->getNumAttributes(); j++)
                    {
                        const char* key;
                        const char* value;
                        bool success = inputsXml->getAttributeByIndex(j, &key, &value);
                        if (success)
                        {
                            assert (key != 0);
                            assert (value != 0);
                            // see if we have a port mapping
                            PortMappings::iterator portIter = nodeEntry.inputPortMappings.find(key);
                            if (portIter != nodeEntry.inputPortMappings.end())
                            {
                                PortEntry& portEntry = portIter->second;
                                // CryLogAlways("PortEntry: key=%s value=%s newval=%s", key,value,portEntry.newName.GetString());

                                // do we have to remap [and potentially transform] the input value?
                                if (portEntry.bRemapValue)
                                {
                                    const char* txStatus = "";
                                    const char* newValue = value;
                                    // transform the value?
                                    if (portEntry.transformFunc != 0)
                                    {
                                        // yes. fetch function
                                        if (Script::CallReturn(pScriptSystem, portEntry.transformFunc, value, newValue) == false)
                                        {
                                            newValue = value;
                                            txStatus = "[TX Failed]";
                                        }
                                        else
                                        {
                                            txStatus = "[TX OK!]";
                                        }
                                    }
                                    else
                                    {
                                        newValue = value;
                                    }

                                    // set new value
                                    inputsXml->setAttr(portEntry.newName.toUtf8().data(), newValue);
                                    ReportEntry reportEntry;
                                    reportEntry.nodeId = nodeId;
                                    reportEntry.description = QStringLiteral("NodeID %1: Remapped attribute '%2' to '%3' value='%4' %5")
                                        .arg(nodeId).arg(key, portEntry.newName, newValue, txStatus);
                                    m_report.push_back(reportEntry);
                                }
                            }
                        }
                    }
                }
            }
        }
    }


    bool bLoadEdges = true;
    if (bLoadEdges)
    {
        XmlNodeRef edgesXml = node->findChild("Edges");
        if (edgesXml)
        {
            HyperNodeID nodeIn = 0, nodeOut = 0;
            QString portIn, portOut;
            QString newPortIn, newPortOut;
            std::vector<XmlNodeRef> toRemoveVec;
            for (int i = 0; i < edgesXml->getChildCount(); i++)
            {
                XmlNodeRef edgeXml = edgesXml->getChild(i);

                edgeXml->getAttr("nodeIn", nodeIn);
                edgeXml->getAttr("nodeOut", nodeOut);
                edgeXml->getAttr("portIn", portIn);
                edgeXml->getAttr("portOut", portOut);

                newPortIn = portIn;
                newPortOut = portOut;

                // remap edges

                // check if nodeIn has been replaced
                {
                    std::map<HyperNodeID, QString>::iterator iter = nodeToNameMap.find(nodeIn);
                    if (iter != nodeToNameMap.end())
                    {
                        // yep
                        const QString& oldName = iter->second;
                        // find node entry
                        NodeMappings::iterator mapIter (m_nodeMappings.find(oldName));
                        assert (mapIter != m_nodeMappings.end());
                        NodeEntry& entry = mapIter->second;
                        // now check if we find a port to be remapped
                        PortMappings::iterator portIter = entry.inputPortMappings.find(portIn);
                        if (portIter != entry.inputPortMappings.end())
                        {
                            const PortEntry& portEntry = portIter->second;
                            assert (portEntry.bIsOutput == false);
                            newPortIn = portEntry.newName;
                        }
                    }
                }
                // check if nodeOut has been replaced
                {
                    std::map<HyperNodeID, QString>::iterator iter = nodeToNameMap.find(nodeOut);
                    if (iter != nodeToNameMap.end())
                    {
                        // yep
                        const QString& oldName = iter->second;
                        // find node entry
                        NodeMappings::iterator mapIter (m_nodeMappings.find(oldName));
                        assert (mapIter != m_nodeMappings.end());
                        NodeEntry& entry = mapIter->second;
                        // now check if we find a port to be remapped
                        PortMappings::iterator portIter = entry.outputPortMappings.find(portOut);
                        if (portIter != entry.outputPortMappings.end())
                        {
                            const PortEntry& portEntry = portIter->second;
                            assert (portEntry.bIsOutput == true);
                            newPortOut = portEntry.newName;
                        }
                    }
                }

                if (newPortIn == DISCARD || newPortOut == DISCARD)
                {
                    // mark this edge as to be removed
                    toRemoveVec.push_back(edgeXml);
                }
                else
                {
                    edgeXml->setAttr("portIn", newPortIn.toUtf8().data());
                    edgeXml->setAttr("portOut", newPortOut.toUtf8().data());
                    if (newPortIn != portIn || newPortOut != portOut)
                    {
                        ReportEntry reportEntry;
                        reportEntry.nodeId = nodeOut;
                        reportEntry.description = QStringLiteral("Edge <%1,%2> -> <%3,%4> replaced by '%5,%6' -> '%7,%8'")
                            .arg(nodeOut).arg(portOut).arg(nodeIn).arg(portIn)
                            .arg(nodeOut).arg(newPortOut).arg(nodeIn).arg(newPortIn);
                        m_report.push_back(reportEntry);
                    }
                }
            }
            // do we have edges to remove?
            if (toRemoveVec.empty() == false)
            {
                std::vector<XmlNodeRef>::iterator iter = toRemoveVec.begin();
                std::vector<XmlNodeRef>::iterator end = toRemoveVec.end();
                while (iter != end)
                {
                    ReportEntry reportEntry;
                    reportEntry.nodeId = nodeOut;
                    reportEntry.description = QStringLiteral("Edge <%1,%2> -> <%3,%4> has been DISCARDED!")
                        .arg(nodeOut).arg(portOut).arg(nodeIn).arg(portIn);
                    m_report.push_back(reportEntry);
                    edgesXml->removeChild(*iter);
                    ++iter;
                }
            }
        }
    }
    // if reports has entries we definitely changed something
    return m_report.size() > 0;
}

bool CFlowGraphMigrationHelper::EvalConditions(XmlNodeRef node, NodeEntry& entry)
{
    if (entry.conditions.empty())
    {
        return true;
    }
    ConditionVector::const_iterator iter = entry.conditions.begin();
    ConditionVector::const_iterator end = entry.conditions.end();
    while (iter != end)
    {
        IConditionPtr pCond = *iter;
        if (pCond->Fulfilled(node, entry) == false)
        {
            return false;
        }
        ++iter;
    }
    return true;
}

void CFlowGraphMigrationHelper::AddEntry(XmlNodeRef node)
{
    NodeEntry entry;

    QString oldClass;
    bool success;
    success = node->getAttr("OldClass", oldClass);
    if (!success)
    {
        Error(QObject::tr("CFlowGraphMigrationHelper::AddEntry: No attribute 'OldClass'").toUtf8().data());
        return;
    }
    success = node->getAttr("NewClass", entry.newName);
    if (!success)
    {
        Error(QObject::tr("CFlowGraphMigrationHelper::AddEntry: No attribute 'NewClass'").toUtf8().data());
        return;
    }

    QString port;

    // now parse port mappings and input values defaults/overrides
    for (int i = 0; i < node->getChildCount(); i++)
    {
        XmlNodeRef portNode = node->getChild(i);

        if (_stricmp(portNode->getTag(), "Inputs") == 0)
        {
            // collect new inputs
            for (int j = 0; j < portNode->getNumAttributes(); j++)
            {
                const char* key;
                const char* value;
                bool success = portNode->getAttributeByIndex(j, &key, &value);
                if (success)
                {
                    assert (key != 0);
                    assert (value != 0);
                    entry.inputValues[key] = value;
                }
            }
        }
        else if (_stricmp(portNode->getTag(), "InputPort") == 0)
        {
            PortEntry portEntry;
            if (portNode->getAttr("OldName", port) == false)
            {
                continue;
            }
            if (portNode->getAttr("NewName", portEntry.newName) == false)
            {
                continue;
            }
            portEntry.bIsOutput = false;
            // when the port should be discarded, don't remap
            if (portEntry.newName == DISCARD)
            {
                portEntry.bRemapValue = false;
            }
            else
            {
                portEntry.bRemapValue = true;
                portNode->getAttr("RemapValue", portEntry.bRemapValue);
            }
            QString transformer;
            if (portNode->getAttr("Transformer", transformer))
            {
                QByteArray func;
                QString funcName;
                // FIXME: messing up global lua namespace with __fg_tx_%d functions
                funcName = QStringLiteral("__fg_tx_%1").arg(m_transformFuncs.size());
                func += QStringLiteral("function %1 (val) %2 end").arg(funcName, transformer).toUtf8();
                IScriptSystem* pScriptSystem = gEnv->pScriptSystem;
                if (pScriptSystem->ExecuteBuffer(func.data(), func.length(), "FlowGraph MG-Helper LUA Transformer") == true)
                {
                    HSCRIPTFUNCTION luaFunc = pScriptSystem->GetFunctionPtr(funcName.toUtf8().data()); // unref is Done in ReleaseLUAFuncs()
                    if (luaFunc)
                    {
                        m_transformFuncs.push_back(luaFunc);
                        portEntry.transformFunc = luaFunc;
                    }
                    else
                    {
                        Error("CFlowGraphMigrationHelper::AddEntry: LUA-Transformer for Class [%s->%s] Port [%s->%s] compiled but function is NIL!",
                            oldClass.toUtf8().data(), entry.newName.toUtf8().data(), port.toUtf8().data(), portEntry.newName.toUtf8().data());
                    }
                }
                else
                {
                    Error("CFlowGraphMigrationHelper::AddEntry: LUA-Transformer for Class [%s->%s] Port [%s->%s] cannot be compiled.",
                        oldClass.toUtf8().data(), entry.newName.toUtf8().data(), port.toUtf8().data(), portEntry.newName.toUtf8().data());
                }
            }

            entry.inputPortMappings.insert (PortMappings::value_type(port, portEntry));
        }
        else if (_stricmp(portNode->getTag(), "OutputPort") == 0)
        {
            PortEntry portEntry;
            if (portNode->getAttr("OldName", port) == false)
            {
                continue;
            }
            if (portNode->getAttr("NewName", portEntry.newName) == false)
            {
                continue;
            }
            portEntry.bRemapValue = false;
            portEntry.bIsOutput = true;
            entry.outputPortMappings.insert (PortMappings::value_type(port, portEntry));
        }
        else if (_stricmp(portNode->getTag(), "InputValueCond") == 0)
        {
            QString val;
            if (portNode->getAttr("Port", port) == false)
            {
                continue;
            }
            if (portNode->getAttr("Value", val) == false)
            {
                continue;
            }
            entry.conditions.push_back(new CInputValueCondition(port, val));
        }
        else
        {
            Error("CFlowGraphMigrationHelper::AddEntry: [OldClass=%s,NewClass=%s] Unknown tag '%s'",
                oldClass.toUtf8().data(),
                entry.newName.toUtf8().data(),
                portNode->getTag());
        }
    }
    m_nodeMappings.insert (NodeMappings::value_type(oldClass, entry));
}

bool
CFlowGraphMigrationHelper::LoadSubstitutions()
{
    ReleaseLUAFuncs();
    m_nodeMappings.clear();

    QString filename = DEPRECATIONS_FILE_PATH;

    XmlNodeRef root = XmlHelpers::LoadXmlFromFile(filename.toUtf8().data());
    if (!root)
    {
        return false;
    }

    if (_stricmp(root->getTag(), "Substitutions") != 0)
    {
        return false;
    }

    for (int i = 0; i < root->getChildCount(); i++)
    {
        XmlNodeRef itemNode = root->getChild(i);
        // Only accept nodes with correct name.
        if (_stricmp(itemNode->getTag(), "Node") != 0)
        {
            continue;
        }
        AddEntry(itemNode);
    }
    return true;
}

const std::vector<CFlowGraphMigrationHelper::ReportEntry>& CFlowGraphMigrationHelper::GetReport() const
{
    return m_report;
}
