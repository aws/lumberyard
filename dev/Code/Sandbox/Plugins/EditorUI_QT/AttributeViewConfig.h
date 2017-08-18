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

#ifndef CRYINCLUDE_EDITORUI_QT_ATTRIBUTEVIEWCONFIG_H
#define CRYINCLUDE_EDITORUI_QT_ATTRIBUTEVIEWCONFIG_H
#pragma once

#include <QString>
#include <qvector.h>
#include <QtXml/QDomDocument>
#include <QMap>
#include "api.h"

class QDomDocument;

class EDITOR_QT_UI_API CAttributeViewConfig
{
public:
    struct config
    {
        struct relation
        {
            QString scr, dst, slot;
        };

        struct item
        {
            QString name;
            QString as;
            QString advanced;
            QString visibility;
            QString onUpdateCallback;

            void* tag;

            std::vector<relation> relations;
            QMap<QString, item> items; //subitems
        };

        struct group
        {
            enum class order_type
            {
                Group,
                Item
            };
            struct order_key
            {
                order_type type;
                short index;
            };
            QString name;
            QString visibility;
            std::vector<group> groups;
            std::vector<item> items;
            std::vector<order_key> order;
            //For CustomPanel
            bool isCustom;
            QString isGroupAttribute;
        };

        struct panel
        {
            group innerGroup;
        };

        struct ignored
        {
            QString path;
        };

        struct root
        {
            // Use QVector to take care of cross plateform issues.
            QVector<panel> panels;
            QVector<ignored> ignored;
            QVector<QString> visibilityVars;
            QMap<QString, QString> variableAlias;
        };
    };
    CAttributeViewConfig();
    CAttributeViewConfig(const QString& filename);
    CAttributeViewConfig(const QDomDocument& doc);
    QString GetDefaultVisibilityValue();

    QString GetVariableAlias(const QString& varName) const;


    const config::root& GetRoot() const { return m_root; }
protected:
    config::root m_root;

private:
    void LoadConfig(const QDomDocument& doc);
    void parseItemSubNodes(CAttributeViewConfig::config::item& vstruct_item, QDomNode ndItem);
    void parseNodeGroup(CAttributeViewConfig::config::group& vstruct_group, QDomNode ndPanel);
    CAttributeViewConfig::config::item parseItemNode(QDomNode item);
};

#endif // CRYINCLUDE_EDITORUI_QT_ATTRIBUTEVIEW_H
