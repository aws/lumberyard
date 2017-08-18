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

#pragma once

#include <QString>
#include "ToolFactory.h"
#include "QPropertyTree/QPropertyTree.h"
#include <QWidget>
#include <QBoxLayout>

class BaseTool;
class QComboBox;
class CBaseObject;
class CMaterial;
class QFrame;
class BaseTool;

namespace CD
{
    class QPropertyTreeWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        template<class ChangeCallback, class SerializationClass>
        void CreatePropertyTree(SerializationClass* serialization, ChangeCallback changeCallback)
        {
            CreatePropertyTree(serialization);
            QObject::connect(m_pPropertyTree, &QPropertyTree::signalChanged, this, [ = ] {changeCallback(false);
                });
            QObject::connect(m_pPropertyTree, &QPropertyTree::signalContinuousChange, this, [ = ] {changeCallback(true);
                });
        }

        template<class SerializationClass>
        void CreatePropertyTree(SerializationClass* serialization)
        {
            QBoxLayout* pBoxLayout = new QBoxLayout(QBoxLayout::TopToBottom);
            pBoxLayout->setContentsMargins(0, 0, 0, 0);
            m_pPropertyTree = new QPropertyTree(this);
            m_pPropertyTree->setExpandLevels(1);
            m_pPropertyTree->attach(Serialization::SStruct(*serialization));
            m_pPropertyTree->setCompact(false);
            pBoxLayout->addWidget(m_pPropertyTree);
            setLayout(pBoxLayout);
        }

        virtual void Serialize(Serialization::IArchive& ar) {}

        QSize contentSize() const { return m_pPropertyTree->contentSize(); }

    protected:
        QPropertyTree* m_pPropertyTree;
    };

    QString GetDefaultButtonStyle();
    QString GetSelectButtonStyle(QWidget* pWidget);

    void FillComboBoxWithSubMaterial(QComboBox* pComboBox, CBaseObject* pObject);

    int  GetSubMatID(QComboBox* pComboBox);
    void SetSubMatID(CBaseObject* pObj, QComboBox* pComboBox, int nID);
    bool SelectMaterial(CBaseObject* pObj, QComboBox* pComboBox, CMaterial* pMaterial);

    void SetAttributeWidget(BaseTool* pTool, QWidget* pWidget, int nAttributeHeight = -1);
    void RemoveAttributeWidget(BaseTool* pTool);

    QFrame* CreateHorizontalLine();
    QWidget* GetMainPanelPtr();

    static const int MaximumAttributePanelHeight = 350;
}