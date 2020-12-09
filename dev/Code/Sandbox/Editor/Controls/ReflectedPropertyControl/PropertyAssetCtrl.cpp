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
#include "StdAfx.h"

#include "PropertyAssetCtrl.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include "CharacterEditor/AnimationBrowser.h"
#include "Controls/MultiMonHelper.h"
#include "Util/UIEnumerations.h"
#include "IResourceSelectorHost.h"
#include "MathConversion.h"


AssetPropertyCtrl::AssetPropertyCtrl(QWidget* parent /*= nullptr*/) : QWidget(parent)
{
    m_assetLabel = new QLabel;

    m_browseButton = new QToolButton;
    m_browseButton->setIcon(QIcon(":/reflectedPropertyCtrl/img/file_browse.png"));
    m_applyButton = new QToolButton;
    m_applyButton->setIcon(QIcon(":/reflectedPropertyCtrl/img/apply.png"));

    m_applyButton->setFocusPolicy(Qt::StrongFocus);
    m_browseButton->setFocusPolicy(Qt::StrongFocus);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_assetLabel, 1);
    layout->addWidget(m_browseButton);
    layout->addWidget(m_applyButton);

    connect(m_browseButton, &QAbstractButton::clicked, this, &AssetPropertyCtrl::OnBrowseClicked);
    connect(m_applyButton, &QAbstractButton::clicked, this, &AssetPropertyCtrl::OnApplyClicked);
};

AssetPropertyCtrl::~AssetPropertyCtrl()
{
}


void AssetPropertyCtrl::SetValue(const CReflectedVarAsset &asset)
{
    m_asset = asset;
    UpdateLabel();
}

CReflectedVarAsset AssetPropertyCtrl::value() const
{
    return m_asset;
}

void AssetPropertyCtrl::OnBrowseClicked()
{
    // Request the AssetBrowser Dialog and set a type filter
    AZStd::string assetType = m_asset.m_assetTypeName;
    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(assetType.c_str());
    selection.SetSelectedAssetId(m_asset.m_assetId);
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (selection.IsValid())
    {
        auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
        if (product != nullptr)
        {
            m_asset.m_assetId = product->GetAssetId();
            UpdateLabel();
            emit ValueChanged(m_asset);
        }
    }
}

void AssetPropertyCtrl::OnApplyClicked()
{
}

QWidget* AssetPropertyCtrl::GetFirstInTabOrder()
{
    return m_browseButton;
}
QWidget* AssetPropertyCtrl::GetLastInTabOrder()
{
    return m_applyButton;
}

void AssetPropertyCtrl::UpdateTabOrder()
{
    setTabOrder(m_browseButton, m_applyButton);
}

void AssetPropertyCtrl::UpdateLabel()
{
    AZStd::string assetPath;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetPath, &AZ::Data::AssetCatalogRequestBus::Events::GetAssetPathById, m_asset.m_assetId);
    AZStd::string assetName = assetPath;

    if (!assetPath.empty())
    {
        AzFramework::StringFunc::Path::GetFileName(assetPath.c_str(), assetName);
    }

    m_assetLabel->setText(assetName.c_str());
}

QWidget* AssetPropertyWidgetHandler::CreateGUI(QWidget* parent)
{
    AssetPropertyCtrl* newCtrl = aznew AssetPropertyCtrl(parent);
    connect(newCtrl, &AssetPropertyCtrl::ValueChanged, newCtrl, [newCtrl]()
    {
        AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
    });
    return newCtrl;
}


void AssetPropertyWidgetHandler::ConsumeAttribute(AssetPropertyCtrl* /*GUI*/, AZ::u32 /*attrib*/, AzToolsFramework::PropertyAttributeReader* /*attrValue*/, const char* /*debugName*/)
{
}

void AssetPropertyWidgetHandler::WriteGUIValuesIntoProperty(size_t /*index*/, AssetPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
{
    CReflectedVarAsset asset = GUI->value();
    instance = static_cast<property_t>(asset);
}

bool AssetPropertyWidgetHandler::ReadValuesIntoGUI(size_t /*index*/, AssetPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* /*node*/)
{
    CReflectedVarAsset asset = instance;
    GUI->SetValue(asset);
    return false;
}

#include <Controls/ReflectedPropertyControl/PropertyAssetCtrl.moc>
