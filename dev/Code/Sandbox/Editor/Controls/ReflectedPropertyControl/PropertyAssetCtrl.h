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

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include "ReflectedVar.h"
#include <QtWidgets/QWidget>

class QToolButton;
class CAnimationBrowser;
class QLabel;

class AssetPropertyCtrl
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(AssetPropertyCtrl, AZ::SystemAllocator, 0);

    AssetPropertyCtrl(QWidget* parent = nullptr);
    virtual ~AssetPropertyCtrl();

    CReflectedVarAsset value() const;

    QWidget* GetFirstInTabOrder();
    QWidget* GetLastInTabOrder();
    void UpdateTabOrder();
    void UpdateLabel();

signals:
    void ValueChanged(CReflectedVarAsset value);

public slots:
    void SetValue(const CReflectedVarAsset& asset);

protected slots:
    void OnBrowseClicked();
    void OnApplyClicked();

private:
    QToolButton* m_browseButton;
    QToolButton* m_applyButton;
    QLabel* m_assetLabel;

    CReflectedVarAsset m_asset;
};

class AssetPropertyWidgetHandler
    : QObject
    , public AzToolsFramework::PropertyHandler <CReflectedVarAsset, AssetPropertyCtrl>
{
public:
    AZ_CLASS_ALLOCATOR(AssetPropertyWidgetHandler, AZ::SystemAllocator, 0);

    virtual AZ::u32 GetHandlerName(void) const override  { return AZ_CRC("AssetId", 0x2289cf4d); }
    virtual bool IsDefaultHandler() const override { return true; }
    virtual QWidget* GetFirstInTabOrder(AssetPropertyCtrl* widget) override { return widget->GetFirstInTabOrder(); }
    virtual QWidget* GetLastInTabOrder(AssetPropertyCtrl* widget) override { return widget->GetLastInTabOrder(); }
    virtual void UpdateWidgetInternalTabbing(AssetPropertyCtrl* widget) override { widget->UpdateTabOrder(); }

    virtual QWidget* CreateGUI(QWidget* parent) override;
    virtual void ConsumeAttribute(AssetPropertyCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    virtual void WriteGUIValuesIntoProperty(size_t index, AssetPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    virtual bool ReadValuesIntoGUI(size_t index, AssetPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)  override;
};
