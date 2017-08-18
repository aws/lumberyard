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

#ifndef CRYINCLUDE_EDITOR_VEHICLES_VEHICLEXMLHELPER_H
#define CRYINCLUDE_EDITOR_VEHICLES_VEHICLEXMLHELPER_H
#pragma once


namespace VehicleXml
{
    bool IsUseNode(const XmlNodeRef& node);
    bool IsArrayNode(const XmlNodeRef& node);
    bool IsArrayElementNode(const XmlNodeRef& node, const char* name);
    bool IsArrayParentNode(const XmlNodeRef& node, const char* name);
    bool IsPropertyNode(const XmlNodeRef& node);
    bool IsOptionalNode(const XmlNodeRef& node);
    bool IsDeprecatedNode(const XmlNodeRef& node);

    bool HasElementNameEqualTo(const XmlNodeRef& node, const char* name);
    bool HasNodeIdEqualTo(const XmlNodeRef& node, const char* id);
    bool HasNodeNameEqualTo(const XmlNodeRef& node, const char* name);

    const char* GetNodeId(const XmlNodeRef& node);
    const char* GetNodeElementName(const XmlNodeRef& node);
    const char* GetNodeName(const XmlNodeRef& node);

    XmlNodeRef GetXmlNodeDefinitionById(const XmlNodeRef& definitionRoot, const char* id);
    XmlNodeRef GetXmlNodeDefinitionByVariable(const XmlNodeRef& definitionRoot, IVariable* pRootVar, IVariable* pVar);

    bool GetVariableHierarchy(IVariable* pRootVar, IVariable* pVar, std::vector< IVariable* >& varHierarchyOut);

    void GetXmlNodeFromVariable(IVariable* pVar, XmlNodeRef& xmlNode);
    void GetXmlNodeChildDefinitions(const XmlNodeRef& definitionRoot, const XmlNodeRef& definition, std::vector< XmlNodeRef >& childDefinitionsOut);
    void GetXmlNodeChildDefinitionsByVariable(const XmlNodeRef& definitionRoot, IVariable* pRootVar, IVariable* pVar, std::vector< XmlNodeRef >& childDefinitionsOut);
    XmlNodeRef GetXmlNodeChildDefinitionsByName(const XmlNodeRef& definitionRoot, const char* childName, IVariable* pRootVar, IVariable* pVar);


    IVariable* CreateDefaultVar(const XmlNodeRef& definitionRoot, const XmlNodeRef& definition, const char* varName);

    void SetExtendedVarProperties(IVariable* pVar, const XmlNodeRef& node);

    template< class T >
    IVariable* CreateVar(T value, const XmlNodeRef& definition)
    {
        return new CVariable< T >();
    }

    IVariable* CreateVar(const char* value, const XmlNodeRef& definition);

    IVariable* CreateSimpleVar(const char* attributeName, const char* attributeValue, const XmlNodeRef& definition);


    void CleanUp(IVariable* pVar);
}


/*
=========================================================
    DefinitionTable
=========================================================
*/

struct DefinitionTable
{
    // Build a list of definitions against their name for easy searching
    typedef std::map<string, XmlNodeRef> TXmlNodeMap;

    static TXmlNodeMap g_useDefinitionMap;

    TXmlNodeMap m_definitionList;

    static void ReloadUseReferenceTables(XmlNodeRef definition);
    void Create(XmlNodeRef definitionNode);
    XmlNodeRef GetDefinition(const char* definitionName);
    XmlNodeRef GetPropertyDefinition(const char* attributeName);
    bool IsArray(XmlNodeRef def);
    bool IsTable(XmlNodeRef def);
    void Dump();

private:
    static void GetUseReferenceTables(XmlNodeRef definition);
};


#endif // CRYINCLUDE_EDITOR_VEHICLES_VEHICLEXMLHELPER_H
