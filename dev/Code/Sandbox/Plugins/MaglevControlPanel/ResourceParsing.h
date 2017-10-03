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

#include <QList>
#include <QString>
#include <QVariantMap>

namespace ResourceParsing
{
    QList<QString> ParseResourceList(const QString& contentString);
    QVariantMap ParseResourceMap(const QString& contentString);
    QVariantMap ParseResourceSettingsMap(const QString& contentString);
    QString GetResourceValueContent(const QString& baseContent, const QString& resourceName);
    QString ParseResourceVariant(const QVariant& thisResource);
    QString GetResourceContent(const QString& baseContent, const QString& resourceName, bool addRelated);
    void AddRelatedResources(const QVariantMap& primaryResource, const QString& resourceName, const QVariantMap& resourceMap, QVariantMap& outputMap);
    void RemoveResourceDependency(QJsonObject& resourcesObject, const QString& resourceName);
    QString SetResourceContent(const QString& baseContent, const QString& resourceName, const QString& contentData);
    QString ReadResources(const QString& baseContent, const QString& contentData);
    QString DeleteResource(const QString& baseContent, const QString& resourceName);
    QString AddResource(const QString& baseContent, const QString& resourceName, const QVariantMap& variantMap);
    QString AddParameters(const QString& baseContent, const QVariantMap& variantMap);
    QString GetActionFromType(const QString& resourceType);
    QString AddLambdaAccess(const QString& baseContent, const QString& resourceName, const QString& functionName);
    bool HasResource(const QString& contentString, const QString& resourceName);
    QVector<QString> ParseStringList(const QVariantMap& variantMap, const QString& sectionName);

    // Detail widget helpers
    QString GetResourceTypeFromFirstObject(const QString& baseContent);
}