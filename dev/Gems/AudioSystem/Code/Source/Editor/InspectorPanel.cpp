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

#include <InspectorPanel.h>

#include <ACETypes.h>
#include <AudioControlsEditorPlugin.h>
#include <IAudioSystemControl.h>
#include <IEditor.h>
#include <QAudioControlEditorIcons.h>

#include <QMessageBox>
#include <QMimeData>
#include <QDropEvent>
#include <QKeyEvent>


namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    CInspectorPanel::CInspectorPanel(CATLControlsModel* pATLModel)
        : m_pATLModel(pATLModel)
        , m_selectedType(eACET_NUM_TYPES)
        , m_notFoundColor(QColor(255, 128, 128))
        , m_bAllControlsSameType(true)
    {
        AZ_Assert(m_pATLModel, "CInspectorPanel - The ATL Controls model is null!");

        setupUi(this);

        connect(m_pNameLineEditor, SIGNAL(editingFinished()), this, SLOT(FinishedEditingName()));
        connect(m_pScopeDropDown, SIGNAL(activated(QString)), this, SLOT(SetControlScope(QString)));
        connect(m_pAutoLoadCheckBox, SIGNAL(clicked(bool)), this, SLOT(SetAutoLoadForCurrentControl(bool)));

        // data validators
        m_pNameLineEditor->setValidator(new QRegExpValidator(QRegExp("^[a-zA-Z0-9_]*$"), m_pNameLineEditor));

        uint size = m_pATLModel->GetPlatformCount();
        for (uint i = 0; i < size; ++i)
        {
            QLabel* pLabel = new QLabel(m_pPlatformsWidget);
            pLabel->setText(QString(m_pATLModel->GetPlatformAt(i).c_str()));
            pLabel->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignVCenter);
            pLabel->setIndent(3);
            QComboBox* pPlatformComboBox = new QComboBox(this);
            if (pPlatformComboBox)
            {
                m_platforms.push_back(pPlatformComboBox);
                QVBoxLayout* pLayout = new QVBoxLayout();
                pLayout->setSpacing(0);
                pLayout->setContentsMargins(-1, 0, 0, -1);
                pLayout->addWidget(pLabel);
                pLayout->addWidget(pPlatformComboBox);
                m_pPlatformsLayout->addLayout(pLayout);
                connect(pPlatformComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(SetControlPlatforms()));
            }
        }
        m_pPlatformsLayout->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

        size = m_pATLModel->GetConnectionGroupCount();
        for (uint i = 0; i < size; ++i)
        {
            QConnectionsWidget* pConnectionWidget = new QConnectionsWidget(nullptr);
            pConnectionWidget->Init(m_pATLModel->GetConnectionGroupAt(i));
            pConnectionWidget->layout()->setContentsMargins(3, 3, 3, 3);
            pConnectionWidget->setMinimumSize(QSize(0, 300));
            pConnectionWidget->setMaximumSize(QSize(16777215, 300));
            m_connectionLists.push_back(pConnectionWidget);
            m_pPlatformGroupsTabWidget->addTab(pConnectionWidget, GetGroupIcon(i), QString());

            const size_t nPlatformCount = m_platforms.size();
            for (size_t j = 0; j < nPlatformCount; ++j)
            {
                m_platforms[j]->addItem(GetGroupIcon(i), QString());
            }
        }

        m_pATLModel->AddListener(this);
        m_pConnectionList->Init("");

        Reload();
    }

    //-------------------------------------------------------------------------------------------//
    CInspectorPanel::~CInspectorPanel()
    {
        m_pATLModel->RemoveListener(this);
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::Reload()
    {
        UpdateScopeData();
        UpdateInspector();
        UpdateConnectionData();
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::SetSelectedControls(const ControlList& selectedControls)
    {
        m_selectedType = eACET_NUM_TYPES;
        m_selectedControls.clear();
        m_bAllControlsSameType = true;
        const size_t size = selectedControls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* pControl = m_pATLModel->GetControlByID(selectedControls[i]);
            if (pControl)
            {
                m_selectedControls.push_back(pControl);
                if (m_selectedType == eACET_NUM_TYPES)
                {
                    m_selectedType = pControl->GetType();
                }
                else if (m_bAllControlsSameType && (m_selectedType != pControl->GetType()))
                {
                    m_bAllControlsSameType = false;
                }
            }
        }

        UpdateInspector();
        UpdateConnectionData();
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateConnectionListControl()
    {
        if ((m_selectedControls.size() == 1) && m_bAllControlsSameType && m_selectedType != eACET_SWITCH)
        {
            if (m_selectedType == eACET_PRELOAD)
            {
                m_pConnectedControlsLabel->setHidden(true);
                m_pConnectionList->setHidden(true);
                m_pConnectedSoundbanksLabel->setHidden(false);
                m_pPlatformGroupsTabWidget->setHidden(false);
                m_pPlatformsLabel->setHidden(false);
                m_pPlatformsWidget->setHidden(false);
            }
            else
            {
                m_pConnectedSoundbanksLabel->setHidden(true);
                m_pPlatformGroupsTabWidget->setHidden(true);
                m_pPlatformsLabel->setHidden(true);
                m_pPlatformsWidget->setHidden(true);
                m_pConnectedControlsLabel->setHidden(false);
                m_pConnectionList->setHidden(false);
            }
        }
        else
        {
            m_pConnectedControlsLabel->setHidden(true);
            m_pConnectionList->setHidden(true);
            m_pConnectedSoundbanksLabel->setHidden(true);
            m_pPlatformGroupsTabWidget->setHidden(true);
            m_pPlatformsLabel->setHidden(true);
            m_pPlatformsWidget->setHidden(true);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateScopeControl()
    {
        const size_t size = m_selectedControls.size();
        if (m_bAllControlsSameType)
        {
            if (size == 1)
            {
                CATLControl* pControl = m_selectedControls[0];
                if (pControl)
                {
                    if (m_selectedType == eACET_SWITCH_STATE)
                    {
                        HideScope(true);
                    }
                    else
                    {
                        QString scope = QString(pControl->GetScope().c_str());
                        if (scope.isEmpty())
                        {
                            m_pScopeDropDown->setCurrentIndex(0);
                        }
                        else
                        {
                            int index = m_pScopeDropDown->findText(scope);
                            m_pScopeDropDown->setCurrentIndex(index);
                        }
                        HideScope(false);
                    }
                }
            }
            else
            {
                bool bSameScope = true;
                AZStd::string sScope;
                for (int i = 0; i < size; ++i)
                {
                    CATLControl* pControl = m_selectedControls[i];
                    if (pControl)
                    {
                        AZStd::string sControlScope = pControl->GetScope();
                        if (sControlScope.empty())
                        {
                            sControlScope = "Global";
                        }
                        if (!sScope.empty() && sControlScope != sScope)
                        {
                            bSameScope = false;
                            break;
                        }
                        sScope = sControlScope;
                    }
                }
                if (bSameScope)
                {
                    int index = m_pScopeDropDown->findText(QString(sScope.c_str()));
                    m_pScopeDropDown->setCurrentIndex(index);
                }
                else
                {
                    m_pScopeDropDown->setCurrentIndex(-1);
                }
            }
        }
        else
        {
            HideScope(true);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateNameControl()
    {
        const size_t size = m_selectedControls.size();
        if (m_bAllControlsSameType)
        {
            if (size == 1)
            {
                CATLControl* pControl = m_selectedControls[0];
                if (pControl)
                {
                    m_pNameLineEditor->setText(QString(pControl->GetName().c_str()));
                    m_pNameLineEditor->setEnabled(true);
                }
            }
            else
            {
                m_pNameLineEditor->setText(" <" +  QString::number(size) + tr(" items selected>"));
                m_pNameLineEditor->setEnabled(false);
            }
        }
        else
        {
            m_pNameLineEditor->setText(" <" +  QString::number(size) + tr(" items selected>"));
            m_pNameLineEditor->setEnabled(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateInspector()
    {
        if (!m_selectedControls.empty())
        {
            m_pPropertiesPanel->setHidden(false);
            m_pEmptyInspectorLabel->setHidden(true);
            UpdateNameControl();
            UpdateScopeControl();
            UpdateAutoLoadControl();
            UpdateConnectionListControl();
        }
        else
        {
            m_pPropertiesPanel->setHidden(true);
            m_pEmptyInspectorLabel->setHidden(false);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateConnectionData()
    {
        if ((m_selectedControls.size() == 1) && m_selectedType != eACET_SWITCH)
        {
            if (m_selectedType == eACET_PRELOAD)
            {
                const size_t size = m_connectionLists.size();
                for (size_t i = 0; i < size; ++i)
                {
                    m_connectionLists[i]->SetControl(m_selectedControls[0]);
                }
            }
            else
            {
                m_pConnectionList->SetControl(m_selectedControls[0]);
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateAutoLoadControl()
    {
        if (!m_selectedControls.empty() && m_bAllControlsSameType && m_selectedType == eACET_PRELOAD)
        {
            HideAutoLoad(false);
            bool bHasAutoLoad = false;
            bool bHasNonAutoLoad = false;
            const size_t size = m_selectedControls.size();
            for (int i = 0; i < size; ++i)
            {
                CATLControl* pControl = m_selectedControls[i];
                if (pControl)
                {
                    if (pControl->IsAutoLoad())
                    {
                        bHasAutoLoad = true;
                    }
                    else
                    {
                        bHasNonAutoLoad = true;
                    }
                }
            }

            if (bHasAutoLoad && bHasNonAutoLoad)
            {
                m_pAutoLoadCheckBox->setTristate(true);
                m_pAutoLoadCheckBox->setCheckState(Qt::PartiallyChecked);
            }
            else
            {
                if (bHasAutoLoad)
                {
                    m_pAutoLoadCheckBox->setChecked(true);
                }
                else
                {
                    m_pAutoLoadCheckBox->setChecked(false);
                }
                m_pAutoLoadCheckBox->setTristate(false);
            }
        }
        else
        {
            HideAutoLoad(true);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::UpdateScopeData()
    {
        m_pScopeDropDown->clear();
        for (int j = 0; j < m_pATLModel->GetScopeCount(); ++j)
        {
            SControlScope scope = m_pATLModel->GetScopeAt(j);
            m_pScopeDropDown->insertItem(0, QString(scope.name.c_str()));
            if (scope.bOnlyLocal)
            {
                m_pScopeDropDown->setItemData(0, m_notFoundColor, Qt::ForegroundRole);
                m_pScopeDropDown->setItemData(0, "Level not found but it is referenced by some audio controls", Qt::ToolTipRole);
            }
            else
            {
                m_pScopeDropDown->setItemData(0, "", Qt::ToolTipRole);
            }
        }
        m_pScopeDropDown->insertItem(0, tr("Global"));
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::SetControlName(QString sName)
    {
        if (m_selectedControls.size() == 1)
        {
            if (!sName.isEmpty())
            {
                CUndo undo("Audio Control Name Changed");
                AZStd::string newName = sName.toUtf8().data();
                CATLControl* pControl = m_selectedControls[0];
                if (pControl && pControl->GetName() != newName)
                {
                    if (!m_pATLModel->IsNameValid(newName, pControl->GetType(), pControl->GetScope(), pControl->GetParent()))
                    {
                        newName = m_pATLModel->GenerateUniqueName(newName, pControl->GetType(), pControl->GetScope(), pControl->GetParent());
                    }
                    pControl->SetName(newName);
                }
            }
            else
            {
                UpdateNameControl();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::SetControlScope(QString scope)
    {
        CUndo undo("Audio Control Scope Changed");
        size_t size = m_selectedControls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* pControl = m_selectedControls[i];
            if (pControl)
            {
                QString currentScope = QString(pControl->GetScope().c_str());
                if (currentScope != scope && (scope != tr("Global") || currentScope != ""))
                {
                    if (scope == tr("Global"))
                    {
                        pControl->SetScope("");
                    }
                    else
                    {
                        pControl->SetScope(scope.toUtf8().data());
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::SetAutoLoadForCurrentControl(bool bAutoLoad)
    {
        CUndo undo("Audio Control Auto-Load Property Changed");
        size_t size = m_selectedControls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* pControl = m_selectedControls[i];
            if (pControl)
            {
                pControl->SetAutoLoad(bAutoLoad);
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::SetControlPlatforms()
    {
        if (m_selectedControls.size() == 1)
        {
            CATLControl* pControl = m_selectedControls[0];
            if (pControl)
            {
                const size_t size = m_platforms.size();
                for (size_t i = 0; i < size; ++i)
                {
                    pControl->SetGroupForPlatform(m_pATLModel->GetPlatformAt(i), m_platforms[i]->currentIndex());
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::FinishedEditingName()
    {
        SetControlName(m_pNameLineEditor->text());
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::OnControlModified(AudioControls::CATLControl* pControl)
    {
        size_t size = m_selectedControls.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (m_selectedControls[i] == pControl)
            {
                UpdateInspector();
                UpdateConnectionData();
                break;
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::HideScope(bool bHide)
    {
        m_pScopeLabel->setHidden(bHide);
        m_pScopeDropDown->setHidden(bHide);
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::HideAutoLoad(bool bHide)
    {
        m_pAutoLoadLabel->setHidden(bHide);
        m_pAutoLoadCheckBox->setHidden(bHide);
    }

    //-------------------------------------------------------------------------------------------//
    void CInspectorPanel::HideGroupConnections(bool bHide)
    {
        m_pPlatformGroupsTabWidget->setHidden(bHide);
    }
} // namespace AudioControls

#include <Source/Editor/InspectorPanel.moc>
