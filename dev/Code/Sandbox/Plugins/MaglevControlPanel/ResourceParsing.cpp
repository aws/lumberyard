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
#include <ResourceParsing.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValueRef>
#include <QVector>

namespace ResourceParsing
{

    void EnsureAccessControl(QJsonObject& resourcesObject, const QString& resourceName)
    {
        if (resourcesObject["AccessControl"].isObject())
        {
            QJsonObject accessControlObject = resourcesObject["AccessControl"].toObject();
            QJsonArray dependsOnArray;
            if (accessControlObject.contains("DependsOn") && !accessControlObject["DependsOn"].isArray())
            {
                dependsOnArray.append(accessControlObject["DependsOn"]);
            }
            else
            {
                dependsOnArray = accessControlObject["DependsOn"].toArray();
            }
            if (!dependsOnArray.contains(resourceName))
            {
                dependsOnArray.append(resourceName);
            }
            accessControlObject["DependsOn"] = dependsOnArray;
            resourcesObject["AccessControl"] = accessControlObject;
        }
    }

    QList<QString> ParseResourceList(const QString& contentString)
    {
        QList<QString> returnList;

        QVariantMap resourceMap = ParseResourceMap(contentString);
        auto keyList = resourceMap.keys();
        keyList.sort();
        for (auto thisKey : keyList)
        {
            returnList.push_back(thisKey);
        }
        return returnList;
    }

    QVariantMap ParseResourceSettingsMap(const QString& contentString)
    {
        QJsonDocument jsonDoc(QJsonDocument::fromJson(contentString.toUtf8()));

        if (jsonDoc.isNull())
        {
            return{};
        }

        QJsonObject jsonObj(jsonDoc.object());
        return jsonObj.toVariantMap();
    }

    QVariantMap ParseResourceMap(const QString& contentString)
    {
        QJsonDocument jsonDoc(QJsonDocument::fromJson(contentString.toUtf8()));

        if (jsonDoc.isNull())
        {
            return {};
        }

        QJsonObject jsonObj(jsonDoc.object());
        QJsonValueRef rootElement = jsonObj["Resources"];

        if (!rootElement.isObject())
        {
            return {};
        }

        return rootElement.toObject().toVariantMap();
    }

    QString GetResourceValueContent(const QString& baseContent, const QString& resourceName)
    {
        QVariantMap resourceMap = ParseResourceMap(baseContent);
        QVariant thisElement = resourceMap[resourceName];
        return ParseResourceVariant(thisElement);
    }

    QString ParseResourceVariant(const QVariant& thisResource)
    {
        QJsonValue thisValue = QJsonValue::fromVariant(thisResource);
        QJsonDocument outDoc(thisValue.toObject());
        QString returnString = outDoc.toJson();
        return returnString;
    }

    QVector<QString> ParseStringList(const QVariantMap& variantMap, const QString& sectionName)
    {
        QVariantMap::const_iterator findElement = variantMap.find(sectionName);
        if (findElement == variantMap.cend())
        {
            return{};
        }

        QJsonValue thisValue = QJsonValue::fromVariant(*findElement);
        if (!thisValue.isArray())
        {
            return{};
        }

        QVector<QString> returnVec;
        for (auto thisEntry : thisValue.toArray())
        {
            if (thisEntry.isString())
            {
                returnVec.push_back(thisEntry.toString());
            }
        }
        return returnVec;
    }
    bool HasResource(const QString& contentString, const QString& resourceName)
    {
        QVariantMap resourceMap = ParseResourceMap(contentString);
        return resourceMap.contains(resourceName);
    }

    QString GetResourceContent(const QString& baseContent, const QString& resourceName, bool addRelated)
    {
        QVariantMap resourceMap = ParseResourceMap(baseContent);
        QVariantMap::iterator itemIter = resourceMap.find(resourceName);
        if (itemIter == resourceMap.end())
        {
            return {};
        }

        QVariantMap outputMap;
        outputMap.insert(resourceName, *itemIter);

        // Are we a lambda which needs to tack on another distinct resource object for view/edit
        QVariantMap thisObject = itemIter->toMap();

        if (addRelated)
        {
            AddRelatedResources(thisObject, resourceName, resourceMap, outputMap);
        }

        QJsonDocument outDoc(QJsonDocument::fromVariant(outputMap));
        QString returnString = outDoc.toJson();
        return returnString;
    }

    void AddRelatedResources(const QVariantMap& primaryResource, const QString& resourceName, const QVariantMap& resourceMap, QVariantMap& outputMap)
    {
        if (!primaryResource.isEmpty())
        {
            QVariant typeStr = primaryResource.value("Type");

            if (typeStr.toString() == "AWS::Lambda::Function")
            {
                QString configName {
                    resourceName + "Configuration"
                };
                QVariantMap::const_iterator resourceIter = resourceMap.find(configName);
                if (resourceIter != resourceMap.end())
                {
                    outputMap.insert(configName, *resourceIter);
                }
            }
            else if (typeStr.toString() == "AWS::GameLift::Fleet")
            {
                QString buildName {
                    resourceName + "Build"
                };
                QVariantMap::const_iterator resourceIter = resourceMap.find(buildName);
                if (resourceIter != resourceMap.end())
                {
                    outputMap.insert(buildName, *resourceIter);
                }

                QString aliasName {
                    resourceName + "Alias"
                };
                resourceIter = resourceMap.find(aliasName);
                if (resourceIter != resourceMap.end())
                {
                    outputMap.insert(aliasName, *resourceIter);
                }
            }
        }
    }

    QString SetResourceContent(const QString& baseContent, const QString& resourceName, const QString& contentData)
    {
        QJsonDocument jsonDoc(QJsonDocument::fromJson(baseContent.toUtf8()));

        if (jsonDoc.isNull())
        {
            return baseContent;
        }

        QJsonObject jsonObj(jsonDoc.object());
        QJsonValueRef rootElement = jsonObj["Resources"];

        if (!rootElement.isObject())
        {
            return baseContent;
        }

        QJsonObject resourcesObject = rootElement.toObject();
        resourcesObject.remove(resourceName);

        QJsonDocument inputDoc(QJsonDocument::fromJson(contentData.toUtf8()));
        resourcesObject[resourceName] = inputDoc.object();

        jsonObj["Resources"] = resourcesObject;
        QJsonDocument outDocument;
        outDocument.setObject(jsonObj);
        return outDocument.toJson();
    }

    QString ReadResources(const QString& baseContent, const QString& contentData)
    {
        QJsonDocument jsonDoc(QJsonDocument::fromJson(baseContent.toUtf8()));

        if (jsonDoc.isNull())
        {
            return baseContent;
        }

        QJsonObject jsonObj(jsonDoc.object());
        QJsonValueRef rootElement = jsonObj["Resources"];

        if (!rootElement.isObject())
        {
            return baseContent;
        }

        QJsonObject resourcesObject = rootElement.toObject();

        QJsonDocument inputDoc(QJsonDocument::fromJson(contentData.toUtf8()));
        QJsonObject inputObj = inputDoc.object();

        QVariantMap resourceMap = inputObj.toVariantMap();
        for (QVariantMap::iterator itemIter = resourceMap.begin(); itemIter != resourceMap.end(); ++itemIter)
        {
            QJsonObject writeObject {
                QJsonObject::fromVariantMap(itemIter->toMap())
            };
            resourcesObject[itemIter.key()] = writeObject;
        }

        jsonObj["Resources"] = resourcesObject;
        QJsonDocument outDocument;
        outDocument.setObject(jsonObj);
        return outDocument.toJson();
    }

    void RemoveResourceFromAccessControl(QJsonObject& resourcesObject, const QString& resourceName)
    {
        if (resourcesObject["AccessControl"].isObject())
        {
            QJsonObject accessControlObject = resourcesObject["AccessControl"].toObject();
            if (accessControlObject["DependsOn"].isArray())
            {
                QJsonArray dependsOnList = accessControlObject["DependsOn"].toArray();

                int index;
                for (index = 0; index < dependsOnList.size(); index++)
                {
                    if (dependsOnList[index] == QJsonValue(resourceName))
                    {
                        break;
                    }
                }

                if (index < dependsOnList.size())
                {
                    dependsOnList.removeAt(index);
                }
                accessControlObject["DependsOn"] = dependsOnList;
                resourcesObject["AccessControl"] = accessControlObject;
            }
        }
    }

    void RemoveReadAndWriteCapacityUnits(QJsonObject& jsonObj, const QString& resourceName)
    {
        QJsonValueRef parametersElement = jsonObj["Parameters"];
        if (parametersElement.isObject())
        {
            QJsonObject parametersObject = parametersElement.toObject();

            QJsonObject::iterator readCapacityUnitsIter = parametersObject.find(resourceName + "ReadCapacityUnits");
            if (readCapacityUnitsIter != parametersObject.end())
            {
                parametersObject.erase(readCapacityUnitsIter);
            }

            QJsonObject::iterator writeCapacityUnitsIter = parametersObject.find(resourceName + "WriteCapacityUnits");
            if (writeCapacityUnitsIter != parametersObject.end())
            {
                parametersObject.erase(writeCapacityUnitsIter);
            }
            jsonObj["Parameters"] = parametersObject;
        }
    }

    void RemoveResourceDependency(QJsonObject& resourcesObject, const QString& resourceName)
    {
        for (auto iter = resourcesObject.begin(); iter != resourcesObject.end(); ++iter)
        {
            QJsonValueRef resourceObjectRef = *iter;
            if (resourceObjectRef.isObject())
            {
                QJsonObject resourceObject = resourceObjectRef.toObject();
                if (resourceObject["Type"] == QJsonValue("Custom::LambdaConfiguration") && resourceObject.find("Properties") != resourceObject.end())
                {
                    QJsonObject propertiesObject = resourceObject["Properties"].toObject();
                    if (propertiesObject.find("Settings") != propertiesObject.end())
                    {
                        QJsonObject settingsIterObject = propertiesObject["Settings"].toObject();
                        QJsonObject::iterator dependencyIter = settingsIterObject.find(resourceName);
                        if (dependencyIter != settingsIterObject.end())
                        {
                            settingsIterObject.erase(dependencyIter);
                            propertiesObject["Settings"] = settingsIterObject;
                            resourceObject["Properties"] = propertiesObject;
                            resourceObjectRef = resourceObject;
                        }
                    } 
                }
            }
        }
    }

    QString DeleteResource(const QString& baseContent, const QString& resourceName)
    {
        QJsonDocument jsonDoc(QJsonDocument::fromJson(baseContent.toUtf8()));

        if (jsonDoc.isNull())
        {
            return baseContent;
        }

        QJsonObject jsonObj(jsonDoc.object());
        QJsonValueRef rootElement = jsonObj["Resources"];

        if (!rootElement.isObject())
        {
            return baseContent;
        }

        QJsonObject resourcesObject = rootElement.toObject();
        QJsonObject::iterator thisIter = resourcesObject.find(resourceName);

        if (thisIter == resourcesObject.end())
        {
            return baseContent;
        }

        if (resourcesObject[resourceName].isObject())
        {
            QJsonObject resourceObject = resourcesObject[resourceName].toObject();
            if (resourceObject["Type"] == QJsonValue("AWS::DynamoDB::Table"))
            {
                RemoveReadAndWriteCapacityUnits(jsonObj, resourceName);
            }
        }

        resourcesObject.erase(thisIter);

        RemoveResourceDependency(resourcesObject, resourceName);

        RemoveResourceFromAccessControl(resourcesObject, resourceName);

        jsonObj["Resources"] = resourcesObject;
        QJsonDocument outDocument;
        outDocument.setObject(jsonObj);
        return outDocument.toJson();
    }

    QString AddResource(const QString& baseContent, const QString& resourceName, const QVariantMap& variantMap)
    {
        if (!resourceName.length())
        {
            return baseContent;
        }

        QJsonDocument jsonDoc(QJsonDocument::fromJson(baseContent.toUtf8()));

        if (jsonDoc.isNull())
        {
            return baseContent;
        }

        QJsonObject jsonObj(jsonDoc.object());
        QJsonValueRef rootElement = jsonObj["Resources"];

        if (!rootElement.isObject())
        {
            return baseContent;
        }

        QJsonObject resourcesObject = rootElement.toObject();
        QJsonObject::iterator thisIter = resourcesObject.find(resourceName);

        if (thisIter != resourcesObject.end())
        {
            return baseContent;
        }

        resourcesObject[resourceName] = QJsonObject::fromVariantMap(variantMap);

        EnsureAccessControl(resourcesObject, resourceName);

        jsonObj["Resources"] = resourcesObject;

        QJsonDocument outDocument;
        outDocument.setObject(jsonObj);
        return outDocument.toJson();
    }

    QString AddParameters(const QString& baseContent, const QVariantMap& variantMap)
    {
        if (!variantMap.size())
        {
            return baseContent;
        }

        QJsonDocument jsonDoc(QJsonDocument::fromJson(baseContent.toUtf8()));

        if (jsonDoc.isNull())
        {
            return baseContent;
        }

        QJsonObject jsonObj(jsonDoc.object());
        QJsonValueRef rootElement = jsonObj["Parameters"];

        QJsonObject parametersObject;
        if (rootElement.isObject())
        {
            parametersObject = rootElement.toObject();
        }

        for (auto thisElement = variantMap.cbegin(); thisElement != variantMap.cend(); ++thisElement)
        {
            parametersObject[thisElement.key()] = thisElement->toJsonObject();
        }

        jsonObj["Parameters"] = parametersObject;

        QJsonDocument outDocument;
        outDocument.setObject(jsonObj);
        return outDocument.toJson();
    }

    QString GetResourceTypeFromFirstObject(const QString& baseContent)
    {
        QJsonDocument jsonDoc(QJsonDocument::fromJson(baseContent.toUtf8()));

        QJsonObject docObject = jsonDoc.object();

        QJsonObject::iterator firstIter = docObject.begin();

        if (firstIter == docObject.end())
        {
            return {};
        }

        QJsonValueRef firstRef = firstIter.value();

        if (firstRef.isObject())
        {
            QVariantMap thisMap = firstRef.toObject().toVariantMap();
            QVariant thisValue = thisMap["Type"];

            QString outString = thisValue.toString();
            return outString;
        }
        return {};
    }

    QString GetActionFromType(const QString& resourceType)
    {
        QString returnString = resourceType.section("::", 1, 1);
        if (returnString.length())
        {
            returnString = returnString.toLower();
            returnString += ":*";
            return returnString;
        }
        return "";
    }

    QString AddLambdaAccess(const QString& baseContent, const QString& resourceName, const QString& functionName)
    {
        if (!resourceName.length())
        {
            return baseContent;
        }

        QJsonDocument jsonDoc(QJsonDocument::fromJson(baseContent.toUtf8()));

        if (jsonDoc.isNull())
        {
            return baseContent;
        }

        QJsonObject jsonObj(jsonDoc.object());
        QJsonValueRef rootElement = jsonObj["Resources"];

        if (!rootElement.isObject())
        {
            return baseContent;
        }

        QJsonObject resourcesObject = rootElement.toObject();
        QJsonObject::iterator thisIter = resourcesObject.find(resourceName);

        if (thisIter == resourcesObject.end() || !thisIter.value().isObject())
        {
            return baseContent;
        }

        QJsonObject thisResource = thisIter.value().toObject();

        QJsonObject metaObject = thisResource["Metadata"].toObject();

        QJsonObject cloudCanvasObject = metaObject["CloudCanvas"].toObject();

        QJsonArray permissionsArray;
        if (cloudCanvasObject.contains("Permissions") && !cloudCanvasObject["Permissions"].isArray())
        {
            permissionsArray.append(cloudCanvasObject["Permissions"]);
        }
        else
        {
            permissionsArray = cloudCanvasObject["Permissions"].toArray();
        }

        QJsonObject permissionObject;
        permissionObject["AbstractRole"] = functionName;
        permissionObject["Action"] = GetActionFromType(thisResource["Type"].toString());

        permissionsArray.append(permissionObject);

        cloudCanvasObject["Permissions"] = permissionsArray;

        metaObject["CloudCanvas"] = cloudCanvasObject;
        thisResource["Metadata"] = metaObject;

        resourcesObject[resourceName] = thisResource;

        EnsureAccessControl(resourcesObject, resourceName);

        jsonObj["Resources"] = resourcesObject;

        QJsonDocument outDocument;
        outDocument.setObject(jsonObj);
        return outDocument.toJson();
    }

}