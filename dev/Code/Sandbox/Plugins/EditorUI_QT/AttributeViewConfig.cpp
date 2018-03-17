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

#include "stdafx.h"
#include "AttributeViewConfig.h"

#include <QFile>
#include <vector>
#include <QDebug>

CAttributeViewConfig::config::item CAttributeViewConfig::parseItemNode(QDomNode item)
{
    auto ndItem = item.toElement();
    CAttributeViewConfig::config::item vstruct_item;

    // item attributes
    vstruct_item.name = ndItem.attribute("name");
    vstruct_item.as = ndItem.attribute("as");
    vstruct_item.advanced = ndItem.attribute("advanced", "no");
    vstruct_item.visibility = ndItem.attribute("visibility", GetDefaultVisibilityValue());
    vstruct_item.onUpdateCallback = ndItem.attribute("onUpdateCallback", "");

    parseItemSubNodes(vstruct_item, ndItem);

    return vstruct_item;
}

void CAttributeViewConfig::parseItemSubNodes(CAttributeViewConfig::config::item& vstruct_item, QDomNode ndItem)
{
    auto children = ndItem.childNodes();
    for (int i = 0; i < children.count(); i++)
    {
        auto child = children.at(i);
        if (child.isElement() == false)
        {
            continue;
        }
        if (child.nodeName() == "relation")
        {
            auto ndRelation = child.toElement();
            CAttributeViewConfig::config::relation vstruct_relation;
            vstruct_relation.scr = ndRelation.attribute("scr");
            vstruct_relation.dst = ndRelation.attribute("dst");
            vstruct_relation.slot = ndRelation.attribute("slot");
            vstruct_item.relations.push_back(vstruct_relation);
        }
        else if (child.nodeName() == "item")
        {
            CAttributeViewConfig::config::item subItem =  parseItemNode(child);
            subItem.as = subItem.as;
            vstruct_item.items.insert(subItem.as, subItem);
        }
    }
}

void CAttributeViewConfig::parseNodeGroup(CAttributeViewConfig::config::group& vstruct_group, QDomNode ndPanel)
{
    vstruct_group.name = ndPanel.toElement().attribute("name");
    vstruct_group.isCustom = (ndPanel.toElement().attribute("custom") == "yes" |
                              ndPanel.toElement().attribute("custom") == "1" |
                              ndPanel.toElement().attribute("custom") == "true");
    vstruct_group.visibility = ndPanel.toElement().attribute("visibility", GetDefaultVisibilityValue());
    // group, both, emitter
    vstruct_group.isGroupAttribute = ndPanel.toElement().attribute("GroupVisibility");


    auto children = ndPanel.childNodes();
    for (int i = 0; i < children.count(); i++)
    {
        auto child = children.at(i);
        if (child.isElement() == false)
        {
            continue;
        }
        if (child.nodeName() == "item")
        {
            CAttributeViewConfig::config::item vstruct_item = parseItemNode(child);

            // add item
            vstruct_group.order.push_back({ CAttributeViewConfig::config::group::order_type::Item, static_cast<short>(vstruct_group.items.size()) });
            vstruct_group.items.push_back(vstruct_item);
        }
        else if (child.nodeName() == "group")
        {
            // parse subgroup
            CAttributeViewConfig::config::group subGroup;
            parseNodeGroup(subGroup, child);
            vstruct_group.order.push_back({CAttributeViewConfig::config::group::order_type::Group, static_cast<short>(vstruct_group.groups.size()) });
            vstruct_group.groups.push_back(subGroup);
        }
    }
}

CAttributeViewConfig::CAttributeViewConfig(const QString& filename)
{
    QMessageLogger log;
    QFile file(filename);
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorColumn;
    doc.setContent(&file, &errorMsg, &errorLine, &errorColumn);
    if (errorMsg.length())
    {
        log.debug("XML Error: %s", errorMsg.toUtf8().data());
    }

    LoadConfig(doc);
}

CAttributeViewConfig::CAttributeViewConfig(const QDomDocument& doc)
{
    LoadConfig(doc);
}

void CAttributeViewConfig::LoadConfig(const QDomDocument& doc)
{
    config::root& vstruct_root = m_root;

    // enumerate ignored
    QDomNodeList lstIgnored = doc.elementsByTagName("ignored");
    for (int i = 0; i < lstIgnored.count(); i++)
    {
        config::ignored vstruct_ignored;
        vstruct_ignored.path = lstIgnored.at(i).toElement().attribute("path");

        vstruct_root.ignored.push_back(vstruct_ignored);
    }
    // enumerate visibility var
    QDomNodeList lstVisibilityKey = doc.elementsByTagName("visibilityVar");
    for (int i = 0; i < lstVisibilityKey.count(); i++)
    {
        QString varName;
        varName = lstVisibilityKey.at(i).toElement().attribute("path");

        vstruct_root.visibilityVars.push_back(varName);
    }

    //alias for none item variables, such as child variables: Emitter Strength
    QDomNodeList lstAliasKey = doc.elementsByTagName("variableAlias");
    for (int i = 0; i < lstAliasKey.count(); i++)
    {
        QString varName;
        varName = lstAliasKey.at(i).toElement().attribute("name");
        QString alias;
        alias = lstAliasKey.at(i).toElement().attribute("alias");

        vstruct_root.variableAlias[varName] = alias;
    }

    // enumerate panels
    // Panels should be enumerated after visibilityvars, since it would need info of
    // visibilityvars to contribute item visibility.
    QDomNodeList lstPanel = doc.elementsByTagName("panel");
    for (int idxPanel = 0; idxPanel < lstPanel.count(); idxPanel++)
    {
        config::panel vstruct_panel;
        auto ndPanel = lstPanel.at(idxPanel);

        // enumerate inner group
        parseNodeGroup(vstruct_panel.innerGroup, ndPanel);

        vstruct_root.panels.push_back(vstruct_panel);
    }
}

CAttributeViewConfig::CAttributeViewConfig()
{
}

QString CAttributeViewConfig::GetVariableAlias(const QString& varName) const
{
    //get the variable name
    if (m_root.variableAlias.find(varName) != m_root.variableAlias.end())
    {
        return m_root.variableAlias[varName];
    }

    return "";
}

QString CAttributeViewConfig::GetDefaultVisibilityValue()
{
    QString returnValue = "";
    for (QString varName : m_root.visibilityVars)
    {
        returnValue += varName + ":all;";
    }
    return returnValue;
}
