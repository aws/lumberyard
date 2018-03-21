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

#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QObject>
#include <QHash>
#include <QString>
#include <QScopedPointer>
#include <QSet>
#include <QStack>

class QApplication;
class QProxyStyle;
class QFileSystemWatcher;
class QRegExp;

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API StyleSheetCache
        : public QObject
    {
        Q_OBJECT

    public:
        explicit StyleSheetCache(QApplication* application, QObject* parent);
        ~StyleSheetCache();

        bool isUI10() const;
        void setIsUI10(bool isUI10);

        QString loadStyleSheet(QString styleFileName);

    public Q_SLOTS:
        void reloadStyleSheets();
        void fileOnDiskChanged(const QString& filePath);

    private:
        QString preprocess(QString styleFileName, QString loadedStyleSheet);
        QString findStyleSheetPath(QString styleFileName);
        void applyGlobalStyleSheet();

        QHash<QString, QString> m_styleSheetCache;
        bool m_isUI10 = false;
        QApplication* m_application;
        QString m_rootDir;

        QSet<QString> m_processingFiles;
        QStack<QString> m_processingStack;

        QFileSystemWatcher* m_fileWatcher;

        QScopedPointer<QRegExp> m_importExpression;
};

} // namespace AzQtComponents

