/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/
#pragma once

#include <QWidget>

class QMenu;
class QDomDocument;

struct SAttributePreset
{
    QString name;
    QString doc;
};

class QAttributePresetWidget
    : public QWidget
{
    Q_OBJECT
public:
    QAttributePresetWidget(QWidget* parent);
    ~QAttributePresetWidget();

    void BuilPresetMenu(QMenu*);
    void StoreSessionPresets();
    bool LoadSessionPresets();

    void AddPreset(QString name, QString data);

    void BuildDefaultLibrary();

signals:
    void SignalCustomPanel(QString);

private:

    QVector<SAttributePreset*> m_presetList;
};
