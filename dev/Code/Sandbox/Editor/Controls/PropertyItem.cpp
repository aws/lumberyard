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

/*
    Relationship between Variable data, PropertyItem value string, and draw string

        Variable:
            holds raw data
        PropertyItem.m_value string:
            Converted via Variable.Get/SetDisplayValue
            = Same as Variable.Get(string) and Variable.Set(string), EXCEPT:
                Enums converted to enum values
                Color reformated to 8-bit range
                SOHelper removes prefix before :
        Draw string:
            Same as .m_value, EXCEPT
                .valueMultiplier conversion applied
                Bool reformated to "True/False"
                Curves replaced with "[Curve]"
                bShowChildren vars add child draw strings
        Input string:
            Set to .m_value, EXCEPT
                Bool converted to "1/0"
                File text replace \ with /
                Vector expand single elem to 3 repeated
*/


#include "StdAfx.h"
#include "PropertyItem.h"
#include "PropertyCtrl.h"
#include "InPlaceEdit.h"
#include "InPlaceComboBox.h"
#include "InPlaceButton.h"
#include "FillSliderCtrl.h"
#include "SplineCtrl.h"
#include "ColorGradientCtrl.h"
#include "SliderCtrlEx.h"
#include "QtViewPaneManager.h"

#include "EquipPackDialog.h"
#include "SelectEAXPresetDlg.h"

#include "ShaderEnum.h"
#include "ShadersDialog.h"

// AI
#include "AIDialog.h"
#include "AICharactersDialog.h"
#include "AIPFPropertiesListDialog.h"
#include "AIEntityClassesDialog.h"
#include "AIAnchorsDialog.h"
#include "AI/AIBehaviorLibrary.h"
#include "AI/AIGoalLibrary.h"
#include "AI/AIManager.h"

// Smart Objects
#include "SmartObjectClassDialog.h"
#include "SmartObjectStateDialog.h"
#include "SmartObjectPatternDialog.h"
#include "SmartObjectActionDialog.h"
#include "SmartObjectHelperDialog.h"
#include "SmartObjectEventDialog.h"
#include "SmartObjectTemplateDialog.h"

#include "CustomActions/CustomActionDialog.h"

#include "Material/MaterialManager.h"

#include "SelectGameTokenDialog.h"
#include "GameTokens/GameTokenManager.h"
#include "UIEnumsDatabase.h"
#include "SelectSequenceDialog.h"
#include "SelectMissionObjectiveDialog.h"
#include "GenericSelectItemDialog.h"
#include "SelectLightAnimationDialog.h"

#include <ITimer.h>
#include <ILocalizationManager.h>
#include <IMusicSystem.h>

#include "Include/IAssetItem.h"
#include "AssetBrowser/AssetBrowserDialog.h"
#include "ISourceControl.h"

#include "Util/UIEnumerations.h"

#include "Objects/ShapeObject.h"
#include "Objects/AIWave.h"

#include <CryName.h>
#include "CharacterEditor/AnimationBrowser.h"

#include "DataBaseDialog.h"

#include "Particles/ParticleManager.h"
#include "Particles/ParticleItem.h"
#include "Particles/ParticleDialog.h"
#include "LensFlareEditor/LensFlareEditor.h"

#include "MultiMonHelper.h"

#include "IResourceSelectorHost.h"
#include "UnicodeFunctions.h"

#include "Undo/UndoVariableChange.h"

#include "UiEditorDLLBus.h"

#include "MainWindow.h"

#include <QTimer>
#include <QColorDialog>
#include <QMessageBox>

//////////////////////////////////////////////////////////////////////////
//wraps QColorGradientControl until PropertyItem is ported to Qt.
#include "QtWinMigrate/qwinwidget.h"
#include <QHBoxLayout>

#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#define GetAValue(rgba)      (LOBYTE((rgba)>>24)) // Microsoft does not provide this one so let's make our own.
#define RGBA(r,g,b,a)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)|(((DWORD)(BYTE)(a))<<24)))


class CColorGradientCtrlWrapper
    : public QWinWidget
{
public:
    CColorGradientCtrlWrapper(CWnd* parentWnd, QObject* parent = 0, Qt::WindowFlags f = 0)
        : QWinWidget(parentWnd, parent, f)
    {
        m_cColorSpline = new CColorGradientCtrl(this);
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(m_cColorSpline);
    }

    CColorGradientCtrl* m_cColorSpline;
};

template<typename T>
class CWidgetWrapper : public CWnd
{
public:
    CWidgetWrapper(CWnd* parent)
    {
        Create(NULL, NULL, WS_VISIBLE | WS_CHILD, CRect(0, 0, 100, 100), parent, NULL);
        SetOwner(parent);
    }
    T* widget() { return m_widget; }

protected:
    T* m_widget;
    QScopedPointer<QWinWidget> m_winWidget;

    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct)
    {
        if (CWnd::OnCreate(lpCreateStruct) == -1)
            return -1;

        m_winWidget.reset(new QWinWidget(this));
        m_widget = new T;
        m_winWidget->setLayout(new QHBoxLayout);
        m_winWidget->layout()->setMargin(0);
        m_winWidget->layout()->addWidget(m_widget);
        m_winWidget->show();
        return 0;
    }

    afx_msg void OnSize(UINT nType, int cx, int cy)
    {
        CWnd::OnSize(nType, cx, cy);
        CRect rect;
        GetClientRect(&rect);
        m_winWidget->setGeometry(rect.left, rect.top, rect.Width(), rect.Height());
    }
    DECLARE_MESSAGE_MAP()
};

BEGIN_TEMPLATE_MESSAGE_MAP(CWidgetWrapper, T, CWnd)
    ON_WM_CREATE()
    ON_WM_SIZE()
    END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////
#define CMD_ADD_CHILD_ITEM 100
#define CMD_ADD_ITEM 101
#define CMD_DELETE_ITEM 102

#define BUTTON_WIDTH (16)
#define NUMBER_CTRL_WIDTH 60

const char* DISPLAY_NAME_ATTR   = "DisplayName";
const char* VALUE_ATTR = "Value";
const char* TYPE_ATTR   =   "Type";
const char* TIP_ATTR    = "Tip";
const char* TIP_CVAR_ATTR   = "TipCVar";
const char* FILEFILTER_ATTR = "FileFilters";
const int FLOAT_NUM_DIGITS = 6;

// default number of increments to cover the range of a property - determined experimentally by feel
const float CPropertyItem::s_DefaultNumStepIncrements = 500.0f;

HDWP CPropertyItem::s_HDWP = 0;

//////////////////////////////////////////////////////////////////////////
// CPropertyItem implementation.
//////////////////////////////////////////////////////////////////////////
CPropertyItem::CPropertyItem(CPropertyCtrl* pCtrl)
{
    m_propertyCtrl = pCtrl;
    m_pControlsHostWnd = 0;
    m_parent = 0;
    m_bExpandable = false;
    m_bExpanded = false;
    m_bEditChildren = false;
    m_bShowChildren = false;
    m_bSelected = false;
    m_bNoCategory = false;

    m_pStaticText = 0;
    m_cNumber = 0;
    m_cNumber1 = 0;
    m_cNumber2 = 0;
    m_cNumber3 = 0;

    m_cFillSlider = 0;
    m_cEdit = 0;
    m_cCombo = 0;
    m_cButton = 0;
    m_cButton2 = 0;
    m_cButton3 = 0;
    m_cButton4 = 0;
    m_cButton5 = 0;
    m_cExpandButton = 0;
    m_cCheckBox = 0;
    m_cSpline = 0;
    m_cColorSpline = 0;
    m_cColorButton = 0;

    m_image = -1;
    m_bIgnoreChildsUpdate = false;
    m_value = "";
    m_type = ePropertyInvalid;

    m_modified = false;
    m_bMoveControls = false;
    m_valueMultiplier = 1;
    m_pEnumDBItem = 0;

    m_bForceModified = false;
    m_bDontSendToControl = false;

    m_nHeight = 14;

    m_nCategoryPageId = -1;

    m_pAnimBrowser = NULL;

    m_strNoScriptDefault = "<<undefined>>";
    m_strScriptDefault = m_strNoScriptDefault;
}

CPropertyItem::~CPropertyItem()
{
    SAFE_DELETE(m_pAnimBrowser);

    // just to make sure we dont double (or infinitely recurse...) delete
    AddRef();

    DestroyInPlaceControl();

    if (m_pVariable)
    {
        ReleaseVariable();
    }

    for (int i = 0; i < m_childs.size(); i++)
    {
        m_childs[i]->m_parent = 0;
    }

    // explicitly shrink the array and delete the child to prevent violated access 
    // to the deleted entry of the array from other thread (CPropertyCtrl::OnEraseBkgnd
    // was causing such an issue before)
    while (!m_childs.empty())
    {
        auto child = *m_childs.begin();
        m_childs.erase(m_childs.begin());
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SetXmlNode(XmlNodeRef& node)
{
    m_node = node;
    // No Undo while just creating properties.
    //GetIEditor()->SuspendUndo();
    ParseXmlNode();
    //GetIEditor()->ResumeUndo();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::ParseXmlNode(bool bRecursive /* =true  */)
{
    if (!m_node->getAttr(DISPLAY_NAME_ATTR, m_name))
    {
        m_name = m_node->getTag();
    }

    CString value;
    bool bHasValue = m_node->getAttr(VALUE_ATTR, value);

    CString type;
    m_node->getAttr(TYPE_ATTR, type);

    m_tip = "";
    m_node->getAttr(TIP_ATTR, m_tip);

    m_type = Prop::GetType(type);
    m_image = Prop::GetNumImages(type);

    m_rangeMin = 0;
    m_rangeMax = 100;
    m_step = 0.1f;
    m_bHardMin = m_bHardMax = false;
    if (m_type == ePropertyFloat || m_type == ePropertyInt)
    {
        if (m_node->getAttr("Min", m_rangeMin))
        {
            m_bHardMin = true;
        }
        if (m_node->getAttr("Max", m_rangeMax))
        {
            m_bHardMax = true;
        }

        int nPrecision;
        if (!m_node->getAttr("Precision", nPrecision))
        {
            nPrecision = max(3 - int(log(m_rangeMax - m_rangeMin) / log(10.f)), 0);
        }
        m_step = powf(10.f, -nPrecision);
    }

    if (bHasValue)
    {
        SetValue(value, false);
    }

    m_bNoCategory = false;

    if ((m_type == ePropertyVector || m_type == ePropertyVector2 || m_type == ePropertyVector4) && !m_propertyCtrl->IsExtenedUI())
    {
        m_bEditChildren = true;
        bRecursive = false;
        m_childs.clear();

        if (m_type == ePropertyVector)
        {
            Vec3 vec;
            m_node->getAttr(VALUE_ATTR, vec);
            // Create 3 sub elements.
            XmlNodeRef x = m_node->createNode("X");
            XmlNodeRef y = m_node->createNode("Y");
            XmlNodeRef z = m_node->createNode("Z");
            x->setAttr(TYPE_ATTR, "Float");
            y->setAttr(TYPE_ATTR, "Float");
            z->setAttr(TYPE_ATTR, "Float");

            x->setAttr(VALUE_ATTR, vec.x);
            y->setAttr(VALUE_ATTR, vec.y);
            z->setAttr(VALUE_ATTR, vec.z);

            // Start ignoring all updates comming from childs. (Initializing childs).
            m_bIgnoreChildsUpdate = true;

            CPropertyItemPtr itemX = new CPropertyItem(m_propertyCtrl);
            itemX->SetXmlNode(x);
            AddChild(itemX);

            CPropertyItemPtr itemY = new CPropertyItem(m_propertyCtrl);
            itemY->SetXmlNode(y);
            AddChild(itemY);

            CPropertyItemPtr itemZ = new CPropertyItem(m_propertyCtrl);
            itemZ->SetXmlNode(z);
            AddChild(itemZ);
        }
        else if (m_type == ePropertyVector2)
        {
            Vec2 vec;
            m_node->getAttr(VALUE_ATTR, vec);
            // Create 3 sub elements.
            XmlNodeRef x = m_node->createNode("X");
            XmlNodeRef y = m_node->createNode("Y");
            x->setAttr(TYPE_ATTR, "Float");
            y->setAttr(TYPE_ATTR, "Float");

            x->setAttr(VALUE_ATTR, vec.x);
            y->setAttr(VALUE_ATTR, vec.y);

            // Start ignoring all updates comming from childs. (Initializing childs).
            m_bIgnoreChildsUpdate = true;

            CPropertyItemPtr itemX = new CPropertyItem(m_propertyCtrl);
            itemX->SetXmlNode(x);
            AddChild(itemX);

            CPropertyItemPtr itemY = new CPropertyItem(m_propertyCtrl);
            itemY->SetXmlNode(y);
            AddChild(itemY);
        }
        else if (m_type == ePropertyVector4)
        {
            Vec4 vec;
            m_node->getAttr(VALUE_ATTR, vec);
            // Create 3 sub elements.
            XmlNodeRef x = m_node->createNode("X");
            XmlNodeRef y = m_node->createNode("Y");
            XmlNodeRef z = m_node->createNode("Z");
            XmlNodeRef w = m_node->createNode("W");
            x->setAttr(TYPE_ATTR, "Float");
            y->setAttr(TYPE_ATTR, "Float");
            z->setAttr(TYPE_ATTR, "Float");
            w->setAttr(TYPE_ATTR, "Float");

            x->setAttr(VALUE_ATTR, vec.x);
            y->setAttr(VALUE_ATTR, vec.y);
            z->setAttr(VALUE_ATTR, vec.z);
            w->setAttr(VALUE_ATTR, vec.w);

            // Start ignoring all updates comming from childs. (Initializing childs).
            m_bIgnoreChildsUpdate = true;

            CPropertyItemPtr itemX = new CPropertyItem(m_propertyCtrl);
            itemX->SetXmlNode(x);
            AddChild(itemX);

            CPropertyItemPtr itemY = new CPropertyItem(m_propertyCtrl);
            itemY->SetXmlNode(y);
            AddChild(itemY);

            CPropertyItemPtr itemZ = new CPropertyItem(m_propertyCtrl);
            itemZ->SetXmlNode(z);
            AddChild(itemZ);

            CPropertyItemPtr itemW = new CPropertyItem(m_propertyCtrl);
            itemW->SetXmlNode(w);
            AddChild(itemW);
        }
        m_bNoCategory = true;
        m_bExpandable = true;
        m_bIgnoreChildsUpdate = false;
    }
    else if (bRecursive)
    {
        // If recursive and not vector.

        m_bExpandable = false;
        // Create sub-nodes.
        for (int i = 0; i < m_node->getChildCount(); i++)
        {
            m_bIgnoreChildsUpdate = true;

            XmlNodeRef child = m_node->getChild(i);
            CPropertyItemPtr item = new CPropertyItem(m_propertyCtrl);
            item->SetXmlNode(m_node->getChild(i));
            AddChild(item);
            m_bExpandable = true;

            m_bIgnoreChildsUpdate = false;
        }
    }
    m_modified = false;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SetVariable(IVariable* var)
{
    if (var == m_pVariable)
    {
        // Early exit optimization if setting the same var as the current var.
        // A common use case, in Track View for example, is to re-use the save var for a property when switching to a new
        // instance of the same variable. The visible display of the value is often handled by invalidating the property,
        // but the non-visible attributes, i.e. the range values, are usually set using this method. Thus we reset the ranges
        // explicitly here when the Ivariable var is the same
        float min, max, step;
        bool  hardMin, hardMax;

        var->GetLimits(min, max, step, hardMin, hardMax);

        m_rangeMin = min;
        m_rangeMax = max;
        m_step = step;
        m_bHardMin = hardMin;
        m_bHardMax = hardMax;

        return;
    }
    _smart_ptr<IVariable> pInputVar = var;

    // Release previous variable.
    if (m_pVariable)
    {
        ReleaseVariable();
    }

    m_pVariable = pInputVar;
    assert(m_pVariable != NULL);

    m_pVariable->AddOnSetCallback(functor(*this, &CPropertyItem::OnVariableChange));
    m_pVariable->EnableNotifyWithoutValueChange(m_bForceModified);

    m_tip = "";
    m_name = m_pVariable->GetHumanName();

    int i = 0;

    int dataType = m_pVariable->GetDataType();

    m_valueMultiplier = 1;
    m_rangeMin = 0;
    m_rangeMax = 100;
    m_step = 0.f;
    m_bHardMin = m_bHardMax = false;

    // Fetch base parameter description
    Prop::Description desc(m_pVariable);
    m_type = desc.m_type;
    m_image = desc.m_numImages;
    m_enumList = desc.m_enumList;
    m_type = desc.m_type;
    m_rangeMin = desc.m_rangeMin;
    m_rangeMax = desc.m_rangeMax;
    m_step = desc.m_step;
    m_bHardMin = desc.m_bHardMin;
    m_bHardMax = desc.m_bHardMax;
    m_valueMultiplier = desc.m_valueMultiplier;
    m_pEnumDBItem = desc.m_pEnumDBItem;

    //////////////////////////////////////////////////////////////////////////
    VarToValue();

    m_bNoCategory = false;

    switch (m_type)
    {
    case ePropertyAiPFPropertiesList:
        m_bEditChildren = true;
        AddChildrenForPFProperties();
        m_bNoCategory = true;
        m_bExpandable = true;
        break;

    case ePropertyAiEntityClasses:
        m_bEditChildren = true;
        AddChildrenForAIEntityClasses();
        m_bNoCategory = true;
        m_bExpandable = true;
        break;

    case ePropertyVector2:
        if (!m_propertyCtrl->IsExtenedUI())
        {
            m_bEditChildren = true;
            m_childs.clear();

            Vec2 vec;
            m_pVariable->Get(vec);
            IVariable* pVX = new CVariable<float>();
            pVX->SetName("x");
            pVX->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
            pVX->Set(vec.x);
            IVariable* pVY = new CVariable<float>();
            pVY->SetName("y");
            pVY->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
            pVY->Set(vec.y);

            // Start ignoring all updates coming from childs. (Initializing childs).
            m_bIgnoreChildsUpdate = true;

            CPropertyItemPtr itemX = new CPropertyItem(m_propertyCtrl);
            itemX->SetVariable(pVX);
            AddChild(itemX);

            CPropertyItemPtr itemY = new CPropertyItem(m_propertyCtrl);
            itemY->SetVariable(pVY);
            AddChild(itemY);

            m_bNoCategory = true;
            m_bExpandable = true;

            m_bIgnoreChildsUpdate = false;
        }
        break;

    case ePropertyVector4:
        if (!m_propertyCtrl->IsExtenedUI())
        {
            m_bEditChildren = true;
            m_childs.clear();

            Vec4 vec;
            m_pVariable->Get(vec);
            IVariable* pVX = new CVariable<float>();
            pVX->SetName("x");
            pVX->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
            pVX->Set(vec.x);
            IVariable* pVY = new CVariable<float>();
            pVY->SetName("y");
            pVY->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
            pVY->Set(vec.y);
            IVariable* pVZ = new CVariable<float>();
            pVZ->SetName("z");
            pVZ->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
            pVZ->Set(vec.z);
            IVariable* pVW = new CVariable<float>();
            pVW->SetName("w");
            pVW->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
            pVW->Set(vec.w);

            // Start ignoring all updates coming from childs. (Initializing childs).
            m_bIgnoreChildsUpdate = true;

            CPropertyItemPtr itemX = new CPropertyItem(m_propertyCtrl);
            itemX->SetVariable(pVX);
            AddChild(itemX);

            CPropertyItemPtr itemY = new CPropertyItem(m_propertyCtrl);
            itemY->SetVariable(pVY);
            AddChild(itemY);

            CPropertyItemPtr itemZ = new CPropertyItem(m_propertyCtrl);
            itemZ->SetVariable(pVZ);
            AddChild(itemZ);

            CPropertyItemPtr itemW = new CPropertyItem(m_propertyCtrl);
            itemZ->SetVariable(pVW);
            AddChild(itemW);

            m_bNoCategory = true;
            m_bExpandable = true;

            m_bIgnoreChildsUpdate = false;
        }
        break;

    case ePropertyVector:
        if (!m_propertyCtrl->IsExtenedUI())
        {
            m_bEditChildren = true;
            m_childs.clear();

            Vec3 vec;
            m_pVariable->Get(vec);
            IVariable* pVX = new CVariable<float>();
            pVX->SetName("x");
            pVX->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
            pVX->Set(vec.x);
            IVariable* pVY = new CVariable<float>();
            pVY->SetName("y");
            pVY->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
            pVY->Set(vec.y);
            IVariable* pVZ = new CVariable<float>();
            pVZ->SetName("z");
            pVZ->SetLimits(m_rangeMin, m_rangeMax, m_step, m_bHardMin, m_bHardMax);
            pVZ->Set(vec.z);

            // Start ignoring all updates coming from childs. (Initializing childs).
            m_bIgnoreChildsUpdate = true;

            CPropertyItemPtr itemX = new CPropertyItem(m_propertyCtrl);
            itemX->SetVariable(pVX);
            AddChild(itemX);

            CPropertyItemPtr itemY = new CPropertyItem(m_propertyCtrl);
            itemY->SetVariable(pVY);
            AddChild(itemY);

            CPropertyItemPtr itemZ = new CPropertyItem(m_propertyCtrl);
            itemZ->SetVariable(pVZ);
            AddChild(itemZ);

            m_bNoCategory = true;
            m_bExpandable = true;

            m_bIgnoreChildsUpdate = false;
        }
        break;
    }

    if (m_pVariable->GetNumVariables() > 0)
    {
        if (m_type == ePropertyInvalid)
        {
            m_type = ePropertyTable;
        }
        m_bExpandable = true;
        if (m_pVariable->GetFlags() & IVariable::UI_SHOW_CHILDREN)
        {
            m_bShowChildren = true;
        }
        m_bIgnoreChildsUpdate = true;
        for (i = 0; i < m_pVariable->GetNumVariables(); i++)
        {
            CPropertyItemPtr item = new CPropertyItem(m_propertyCtrl);
            item->SetVariable(m_pVariable->GetVariable(i));
            AddChild(item);
        }
        m_bIgnoreChildsUpdate = false;
    }

    m_modified = false;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::ReleaseVariable()
{
    if (m_pVariable)
    {
        // Unwire all from variable.
        m_pVariable->RemoveOnSetCallback(functor(*this, &CPropertyItem::OnVariableChange));
    }
    m_pVariable = 0;
}

//////////////////////////////////////////////////////////////////////////
//! Find item that reference specified property.
CPropertyItem* CPropertyItem::FindItemByVar(IVariable* pVar)
{
    if (m_pVariable == pVar)
    {
        return this;
    }
    for (int i = 0; i < m_childs.size(); i++)
    {
        CPropertyItem* pFound = m_childs[i]->FindItemByVar(pVar);
        if (pFound)
        {
            return pFound;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
CString CPropertyItem::GetFullName() const
{
    if (m_parent)
    {
        return m_parent->GetFullName() + "::" + m_name;
    }
    else
    {
        return m_name;
    }
}

//////////////////////////////////////////////////////////////////////////
CPropertyItem* CPropertyItem::FindItemByFullName(const CString& name)
{
    if (GetFullName() == name)
    {
        return this;
    }
    for (int i = 0; i < m_childs.size(); i++)
    {
        CPropertyItem* pFound = m_childs[i]->FindItemByFullName(name);
        if (pFound)
        {
            return pFound;
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::AddChild(CPropertyItem* item)
{
    assert(item);
    m_bExpandable = true;
    item->m_parent = this;
    m_childs.push_back(item);
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::RemoveChild(CPropertyItem* item)
{
    // Find item and erase it from childs array.
    for (int i = 0; i < m_childs.size(); i++)
    {
        if (item == m_childs[i])
        {
            m_childs.erase(m_childs.begin() + i);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::RemoveAllChildren()
{
    m_childs.clear();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnChildChanged(CPropertyItem* child)
{
    if (m_bIgnoreChildsUpdate)
    {
        return;
    }

    if (m_bEditChildren)
    {
        // Update parent.
        CString sValue;
        if ((m_type == ePropertyAiPFPropertiesList) || (m_type == ePropertyAiEntityClasses))
        {
            for (int i = 0; i < m_childs.size(); i++)
            {
                CString token;
                int index = 0;
                CString value = m_childs[i]->GetValue();
                while (!(token = value.Tokenize(" ,", index)).IsEmpty())
                {
                    if (!sValue.IsEmpty())
                    {
                        sValue += ", ";
                    }
                    sValue += token;
                }
            }
        }
        else
        {
            for (int i = 0; i < m_childs.size(); i++)
            {
                if (i > 0)
                {
                    sValue += ", ";
                }
                sValue += m_childs[i]->GetValue();
            }
        }
        SetValue(sValue, false);
    }
    if (m_bEditChildren || m_bShowChildren)
    {
        SendToControl();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SetSelected(bool selected)
{
    m_bSelected = selected;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SetExpanded(bool expanded)
{
    if (IsDisabled())
    {
        m_bExpanded = false;
    }
    else
    {
        m_bExpanded = expanded;
    }
}

bool CPropertyItem::IsDisabled() const
{
    if (m_pVariable)
    {
        return m_pVariable->GetFlags() & IVariable::UI_DISABLED;
    }
    return false;
}

bool CPropertyItem::IsInvisible() const
{
    if (m_pVariable)
    {
        return m_pVariable->GetFlags() & IVariable::UI_INVISIBLE;
    }
    return false;
}


bool CPropertyItem::IsBold() const
{
    if (m_pVariable)
    {
        return m_pVariable->GetFlags() & IVariable::UI_BOLD;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::CreateInPlaceControl(CWnd* pWndParent, CRect& ctrlRect)
{
    if (IsDisabled())
    {
        return;
    }

    m_controls.clear();

    m_bMoveControls = true;
    CRect nullRc(0, 0, 0, 0);
    DWORD style;
    switch (m_type)
    {
    case ePropertyFloat:
    case ePropertyInt:
    case ePropertyAngle:
    {
        if (m_pEnumDBItem)
        {
            // Combo box.
            CreateInPlaceComboBox(pWndParent, WS_CHILD | WS_VISIBLE);
            PopulateInPlaceComboBox(m_pEnumDBItem->strings);
        }
        else
        {
            m_cNumber = new CNumberCtrl;
            m_cNumber->SetUpdateCallback(functor(*this, &CPropertyItem::OnNumberCtrlUpdate));
            m_cNumber->EnableUndo(m_name + " Modified");
            m_cNumber->EnableNotifyWithoutValueChange(m_bForceModified);
            //m_cNumber->SetBeginUpdateCallback( functor(*this,OnNumberCtrlBeginUpdate) );
            //m_cNumber->SetEndUpdateCallback( functor(*this,OnNumberCtrlEndUpdate) );

            // (digits behind the comma, only used for floats)
            if (m_type == ePropertyFloat)
            {
                m_cNumber->SetStep(m_step);
                m_cNumber->SetFloatFormatPrecision(FLOAT_NUM_DIGITS);
            }
            else if (m_type == ePropertyAngle)
            {
                m_cNumber->SetFloatFormatPrecision(FLOAT_NUM_DIGITS);
            }

            // Only for integers.
            if (m_type == ePropertyInt)
            {
                m_cNumber->SetInteger(true);
            }
            m_cNumber->SetMultiplier(m_valueMultiplier);
            m_cNumber->SetRange(m_bHardMin ? m_rangeMin : -FLT_MAX, m_bHardMax ? m_rangeMax : FLT_MAX);

            m_cNumber->Create(pWndParent, nullRc, 1, CNumberCtrl::NOBORDER | CNumberCtrl::LEFTALIGN);
            m_cNumber->SetLeftAlign(true);

            m_cFillSlider = new CFillSliderCtrl;
            m_cFillSlider->EnableUndo(m_name + " Modified");
            m_cFillSlider->Create(WS_VISIBLE | WS_CHILD, nullRc, pWndParent, 2);
            m_cFillSlider->SetUpdateCallback(functor(*this, &CPropertyItem::OnFillSliderCtrlUpdate));
            m_cFillSlider->SetRangeFloat(m_rangeMin / m_valueMultiplier, m_rangeMax / m_valueMultiplier, m_step / m_valueMultiplier);
        }
    }
    break;

    case ePropertyTable:
        if (!m_bEditChildren)
        {
            break;
        }

    case ePropertyString:
        if (m_pEnumDBItem)
        {
            // Combo box.
            CreateInPlaceComboBox(pWndParent, WS_CHILD | WS_VISIBLE);
            PopulateInPlaceComboBox(m_pEnumDBItem->strings);
            break;
        }
        else
        {
            // Here we must check the global table to see if this enum should be loaded from the XML.
            TDValues*   pcValue = (m_pVariable) ? GetEnumValues(m_pVariable->GetName()) : NULL;

            // If there is not even this, we THEN fall back to create the edit box.
            if (pcValue)
            {
                CreateInPlaceComboBox(pWndParent, WS_CHILD | WS_VISIBLE);
                PopulateInPlaceComboBox(*pcValue);
                break;
            }
        }
    // ... else, fall through to create edit box.

    case ePropertyVector:
    case ePropertyVector2:
    case ePropertyVector4:
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
        break;

    case ePropertyShader:
    case ePropertyMaterial:
    case ePropertyAiBehavior:
    case ePropertyAiAnchor:
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
    case ePropertyAiCharacter:
#endif
    case ePropertyAiPFPropertiesList:
    case ePropertyAiEntityClasses:
    case ePropertyEquip:
    case ePropertyReverbPreset:
    case ePropertySOClass:
    case ePropertySOClasses:
    case ePropertySOState:
    case ePropertySOStates:
    case ePropertySOStatePattern:
    case ePropertySOAction:
    case ePropertySOHelper:
    case ePropertySONavHelper:
    case ePropertySOAnimHelper:
    case ePropertySOEvent:
    case ePropertySOTemplate:
    case ePropertyCustomAction:
    case ePropertyGameToken:
    case ePropertyMissionObj:
    case ePropertySequence:
    case ePropertySequenceId:
    case ePropertyUser:
    case ePropertyLocalString:
    case ePropertyLightAnimation:
    case ePropertyParticleName:
    case ePropertyUiElement:
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        style = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT;
        if (m_type == ePropertySequenceId)
        {
            style |= ES_READONLY;
        }
        m_cEdit->Create(style, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
        if (m_type == ePropertyShader)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnShaderBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyAiBehavior)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAIBehaviorBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyAiAnchor)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAIAnchorBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
        if (m_type == ePropertyAiCharacter)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAICharacterBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
#endif
        if (m_type == ePropertyAiPFPropertiesList)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAIPFPropertiesListBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyAiEntityClasses)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAIEntityClassesBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyEquip)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnEquipBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyReverbPreset)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnEAXPresetBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyMaterial)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnMaterialBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);

            m_cButton2 = new CInPlaceButton(functor(*this, &CPropertyItem::OnMaterialPickSelectedButton));
            m_cButton2->Create("<", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 5);
        }
        else if (m_type == ePropertySOClass)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOClassBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOClasses)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOClassesBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOState)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOStateBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOStates)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOStatesBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOStatePattern)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOStatePatternBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOAction)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOActionBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOHelper)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOHelperBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySONavHelper)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSONavHelperBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOAnimHelper)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOAnimHelperBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOEvent)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOEventBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOTemplate)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOTemplateBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyCustomAction)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnCustomActionBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyGameToken)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnGameTokenBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySequence)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSequenceBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySequenceId)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSequenceIdBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyMissionObj)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnMissionObjButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyUser)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnUserBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyLocalString)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnLocalStringBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyLightAnimation)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnLightAnimationBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyParticleName)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnParticleBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);

            m_cButton2 = new CInPlaceButton(functor(*this, &CPropertyItem::OnParticleApplyButton));
            m_cButton2->Create("<", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 5);
        }
        else if (m_type == ePropertyUiElement)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnUiElementBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);

            m_cButton2 = new CInPlaceButton(functor(*this, &CPropertyItem::OnUiElementPickSelectedButton));
            m_cButton2->Create("<", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 5);
        }
        break;

    case ePropertyAiTerritory:
    case ePropertyAiWave:
    case ePropertySelection:
    {
        if (m_type == ePropertyAiTerritory)
        {
            PopulateAITerritoriesList();
        }

        if (m_type == ePropertyAiWave)
        {
            PopulateAIWavesList();
        }

        // Combo box.
        CreateInPlaceComboBox(pWndParent, WS_CHILD | WS_VISIBLE);
        PopulateInPlaceComboBoxFromEnumList();
    }
    break;

    case ePropertyColor:
    {
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
        m_cColorButton = new CInPlaceColorButton(functor(*this, &CPropertyItem::OnColorBrowseButton));
        m_cColorButton->Create("", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, nullRc, pWndParent, 4);
    }
    break;

    case ePropertyFloatCurve:
    {
        m_cSpline = new CWidgetWrapper<CSplineCtrl>(pWndParent);
        m_cSpline->widget()->show();
        m_cSpline->widget()->SetUpdateCallback((CSplineCtrl::UpdateCallback const&)functor(*this, &CPropertyItem::ReceiveFromControl));
        m_cSpline->widget()->SetTimeRange(0, 1);
        m_cSpline->widget()->SetValueRange(0, 1);
        m_cSpline->widget()->SetGrid(12, 12);
        m_cSpline->widget()->SetSpline(m_pVariable->GetSpline(true));
    }
    break;

    case ePropertyColorCurve:
    {
        m_cColorSpline = new CColorGradientCtrlWrapper(pWndParent);
        m_cColorSpline->show();
        m_cColorSpline->m_cColorSpline->SetUpdateCallback((CColorGradientCtrl::UpdateCallback const&)functor(*this, &CPropertyItem::ReceiveFromControl));
        m_cColorSpline->m_cColorSpline->SetTimeRange(0, 1);
        m_cColorSpline->m_cColorSpline->SetSpline(m_pVariable->GetSpline(true));
    }
    break;

    case ePropertyTexture:
    {
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);

        HICON hButtonIcon(NULL);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILE_BROWSE), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton3 = new CInPlaceButton(functor(*this, &CPropertyItem::OnFileBrowseButton));
        m_cButton3->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        m_cButton3->SetIcon(CSize(16, 15), hButtonIcon);
        m_cButton3->SetBorderGap(6);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_LAYERS), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton2 = new CInPlaceButton(functor(*this, &CPropertyItem::OnTextureBrowseButton));
        m_cButton2->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 5);     // Browse textures semantics
        m_cButton2->SetIcon(CSize(16, 15), hButtonIcon);
        m_cButton2->SetBorderGap(6);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILEOPEN_BACK), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnTextureApplyButton));
        m_cButton->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 6);     // Apply selected texture semantics.
        m_cButton->SetIcon(CSize(16, 15), hButtonIcon);
        m_cButton->SetBorderGap(6);
    }
    break;


    case ePropertyAnimation:
    {
        HICON hButtonIcon(NULL);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILE_BROWSE), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton2 = new CInPlaceButton(functor(*this, &CPropertyItem::OnAnimationBrowseButton));
        m_cButton2->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 5);         // Browse textures semantics
        m_cButton2->SetIcon(CSize(16, 15), hButtonIcon);
        m_cButton2->SetBorderGap(6);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILEOPEN_BACK), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAnimationApplyButton));
        m_cButton->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 6);         // Apply selected texture semantics.
        m_cButton->SetIcon(CSize(16, 15), hButtonIcon);
        m_cButton->SetBorderGap(6);
    }
    break;

    case ePropertyFlare:
    {
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);

        m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnFlareDatabaseButton));
        m_cButton->Create("D", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
    }
    break;

    case ePropertyModel:
    case ePropertyGeomCache:
    case ePropertyAudioTrigger:
    case ePropertyAudioSwitch:
    case ePropertyAudioSwitchState:
    case ePropertyAudioRTPC:
    case ePropertyAudioEnvironment:
    case ePropertyAudioPreloadRequest:
    {
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);

        m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnResourceSelectorButton));
        m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);

        // Use file browse icon.
        HICON hOpenIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILE_BROWSE), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton->SetIcon(CSize(16, 15), hOpenIcon);
        m_cButton->SetBorderGap(0);
    }
    break;
    case ePropertyFile:
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);

        m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnFileBrowseButton));
        m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);

        // Use file browse icon.
        HICON hOpenIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILE_BROWSE), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        //m_cButton->SetIcon( CSize(16,15),IDI_FILE_BROWSE );
        m_cButton->SetIcon(CSize(16, 15), hOpenIcon);
        m_cButton->SetBorderGap(0);
        break;
        /*
                case ePropertyList:
                    {
                        AddButton( "Add", CWDelegate(this,(TDelegate)OnAddItemButton) );
                    }
                    break;
        */
    }

    MoveInPlaceControl(ctrlRect);
    SendToControl();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::CreateControls(CWnd* pWndParent, CRect& textRect, CRect& ctrlRect)
{
    m_pControlsHostWnd = pWndParent;
    m_rcText = textRect;
    m_rcControl = ctrlRect;

    m_bMoveControls = false;
    CRect nullRc(0, 0, 0, 0);
    DWORD style;

    if (IsExpandable() && !m_propertyCtrl->IsCategory(this))
    {
        m_cExpandButton  = new CInPlaceButton(functor(*this, &CPropertyItem::OnExpandButton));
        m_cExpandButton->Create("", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,  CRect(textRect.left, textRect.top + 2, textRect.left + 12, textRect.top + 14), pWndParent, 5);
        m_cExpandButton->SetFlatStyle(TRUE);
        if (!IsExpanded())
        {
            m_cExpandButton->SetIcon(CSize(8, 8), IDI_EXPAND1);
        }
        else
        {
            m_cExpandButton->SetIcon(CSize(8, 8), IDI_EXPAND2);
        }
        textRect.left += 12;
        RegisterCtrl(m_cExpandButton);
    }

    // Create static text. Update: put text on left of Bools as well, for UI consistency.
    // if (m_type != ePropertyBool)
    {
        m_pStaticText = new CColorCtrl<CStatic>;
        CString text = GetName();
        if (m_type != ePropertyTable)
        {
            text += " . . . . . . . . . . . . . . . . . . . . . . . . . .";
        }
        m_pStaticText->Create(text, WS_CHILD | WS_VISIBLE | SS_LEFT, textRect, pWndParent);
        m_pStaticText->SetFont(CFont::FromHandle((HFONT)gSettings.gui.hSystemFont));
        RegisterCtrl(m_pStaticText);
    }

    switch (m_type)
    {
    case ePropertyBool:
    {
        m_cCheckBox = new CInPlaceCheckBox(functor(*this, &CPropertyItem::OnCheckBoxButton));
        m_cCheckBox->Create("", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, ctrlRect, pWndParent, 0);
        m_cCheckBox->SetFont(m_propertyCtrl->GetFont());
        RegisterCtrl(m_cCheckBox);
    }
    break;
    case ePropertyFloat:
    case ePropertyInt:
    case ePropertyAngle:
    {
        m_cNumber = new CNumberCtrl;
        m_cNumber->SetUpdateCallback(functor(*this, &CPropertyItem::OnNumberCtrlUpdate));
        m_cNumber->EnableUndo(m_name + " Modified");
        RegisterCtrl(m_cNumber);

        // (digits behind the comma, only used for floats)
        if (m_type == ePropertyFloat)
        {
            m_cNumber->SetStep(m_step);
        }

        // Only for integers.
        if (m_type == ePropertyInt)
        {
            m_cNumber->SetInteger(true);
        }
        m_cNumber->SetMultiplier(m_valueMultiplier);
        m_cNumber->SetRange(m_bHardMin ? m_rangeMin : -FLT_MAX, m_bHardMax ? m_rangeMax : FLT_MAX);

        m_cNumber->Create(pWndParent, nullRc, 1);
        m_cNumber->SetLeftAlign(true);

        /*
        CSliderCtrlEx *pSlider = new CSliderCtrlEx;
        //pSlider->EnableUndo( m_name + " Modified" );
        CRect rcSlider = ctrlRect;
        rcSlider.left += NUMBER_CTRL_WIDTH;
        pSlider->Create( WS_VISIBLE|WS_CHILD,rcSlider,pWndParent,2);
        */
        m_cFillSlider = new CFillSliderCtrl;
        m_cFillSlider->SetFilledLook(true);
        //m_cFillSlider->SetFillStyle(CFillSliderCtrl::eFillStyle_Background);
        m_cFillSlider->EnableUndo(m_name + " Modified");
        m_cFillSlider->Create(WS_VISIBLE | WS_CHILD, nullRc, pWndParent, 2);
        m_cFillSlider->SetUpdateCallback(functor(*this, &CPropertyItem::OnFillSliderCtrlUpdate));
        m_cFillSlider->SetRangeFloat(m_rangeMin / m_valueMultiplier, m_rangeMax / m_valueMultiplier, m_step / m_valueMultiplier);
        RegisterCtrl(m_cFillSlider);
    }
    break;

    case ePropertyVector:
    case ePropertyVector2:
    case ePropertyVector4:
    {
        int numberOfElements = 3;
        if (m_type == ePropertyVector2)
        {
            numberOfElements = 2;
        }
        else if (m_type == ePropertyVector4)
        {
            numberOfElements = 4;
        }

        CNumberCtrl** cNumber[4] = { &m_cNumber, &m_cNumber1, &m_cNumber2, &m_cNumber3 };

        for (int a = 0; a < numberOfElements; a++)
        {
            CNumberCtrl* pNumber = *cNumber[a] = new CNumberCtrl;
            pNumber->Create(pWndParent, nullRc, 1);
            pNumber->SetLeftAlign(true);
            pNumber->SetUpdateCallback(functor(*this, &CPropertyItem::OnNumberCtrlUpdate));
            pNumber->EnableUndo(m_name + " Modified");
            pNumber->SetMultiplier(m_valueMultiplier);
            pNumber->SetRange(m_bHardMin ? m_rangeMin : -FLT_MAX, m_bHardMax ? m_rangeMax : FLT_MAX);
            RegisterCtrl(pNumber);
        }
    }
    break;

    case ePropertyTable:
        if (!m_bEditChildren)
        {
            break;
        }
    case ePropertyString:
        //case ePropertyVector:
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
        RegisterCtrl(m_cEdit);
        break;

    case ePropertyShader:
    case ePropertyMaterial:
    case ePropertyAiBehavior:
    case ePropertyAiAnchor:
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
    case ePropertyAiCharacter:
#endif
    case ePropertyAiPFPropertiesList:
    case ePropertyAiEntityClasses:
    case ePropertyEquip:
    case ePropertyReverbPreset:
    case ePropertySOClass:
    case ePropertySOClasses:
    case ePropertySOState:
    case ePropertySOStates:
    case ePropertySOStatePattern:
    case ePropertySOAction:
    case ePropertySOHelper:
    case ePropertySONavHelper:
    case ePropertySOAnimHelper:
    case ePropertyCustomAction:
    case ePropertyGameToken:
    case ePropertySequence:
    case ePropertySequenceId:
    case ePropertyUser:
    case ePropertyLocalString:
    case ePropertyLightAnimation:
    case ePropertyUiElement:
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        style = WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP;
        if (m_type == ePropertySequenceId)
        {
            style |= ES_READONLY;
        }
        m_cEdit->Create(style, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
        RegisterCtrl(m_cEdit);

        if (m_type == ePropertyShader)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnShaderBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyAiBehavior)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAIBehaviorBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyAiAnchor)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAIAnchorBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
        if (m_type == ePropertyAiCharacter)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAICharacterBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
#endif
        if (m_type == ePropertyAiPFPropertiesList)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAIPFPropertiesListBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyAiEntityClasses)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAIEntityClassesBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyEquip)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnEquipBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        if (m_type == ePropertyReverbPreset)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnEAXPresetBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyMaterial)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnMaterialBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);

            m_cButton2 = new CInPlaceButton(functor(*this, &CPropertyItem::OnMaterialPickSelectedButton));
            m_cButton2->Create("<", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 5);
        }
        else if (m_type == ePropertySOClass)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOClassBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOClasses)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOClassesBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOState)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOStateBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOStates)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOStatesBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOStatePattern)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOStatePatternBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOAction)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOActionBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOHelper)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOHelperBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySONavHelper)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSONavHelperBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySOAnimHelper)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSOAnimHelperBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyCustomAction)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnCustomActionBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyGameToken)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnGameTokenBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySequence)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSequenceBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertySequenceId)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnSequenceIdBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyUser)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnUserBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyLocalString)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnLocalStringBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyLightAnimation)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnLightAnimationBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        }
        else if (m_type == ePropertyUiElement)
        {
            m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnMaterialBrowseButton));
            m_cButton->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);

            m_cButton2 = new CInPlaceButton(functor(*this, &CPropertyItem::OnMaterialPickSelectedButton));
            m_cButton2->Create("<", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 5);
        }

        if (m_cButton)
        {
            RegisterCtrl(m_cButton);
        }
        if (m_cButton2)
        {
            RegisterCtrl(m_cButton2);
        }
        break;

    case ePropertyAiTerritory:
    case ePropertyAiWave:
    case ePropertySelection:
    {
        if (m_type == ePropertyAiTerritory)
        {
            PopulateAITerritoriesList();
        }

        if (m_type == ePropertyAiWave)
        {
            PopulateAIWavesList();
        }

        // Combo box.
        CreateInPlaceComboBox(pWndParent, WS_CHILD | WS_VISIBLE | WS_TABSTOP);
        RegisterCtrl(m_cCombo);
        PopulateInPlaceComboBoxFromEnumList();
    }
    break;

    case ePropertyColor:
    {
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
        RegisterCtrl(m_cEdit);

        //m_cButton = new CInPlaceButton( functor(*this,OnColorBrowseButton) );
        //m_cButton->Create( "",WS_CHILD|WS_VISIBLE,nullRc,pWndParent,4 );
        m_cColorButton = new CInPlaceColorButton(functor(*this, &CPropertyItem::OnColorBrowseButton));
        m_cColorButton->Create("", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW, nullRc, pWndParent, 4);
        RegisterCtrl(m_cColorButton);
    }
    break;

    case ePropertyFloatCurve:
    {
        m_cSpline = new CWidgetWrapper<CSplineCtrl>(pWndParent);
        m_cSpline->widget()->show();
        m_cSpline->widget()->SetUpdateCallback((CSplineCtrl::UpdateCallback const&)functor(*this, &CPropertyItem::ReceiveFromControl));
        m_cSpline->widget()->SetTimeRange(0, 1);
        m_cSpline->widget()->SetValueRange(0, 1);
        m_cSpline->widget()->SetGrid(12, 12);
        m_cSpline->widget()->SetSpline(m_pVariable->GetSpline(true));
        RegisterCtrl(m_cSpline);
    }
    break;

    case ePropertyColorCurve:
    {
        m_cColorSpline = new CColorGradientCtrlWrapper(pWndParent);
        m_cColorSpline->show();
        m_cColorSpline->m_cColorSpline->SetUpdateCallback((CColorGradientCtrl::UpdateCallback const&)functor(*this, &CPropertyItem::ReceiveFromControl));
        m_cColorSpline->m_cColorSpline->SetTimeRange(0, 1);
        m_cColorSpline->m_cColorSpline->SetSpline(m_pVariable->GetSpline(true));
#ifdef KDAB_PORT_TODO
        //enable this once all controls are ported to Qt
        RegisterCtrl(m_cColorSpline);
#endif
    }
    break;


    case ePropertyTexture:
    {
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
        RegisterCtrl(m_cEdit);

        HICON hButtonIcon(NULL);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILE_BROWSE), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton3 = new CInPlaceButton(functor(*this, &CPropertyItem::OnFileBrowseButton));
        m_cButton3->Create("...", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        RegisterCtrl(m_cButton3);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_LAYERS), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton2 = new CInPlaceButton(functor(*this, &CPropertyItem::OnTextureBrowseButton));
        m_cButton2->Create("A", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 5); // Browse textures semantics
        m_cButton2->SetTooltip(CString(MAKEINTRESOURCE(IDS_SELECTASSETBROWSERNEW_PROPERTYITEM_TOOLTIP)));
        RegisterCtrl(m_cButton2);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILEOPEN_BACK), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnTextureApplyButton));
        m_cButton->Create("<", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 6); // Apply selected texture semantics.
        m_cButton->SetTooltip(CString(MAKEINTRESOURCE(IDS_SELECTASSETBROWSEREXISTING_PROPERTYITEM_TOOLTIP)));
        RegisterCtrl(m_cButton);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TIFF), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton4 = new CInPlaceButton(functor(*this, &CPropertyItem::OnTextureEditButton));
        m_cButton4->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 7); // edit selected texture semantics.
        m_cButton4->SetIcon(CSize(16, 15), hButtonIcon);
        m_cButton4->SetBorderGap(6);
        RegisterCtrl(m_cButton4);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PSD), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton5 = new CInPlaceButton(functor(*this, &CPropertyItem::OnPsdEditButton));
        m_cButton5->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 8); // edit selected texture semantics.
        m_cButton5->SetIcon(CSize(16, 15), hButtonIcon);
        m_cButton5->SetBorderGap(6);
        RegisterCtrl(m_cButton5);
    }
    break;

    case ePropertyAnimation:
    {
        HICON hButtonIcon(NULL);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILE_BROWSE), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton2 = new CInPlaceButton(functor(*this, &CPropertyItem::OnAnimationBrowseButton));
        m_cButton2->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 5); // Browse textures semantics
        m_cButton2->SetIcon(CSize(16, 15), hButtonIcon);
        m_cButton2->SetBorderGap(6);
        RegisterCtrl(m_cButton2);

        hButtonIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILEOPEN_BACK), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnAnimationApplyButton));
        m_cButton->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 6); // Apply selected texture semantics.
        m_cButton->SetIcon(CSize(16, 15), hButtonIcon);
        m_cButton->SetBorderGap(6);
        RegisterCtrl(m_cButton);
    }
    break;

    case ePropertyModel:
    case ePropertyAudioTrigger:
    case ePropertyAudioSwitch:
    case ePropertyAudioSwitchState:
    case ePropertyAudioRTPC:
    case ePropertyAudioEnvironment:
    case ePropertyAudioPreloadRequest:
    {
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
        RegisterCtrl(m_cEdit);

        m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnResourceSelectorButton));
        m_cButton->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        RegisterCtrl(m_cButton);

        HICON hOpenIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILE_BROWSE), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton->SetIcon(CSize(16, 15), hOpenIcon);
        m_cButton->SetBorderGap(6);
    }
    break;
    case ePropertyFile:
        m_cEdit = new CInPlaceEdit(m_value, functor(*this, &CPropertyItem::OnEditChanged));
        m_cEdit->Create(WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT | WS_TABSTOP, nullRc, pWndParent, 2);
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
        RegisterCtrl(m_cEdit);

        m_cButton = new CInPlaceButton(functor(*this, &CPropertyItem::OnFileBrowseButton));
        m_cButton->Create("", WS_CHILD | WS_VISIBLE, nullRc, pWndParent, 4);
        RegisterCtrl(m_cButton);

        // Use file browse icon.
        HICON hOpenIcon = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FILE_BROWSE), IMAGE_ICON, 16, 15, LR_DEFAULTCOLOR | LR_SHARED);
        m_cButton->SetIcon(CSize(16, 15), hOpenIcon);
        m_cButton->SetBorderGap(6);
        break;
    }

    if (m_cEdit)
    {
        m_cEdit->ModifyStyleEx(0, WS_EX_CLIENTEDGE);
    }
    if (m_cCombo)
    {
        //m_cCombo->ModifyStyleEx( 0,WS_EX_CLIENTEDGE );
    }

    MoveInPlaceControl(ctrlRect);
    SendToControl();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::DestroyInPlaceControl(bool bRecursive)
{
    if (!m_propertyCtrl->IsExtenedUI() && !m_bForceModified)
    {
        ReceiveFromControl();
    }

    SAFE_DELETE(m_pStaticText);
    SAFE_DELETE(m_cNumber);
    SAFE_DELETE(m_cNumber1);
    SAFE_DELETE(m_cNumber2);
    SAFE_DELETE(m_cNumber3);
    SAFE_DELETE(m_cFillSlider);
    SAFE_DELETE(m_cSpline);
    SAFE_DELETE(m_cColorSpline);
    SAFE_DELETE(m_cEdit);
    SAFE_DELETE(m_cCombo);
    SAFE_DELETE(m_cButton);
    SAFE_DELETE(m_cButton2);
    SAFE_DELETE(m_cButton3);
    SAFE_DELETE(m_cButton4);
    SAFE_DELETE(m_cButton5);
    SAFE_DELETE(m_cExpandButton);
    SAFE_DELETE(m_cCheckBox);
    SAFE_DELETE(m_cColorButton);

    if (bRecursive)
    {
        int num = GetChildCount();
        for (int i = 0; i < num; i++)
        {
            GetChild(i)->DestroyInPlaceControl(bRecursive);
        }
        m_controls.clear();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::MoveInPlaceControl(const CRect& rect)
{
    m_rcControl = rect;

    int nButtonWidth = BUTTON_WIDTH;
    if (m_propertyCtrl->IsExtenedUI())
    {
        nButtonWidth += 10;
    }

    CRect rc = rect;

    if (m_cButton)
    {
        if (m_type == ePropertyColor)
        {
            rc.right = rc.left + nButtonWidth + 2;
            RepositionWindow(m_cButton, rc);
            rc = rect;
            rc.left += nButtonWidth + 2 + 4;
            if (rc.right > rc.left - 100)
            {
                rc.right = rc.left + 100;
            }
        }
        else
        {
            rc.left = rc.right - nButtonWidth;
            RepositionWindow(m_cButton, rc);
            rc = rect;
            rc.right -= nButtonWidth;
        }
    }
    if (m_cColorButton)
    {
        rc.right = rc.left + nButtonWidth + 2;
        RepositionWindow(m_cColorButton, rc);
        rc = rect;
        rc.left += nButtonWidth + 2 + 4;
        if (rc.right > rc.left - 80)
        {
            rc.right = rc.left + 80;
        }
    }
    if (m_cButton4)
    {
        CRect brc(rc);
        brc.left = brc.right - nButtonWidth - 2;
        m_cButton4->MoveWindow(brc, FALSE);
        rc.right -= nButtonWidth + 4;
        m_cButton4->SetFont(m_propertyCtrl->GetFont());
    }
    if (m_cButton5)
    {
        CRect brc(rc);
        brc.left = brc.right - nButtonWidth - 2;
        m_cButton5->MoveWindow(brc, FALSE);
        rc.right -= nButtonWidth + 4;
        m_cButton5->SetFont(m_propertyCtrl->GetFont());
    }
    if (m_cButton2)
    {
        CRect brc(rc);
        brc.left = brc.right - nButtonWidth - 2;
        RepositionWindow(m_cButton2, brc);
        rc.right -= nButtonWidth + 4;
    }
    if (m_cButton3)
    {
        CRect brc(rc);
        brc.left = brc.right - nButtonWidth - 2;
        RepositionWindow(m_cButton3, brc);
        rc.right -= nButtonWidth + 4;
    }

    if (m_cNumber)
    {
        CRect rcn = rc;
        if (m_cNumber1 && m_cNumber2 && m_cNumber3)
        {
            int x = NUMBER_CTRL_WIDTH + 4;
            CRect rc0, rc1, rc2, rc3;
            rc0 = CRect(rc.left, rc.top, rc.left + NUMBER_CTRL_WIDTH, rc.bottom);
            rc1 = CRect(rc.left + x, rc.top, rc.left + x + NUMBER_CTRL_WIDTH, rc.bottom);
            rc2 = CRect(rc.left + x * 2, rc.top, rc.left + x * 2 + NUMBER_CTRL_WIDTH, rc.bottom);
            rc3 = CRect(rc.left + x * 3, rc.top, rc.left + x * 3 + NUMBER_CTRL_WIDTH, rc.bottom);
            RepositionWindow(m_cNumber, rc0);
            RepositionWindow(m_cNumber1, rc1);
            RepositionWindow(m_cNumber2, rc2);
            RepositionWindow(m_cNumber3, rc3);
        }
        else if (m_cNumber1 && m_cNumber2)
        {
            int x = NUMBER_CTRL_WIDTH + 4;
            CRect rc0, rc1, rc2;
            rc0 = CRect(rc.left, rc.top, rc.left + NUMBER_CTRL_WIDTH, rc.bottom);
            rc1 = CRect(rc.left + x, rc.top, rc.left + x + NUMBER_CTRL_WIDTH, rc.bottom);
            rc2 = CRect(rc.left + x * 2, rc.top, rc.left + x * 2 + NUMBER_CTRL_WIDTH, rc.bottom);
            RepositionWindow(m_cNumber, rc0);
            RepositionWindow(m_cNumber1, rc1);
            RepositionWindow(m_cNumber2, rc2);
        }
        else if (m_cNumber1)
        {
            int x = NUMBER_CTRL_WIDTH + 4;
            CRect rc0, rc1;
            rc0 = CRect(rc.left, rc.top, rc.left + NUMBER_CTRL_WIDTH, rc.bottom);
            rc1 = CRect(rc.left + x, rc.top, rc.left + x + NUMBER_CTRL_WIDTH, rc.bottom);
            RepositionWindow(m_cNumber, rc0);
            RepositionWindow(m_cNumber1, rc1);
        }
        else
        {
            //rcn.right = rc.left + NUMBER_CTRL_WIDTH;
            if (rcn.Width() > NUMBER_CTRL_WIDTH)
            {
                rcn.right = rc.left + NUMBER_CTRL_WIDTH;
            }
            RepositionWindow(m_cNumber, rcn);
        }
        if (m_cFillSlider)
        {
            CRect rcSlider = rc;
            rcSlider.left = rcn.right + 1;
            RepositionWindow(m_cFillSlider, rcSlider);
        }
    }
    if (m_cEdit)
    {
        RepositionWindow(m_cEdit, rc);
    }
    if (m_cCombo)
    {
        RepositionWindow(m_cCombo, rc);
    }
    if (m_cSpline)
    {
        // Grow beyond current line.
        rc.bottom = rc.top + 48;
        rc.right -= 4;
        RepositionWindow(m_cSpline, rc);
    }
    if (m_cColorSpline)
    {
        // Grow beyond current line.
        rc.bottom = rc.top + 32;
        rc.right -= 4;
        m_cColorSpline->setGeometry(rc.left, rc.top, rc.Width(), rc.Height());
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SetFocus()
{
    if (m_cNumber)
    {
        m_cNumber->SetFocus();
    }
    if (m_cEdit)
    {
        m_cEdit->SetFocus();
    }
    if (m_cCombo)
    {
        m_cCombo->SetFocus();
    }
    if (!m_cNumber && !m_cEdit && !m_cCombo)
    {
        m_propertyCtrl->SetFocus();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::ReceiveFromControl()
{
    if (IsDisabled())
    {
        return;
    }

    m_bDontSendToControl = false;

    if (m_cEdit)
    {
        CString str;
        m_cEdit->GetWindowText(str);
        SetValue(str);
    }
    if (m_cCombo)
    {
        if (!CUndo::IsRecording())
        {
            if (m_pEnumDBItem)
            {
                SetValue(m_pEnumDBItem->NameToValue(m_cCombo->GetSelectedString()));
            }
            else
            {
                SetValue(m_cCombo->GetSelectedString());
            }
        }
        else
        {
            if (m_pEnumDBItem)
            {
                SetValue(m_pEnumDBItem->NameToValue(m_cCombo->GetSelectedString()));
            }
            else
            {
                SetValue(m_cCombo->GetSelectedString());
            }
        }
    }
    if (m_cNumber)
    {
        if (m_cNumber1 && m_cNumber2 && m_cNumber3)
        {
            CString val;
            val.Format("%g,%g,%g,%g", m_cNumber->GetValue(), m_cNumber1->GetValue(), m_cNumber2->GetValue(), m_cNumber3->GetValue());
            SetValue(val);
        }
        if (m_cNumber1 && m_cNumber2)
        {
            CString val;
            val.Format("%g,%g,%g", m_cNumber->GetValue(), m_cNumber1->GetValue(), m_cNumber2->GetValue());
            SetValue(val);
        }
        else if (m_cNumber1)
        {
            CString val;
            val.Format("%g,%g", m_cNumber->GetValue(), m_cNumber1->GetValue());
            SetValue(val);
        }
        else
        {
            SetValue(m_cNumber->GetValueAsString());
        }
    }
    if (m_cSpline != 0 || m_cColorSpline != 0)
    {
        // Splines update variables directly so don't call OnVariableChange or SetValue here or values will be overwritten.
        m_modified = true;
        m_pVariable->Get(m_value);

        // Call OnSetValue to force this field to notify this variable that its model has changed without going through the
        // full OnVariableChange pass
        //
        // Set m_bDontSendToControl to prevent the control's data from being overwritten (as the variable's data won't
        // necessarily be up to date vs the controls at the point this happens).
        m_bDontSendToControl = true;
        m_pVariable->OnSetValue(false);
        m_bDontSendToControl = false;

        m_propertyCtrl->OnItemChange(this);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::Touch()
{
    OnVariableChange(m_pVariable);
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SendToControl()
{
    // This occurs when a spline that already updated its own state - don't want to reset its data here
    if (m_bDontSendToControl)
    {
        return;
    }

    bool bInPlaceCtrl = m_propertyCtrl->IsExtenedUI();
    if (m_cButton)
    {
        if (m_type == ePropertyColor)
        {
            COLORREF clr = StringToColor(m_value);
            m_cButton->SetColorFace(clr);
            m_cButton->RedrawWindow();
            bInPlaceCtrl = true;
        }
        m_cButton->Invalidate();
    }
    if (m_cColorButton)
    {
        COLORREF clr = StringToColor(m_value);
        m_cColorButton->SetColor(clr);
        m_cColorButton->Invalidate();
        bInPlaceCtrl = true;
    }
    if (m_cCheckBox)
    {
        m_cCheckBox->SetChecked(GetBoolValue() ? BST_CHECKED : BST_UNCHECKED);
        m_cCheckBox->Invalidate();
    }
    if (m_cEdit)
    {
        m_cEdit->SetText(m_value);
        bInPlaceCtrl = true;
        m_cEdit->Invalidate();
    }
    if (m_cCombo)
    {
        if (m_type == ePropertyBool)
        {
            m_cCombo->SetCurSel(GetBoolValue() ? 0 : 1);
        }
        else
        {
            if (m_pEnumDBItem)
            {
                m_cCombo->SelectString(-1, m_pEnumDBItem->ValueToName(m_value));
            }
            else
            {
                m_cCombo->SelectString(-1, m_value);
            }
        }
        bInPlaceCtrl = true;
        m_cCombo->Invalidate();
    }
    if (m_cNumber)
    {
        if (m_cNumber1 && m_cNumber2 && m_cNumber3)
        {
            float x, y, z, w;
            sscanf(m_value, "%f,%f,%f,%f", &x, &y, &z, &w);
            m_cNumber->SetValue(x);
            m_cNumber1->SetValue(y);
            m_cNumber2->SetValue(z);
            m_cNumber3->SetValue(w);
        }
        else if (m_cNumber1 && m_cNumber2)
        {
            float x, y, z;
            sscanf(m_value, "%f,%f,%f", &x, &y, &z);
            m_cNumber->SetValue(x);
            m_cNumber1->SetValue(y);
            m_cNumber2->SetValue(z);
        }
        else if (m_cNumber1)
        {
            float x, y;
            sscanf(m_value, "%f,%f", &x, &y);
            m_cNumber->SetValue(x);
            m_cNumber1->SetValue(y);
        }
        else
        {
            m_cNumber->SetValue(atof(m_value));
        }
        bInPlaceCtrl = true;
        m_cNumber->Invalidate();
    }
    if (m_cFillSlider)
    {
        m_cFillSlider->SetValue(atof(m_value));
        bInPlaceCtrl = true;
        m_cFillSlider->Invalidate();
    }
    if (!bInPlaceCtrl)
    {
        CRect rc;
        m_propertyCtrl->GetItemRect(this, rc);
        m_propertyCtrl->InvalidateRect(rc, TRUE);

        CWnd* pFocusWindow = CWnd::GetFocus();
        if (pFocusWindow && m_propertyCtrl->IsChild(pFocusWindow))
        {
            m_propertyCtrl->SetFocus();
        }
    }
    if (m_cSpline)
    {
        m_cSpline->widget()->SetSpline(m_pVariable->GetSpline(true), TRUE);
    }
    if (m_cColorSpline)
    {
        m_cColorSpline->m_cColorSpline->SetSpline(m_pVariable->GetSpline(true), TRUE);
    }
    if (m_pVariable && m_pVariable->GetFlags() & IVariable::UI_HIGHLIGHT_EDITED)
    {
        CheckControlActiveColor();
    }
}

//////////////////////////////////////////////////////////////////////////
bool CPropertyItem::HasDefaultValue(bool bChildren) const
{
    if (m_pVariable && !m_pVariable->HasDefaultValue())
    {
        return false;
    }

    if (bChildren)
    {
        // Evaluate children.
        for (int i = 0; i < m_childs.size(); i++)
        {
            if (!m_childs[i]->HasDefaultValue(true))
            {
                return false;
            }
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::CheckControlActiveColor()
{
    if (m_pStaticText)
    {
        COLORREF clrStaticText = XTPColorManager()->LightColor(QtUtil::GetXtremeColorPort(COLOR_3DFACE), QtUtil::GetXtremeColorPort(COLOR_BTNTEXT), 30);
        COLORREF nTextColor = HasDefaultValue(true) ? clrStaticText : QtUtil::GetXtremeColorPort(COLOR_HIGHLIGHT);
        if (m_pStaticText->GetTextColor() != nTextColor)
        {
            m_pStaticText->SetTextColor(nTextColor);
        }
    }

    if (m_cExpandButton)
    {
        static COLORREF nDefColor = QtUtil::GetSysColorPort(COLOR_BTNFACE);
        static COLORREF nDefTextColor = QtUtil::GetSysColorPort(COLOR_BTNTEXT);
        static COLORREF nNondefColor = QtUtil::GetXtremeColorPort(COLOR_HIGHLIGHT);
        static COLORREF nNondefTextColor = QtUtil::GetXtremeColorPort(COLOR_HIGHLIGHTTEXT);
        bool bDefault = HasDefaultValue(true);
        COLORREF nColor = bDefault ? nDefColor : nNondefColor;

        if (m_cExpandButton->GetColor() != nColor)
        {
            m_cExpandButton->SetColor(nColor);
            m_cExpandButton->SetTextColor(bDefault ? nDefTextColor : nNondefTextColor);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnComboSelection()
{
    ReceiveFromControl();
    SendToControl();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::DrawValue(CDC* dc, CRect rect)
{
    // Setup text.
    dc->SetBkMode(TRANSPARENT);

    CString val = GetDrawValue();

    if (m_type == ePropertyBool)
    {
        int sz = rect.bottom - rect.top - 1;
        int borderwidth = (int)(sz * 0.13f); //white frame border
        int borderoffset = (int)(sz * 0.1f); //offset from the frameborder
        CPoint p1(rect.left, rect.top + 1); // box
        CPoint p2(rect.left + borderwidth, rect.top + 1  + borderwidth); //check-mark

        const bool bPropertyCtrlIsDisabled = m_propertyCtrl->IsReadOnly() || m_propertyCtrl->IsGrayed() || (m_propertyCtrl->IsWindowEnabled() != TRUE);
        const bool bShowInactiveFrame = IsDisabled() || bPropertyCtrlIsDisabled;

        CRect rc(p1.x, p1.y, p1.x + sz, p1.y + sz);
        dc->DrawFrameControl(rc, DFC_BUTTON, DFCS_BUTTON3STATE | (bShowInactiveFrame ? DFCS_INACTIVE : 0));
        if (GetBoolValue())
        {
            COLORREF color = StringToColor(val);
            if (bShowInactiveFrame)
            {
                color = (COLORREF)ColorB::ComputeAvgCol_Fast(RGB(255, 255, 255), color);
            }
            CPen pen(PS_SOLID, 1, color);
            CPen* pPen = dc->SelectObject(&pen);

            //Draw bold check-mark
            int number_of_lines = sz / 8;
            number_of_lines += 1;
            for (int i = 0; i < number_of_lines; i++)
            {
                CPoint start = p2 + CPoint(i, 0);
                start.y += ((sz - borderwidth * 2) / 2);
                start.x += borderoffset;

                CPoint middle = p2 + CPoint(i, 0);
                middle.y += sz - borderwidth * 2;
                middle.x += (sz - borderwidth - number_of_lines) / 2;
                middle.y -= borderoffset;

                CPoint end = p2 + CPoint(i, 0);
                end.x += sz - borderwidth * 2 - number_of_lines;
                end.x -= borderoffset;
                end.y += borderoffset;

                dc->MoveTo(start);
                dc->LineTo(middle);
                dc->LineTo(end);
            }
        }

        //offset text with scaling
        rect.left += (int)(sz * 1.25f);
    }
    else if (m_type == ePropertyFile || m_type == ePropertyTexture || m_type == ePropertyModel)
    {
        // Any file.
        // Check if file name fits into the designated rectangle.
        CSize textSize = dc->GetTextExtent(val, val.GetLength());
        if (textSize.cx > rect.Width())
        {
            // Cut file name...
            if (Path::GetExt(val).IsEmpty() == false)
            {
                val = CString("...\\") + Path::GetFile(val);
            }
        }
    }
    else if (m_type == ePropertyColor)
    {
        //CRect rc( CPoint(rect.right-BUTTON_WIDTH,rect.top),CSize(BUTTON_WIDTH,rect.bottom-rect.top) );
        CRect rc(CPoint(rect.left, rect.top + 1), CSize(BUTTON_WIDTH + 2, rect.bottom - rect.top - 2));
        //CPen pen( PS_SOLID,1,RGB(128,128,128));
        CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
        COLORREF color = StringToColor(val);
        CBrush brush(color & 0xFFFFFF);
        CPen* pOldPen = dc->SelectObject(&pen);
        CBrush* pOldBrush = dc->SelectObject(&brush);
        dc->Rectangle(rc);
        //COLORREF col = StringToColor(m_value);
        //rc.DeflateRect( 1,1 );
        //dc->FillSolidRect( rc,col );
        dc->SelectObject(pOldPen);
        dc->SelectObject(pOldBrush);
        rect.left = rect.left + BUTTON_WIDTH + 2 + 4;
    }
    else if (m_pVariable != 0 && m_pVariable->GetSpline(false) && m_pVariable->GetSpline(false)->GetKeyCount() > 0)
    {
        // Draw mini-curve or gradient.
        CPen* pOldPen = 0;

        ISplineInterpolator* pSpline = m_pVariable->GetSpline(true);
        int width = min(rect.Width() - 1, 128);
        for (int x = 0; x < width; x++)
        {
            float time = float(x) / (width - 1);
            ISplineInterpolator::ValueType val;
            pSpline->Interpolate(time, val);

            if (m_type == ePropertyColorCurve)
            {
                COLORREF col = RGB(pos_round(val[0] * 255), pos_round(val[1] * 255), pos_round(val[2] * 255));
                CPen pen(PS_SOLID, 1, col);
                if (!pOldPen)
                {
                    pOldPen = dc->SelectObject(&pen);
                }
                else
                {
                    dc->SelectObject(&pen);
                }
                dc->MoveTo(CPoint(rect.left + x, rect.bottom));
                dc->LineTo(CPoint(rect.left + x, rect.top));
            }
            else if (m_type == ePropertyFloatCurve)
            {
                CPoint point;
                point.x = rect.left + x;
                point.y = int_round((rect.bottom - 1) * (1.f - val[0]) + (rect.top + 1) * val[0]);
                if (x == 0)
                {
                    dc->MoveTo(point);
                }
                else
                {
                    dc->LineTo(point);
                }
            }

            if (pOldPen)
            {
                dc->SelectObject(pOldPen);
            }
        }

        // No text.
        return;
    }

    /*
    //////////////////////////////////////////////////////////////////////////
    // Draw filled bar like in CFillSliderCtrl.
    //////////////////////////////////////////////////////////////////////////
    if (m_type == ePropertyFloat || m_type == ePropertyInt || m_type == ePropertyAngle)
    {
        CRect rc = rect;
        rc.left += NUMBER_CTRL_WIDTH;
        rc.top += 1;
        rc.bottom -= 1;

        float value = atof(m_value);
        float min = m_rangeMin/m_valueMultiplier;
        float max = m_rangeMax/m_valueMultiplier;
        if (min == max)
            max = min + 1;
        float pos = (value-min) / fabs(max-min);
        int splitPos = rc.left + pos * rc.Width();

        // Paint filled rect.
        CRect fillRc = rc;
        fillRc.right = splitPos;
        dc->FillRect(fillRc,CBrush::FromHandle((HBRUSH)GetStockObject(LTGRAY_BRUSH)) );

        // Paint empty rect.
        CRect emptyRc = rc;
        emptyRc.left = splitPos+1;
        emptyRc.IntersectRect(emptyRc,rc);
        dc->FillRect(emptyRc,CBrush::FromHandle((HBRUSH)GetStockObject(WHITE_BRUSH)) );
    }
    */

    CRect textRc;
    textRc = rect;
    ::DrawTextEx(dc->GetSafeHdc(), val.GetBuffer(), val.GetLength(), textRc, DT_END_ELLIPSIS | DT_LEFT | DT_SINGLELINE | DT_VCENTER, NULL);
}

COLORREF CPropertyItem::StringToColor(const CString &value)
{
    float r, g, b, a;
    int res = 0;
    res = sscanf(value, "%f,%f,%f,%f", &r, &g, &b, &a);
    if (res == 3)
    {
        a = 255;
    }

    if (res < 3)
    {
        res = sscanf(value, "R:%f,G:%f,B:%f,A:%f", &r, &g, &b, &a);
        if (res == 3)
        {
            a = 255;
        }
    }
    if (res < 3)
    {
        res = sscanf(value, "R:%f G:%f B:%f A:%f", &r, &g, &b, &a);
        if (res == 3)
        {
            a = 255;
        }
    }
    if (res < 3)
    {
        res = sscanf(value, "%f %f %f %f", &r, &g, &b, &a);
        if (res == 3)
        {
            a = 255;
        }
    }
    if (res != 3)
    {
        sscanf(value, "%f", &r);
        return r;
    }
    int ir = r;
    int ig = g;
    int ib = b;
    int ia = a;

    return RGBA(ir, ig, ib, ia);
}

bool CPropertyItem::GetBoolValue()
{
    if (_stricmp(m_value, "true") == 0 || atoi(m_value) != 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//////////////////////////////////////////////////////////////////////////
const char* CPropertyItem::GetValue() const
{
    return m_value;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::SetValue(const char* sValue, bool bRecordUndo, bool bForceModified)
{
    if (bRecordUndo && IsDisabled())
    {
        return;
    }

    _smart_ptr<CPropertyItem> holder = this; // Make sure we are not released during this function.

    CString value = sValue;

    switch (m_type)
    {
    case ePropertyAiTerritory:
#ifdef USE_SIMPLIFIED_AI_TERRITORY_SHAPE
        if ((value == "<None>") || m_value.Compare(value))
#else
        if ((value == "<Auto>") || (value == "<None>") || m_value.Compare(value))
#endif
        {
            CPropertyItem* pPropertyItem = GetParent()->FindItemByFullName("::AITerritoryAndWave::Wave");
            if (pPropertyItem)
            {
                pPropertyItem->SetValue("<None>");
            }
        }
        break;

    case ePropertyBool:
        if (_stricmp(value, "true") == 0 || atof(value) != 0)
        {
            value = "1";
        }
        else
        {
            value = "0";
        }
        break;

    case ePropertyVector2:
        if (value.Find(',') < 0)
        {
            value = value + ", " + value;
        }
        break;

    case ePropertyVector4:
        if (value.Find(',') < 0)
        {
            value = value + ", " + value + ", " + value + ", " + value;
        }
        break;

    case ePropertyVector:
        if (value.Find(',') < 0)
        {
            value = value + ", " + value + ", " + value;
        }
        break;

    case ePropertyTexture:
    case ePropertyModel:
    case ePropertyMaterial:
        value.Replace('\\', '/');
        break;
    }

    // correct the length of value
    switch (m_type)
    {
    case ePropertyTexture:
    case ePropertyModel:
    case ePropertyMaterial:
    case ePropertyFile:
        if (value.GetLength() >= MAX_PATH)
        {
            value = value.Left(MAX_PATH);
        }
        break;
    }


    bool bModified = m_bForceModified || bForceModified || m_value.Compare(value) != 0;
    bool bStoreUndo = (m_value.Compare(value) != 0 || bForceModified) && bRecordUndo;

    std::unique_ptr<CUndo> undo;
    if (bStoreUndo && !CUndo::IsRecording())
    {
        if (!m_propertyCtrl->CallUndoFunc(this))
        {
            undo.reset(new CUndo(GetName() + " Modified"));
        }
    }

    m_value = value;

    if (m_pVariable)
    {
        if (bModified)
        {
            if (m_propertyCtrl->IsStoreUndoByItems() && bStoreUndo && CUndo::IsRecording())
            {
                CUndo::Record(new CUndoVariableChange(m_pVariable, "PropertyChange"));
            }

            if (bForceModified)
            {
                m_pVariable->SetForceModified(true);
            }
            ValueToVar();
        }
    }
    else
    {
        //////////////////////////////////////////////////////////////////////////
        // DEPRICATED (For XmlNode).
        //////////////////////////////////////////////////////////////////////////
        if (m_node)
        {
            m_node->setAttr(VALUE_ATTR, m_value);
        }
        //CString xml = m_node->getXML();

        SendToControl();

        if (bModified)
        {
            m_modified = true;
            if (m_bEditChildren)
            {
                // Extract child components.
                int pos = 0;
                for (int i = 0; i < m_childs.size(); i++)
                {
                    CString elem = m_value.Tokenize(", ", pos);
                    m_childs[i]->m_value = elem;
                    m_childs[i]->SendToControl();
                }
            }

            if (m_parent)
            {
                m_parent->OnChildChanged(this);
            }
            // If Value changed mark document modified.
            // Notify parent that this Item have been modified.
            m_propertyCtrl->OnItemChange(this);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::VarToValue()
{
    assert(m_pVariable != 0);

    if (m_type == ePropertyColor)
    {
        if (m_pVariable->GetType() == IVariable::VECTOR)
        {
            Vec3 v(0, 0, 0);
            m_pVariable->Get(v);
            COLORREF col = ColorLinearToGamma(ColorF(v.x, v.y, v.z));
            m_value.Format("%d,%d,%d", GetRValue(col), GetGValue(col), GetBValue(col));
        }
        else if (m_pVariable->GetType() == IVariable::VECTOR4)
        {
            Vec4 v(0, 0, 0, 0);
            m_pVariable->Get(v);
            COLORREF col = ColorLinearToGamma(ColorF(v.x, v.y, v.z, v.w));
            m_value.Format("%d,%d,%d,%d", GetRValue(col), GetGValue(col), GetBValue(col), GetAValue(col));
        }
        else
        {
            int col(0);
            m_pVariable->Get(col);
            m_value.Format("%d,%d,%d", GetRValue((uint32)col), GetGValue((uint32)col), GetBValue((uint32)col));
        }
        return;
    }

    if (m_type == ePropertyFloat)
    {
        float value;
        m_pVariable->Get(value);

        FormatFloatForUI(m_value, FLOAT_NUM_DIGITS, value);
    }
    else
    {
        m_value = m_pVariable->GetDisplayValue();
    }

    if (m_type == ePropertySOHelper || m_type == ePropertySONavHelper || m_type == ePropertySOAnimHelper)
    {
        // hide smart object class part
        int f = m_value.Find(':');
        if (f >= 0)
        {
            m_value.Delete(0, f + 1);
        }
    }
}

CString CPropertyItem::GetDrawValue()
{
    CString value = m_value;

    if (m_pEnumDBItem)
    {
        value = m_pEnumDBItem->ValueToName(value);
    }

    if (m_valueMultiplier != 1.f)
    {
        float f = atof(m_value) * m_valueMultiplier;
        if (m_type == ePropertyInt)
        {
            value.Format("%d", int_round(f));
        }
        else
        {
            value.Format("%g", f);
        }
        if (m_valueMultiplier == 100)
        {
            value += "%";
        }
    }
    else if (m_type == ePropertyBool)
    {
        return "";
    }
    else if (m_type == ePropertyFloatCurve || m_type == ePropertyColorCurve)
    {
        // Don't display actual values in field.
        if (!value.IsEmpty())
        {
            value = "[Curve]";
        }
    }
    else if (m_type == ePropertySequenceId)
    {
        uint32 id = (uint32)atoi(value);
        IAnimSequence* pSeq = GetIEditor()->GetMovieSystem()->FindSequenceById(id);
        if (pSeq)   // Show its human-readable name instead of its ID.
        {
            return pSeq->GetName();
        }
    }

    return value;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::ValueToVar()
{
    assert(m_pVariable != NULL);

    _smart_ptr<CPropertyItem> holder = this; // Make sure we are not released during the call to variable Set.

    if (m_type == ePropertyColor)
    {
        COLORREF col = StringToColor(m_value);
        if (m_pVariable->GetType() == IVariable::VECTOR)
        {
            ColorF colLin = ColorGammaToLinear(col);
            m_pVariable->Set(Vec3(colLin.r, colLin.g, colLin.b));
        }
        else
        {
            m_pVariable->Set((int)col);
        }
    }
    else if (m_type == ePropertySOHelper || m_type == ePropertySONavHelper || m_type == ePropertySOAnimHelper)
    {
        // keep smart object class part

        CString oldValue;
        m_pVariable->Get(oldValue);
        int f = oldValue.Find(':');
        if (f >= 0)
        {
            oldValue.Truncate(f + 1);
        }

        CString newValue(m_value);
        f = newValue.Find(':');
        if (f >= 0)
        {
            newValue.Delete(0, f + 1);
        }

        m_pVariable->Set(oldValue + newValue);
    }
    else if (m_type != ePropertyInvalid)
    {
        m_pVariable->SetDisplayValue(m_value);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnVariableChange(IVariable* pVar)
{
    assert(pVar != 0 && pVar == m_pVariable);

    // When variable changes, invalidate UI.
    m_modified = true;

    VarToValue();

    // update enum list
    if (m_type == ePropertySelection)
    {
        m_enumList = m_pVariable->GetEnumList();
        PopulateInPlaceComboBoxFromEnumList();
    }

    SendToControl();

    if (m_bEditChildren)
    {
        switch (m_type)
        {
        case ePropertyAiPFPropertiesList:
            AddChildrenForPFProperties();
            break;

        case ePropertyAiEntityClasses:
            AddChildrenForAIEntityClasses();
            break;

        default:
        {
            // Parse comma-separated values, set children.
            bool bPrevIgnore = m_bIgnoreChildsUpdate;
            m_bIgnoreChildsUpdate = true;

            int pos = 0;
            for (int i = 0; i < m_childs.size(); i++)
            {
                CString sElem = pos >= 0 ? m_value.Tokenize(", ", pos) : CString();
                m_childs[i]->SetValue(sElem, false);
            }
            m_bIgnoreChildsUpdate = bPrevIgnore;
        }
        }
    }

    if (m_parent)
    {
        m_parent->OnChildChanged(this);
    }

    // If Value changed mark document modified.
    // Notify parent that this Item have been modified.
    // This may delete this control...
    m_propertyCtrl->OnItemChange(this);
}
//////////////////////////////////////////////////////////////////////////
CPropertyItem::TDValues*    CPropertyItem::GetEnumValues(const CString& strPropertyName)
{
    TDValuesContainer::iterator itIterator;
    TDValuesContainer&                  cEnumContainer = CUIEnumerations::GetUIEnumerationsInstance().GetStandardNameContainer();

    itIterator = cEnumContainer.find(strPropertyName);
    if (itIterator == cEnumContainer.end())
    {
        return NULL;
    }

    return &itIterator->second;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
    if (m_propertyCtrl->IsReadOnly())
    {
        return;
    }

    if (m_cCombo)
    {
        int sel = m_cCombo->GetCurSel();
        if (zDelta > 0)
        {
            sel++;
            if (m_cCombo->SetCurSel(sel) == CB_ERR)
            {
                m_cCombo->SetCurSel(0);
            }
        }
        else
        {
            sel--;
            if (m_cCombo->SetCurSel(sel) == CB_ERR)
            {
                m_cCombo->SetCurSel(m_cCombo->GetCount() - 1);
            }
        }
    }
    else if (m_cNumber)
    {
        if (zDelta > 0)
        {
            m_cNumber->SetValue(m_cNumber->GetValue() + m_cNumber->GetStep());
        }
        else
        {
            m_cNumber->SetValue(m_cNumber->GetValue() - m_cNumber->GetStep());
        }
        ReceiveFromControl();
    }
}


//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    if (m_propertyCtrl->IsReadOnly())
    {
        return;
    }

    if (IsDisabled())
    {
        return;
    }

    if (m_type == ePropertyBool)
    {
        // Swap boolean value.
        if (GetBoolValue())
        {
            SetValue("0");
        }
        else
        {
            SetValue("1");
        }
    }
    else
    {
        // Simulate button click.
        if (m_cButton)
        {
            m_cButton->Click();
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (m_propertyCtrl->IsReadOnly())
    {
        return;
    }

    if (m_type == ePropertyBool)
    {
        CRect rect;
        m_propertyCtrl->GetItemRect(this, rect);
        rect = m_propertyCtrl->GetItemValueRect(rect);

        CPoint p(rect.left - 2, rect.top + 1);
        int sz = rect.bottom - rect.top;
        rect = CRect(p.x, p.y, p.x + sz, p.y + sz);

        if (rect.PtInRect(point))
        {
            // Swap boolean value.
            if (GetBoolValue())
            {
                SetValue("0");
            }
            else
            {
                SetValue("1");
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnCheckBoxButton()
{
    if (m_cCheckBox)
    {
        if (m_cCheckBox->GetChecked() == BST_CHECKED)
        {
            SetValue("1");
        }
        else
        {
            SetValue("0");
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnColorBrowseButton()
{
    /*
    COLORREF clr = StringToColor(m_value);
  if (GetIEditor()->SelectColor(clr,m_propertyCtrl))
  {
        int r,g,b;
        r = GetRValue(clr);
        g = GetGValue(clr);
        b = GetBValue(clr);
    //val.Format( "R:%d G:%d B:%d",r,g,b );
        CString val;
        val.Format( "%d,%d,%d",r,g,b );
        SetValue( val );
        m_propertyCtrl->Invalidate();
        //RedrawWindow( OwnerProperties->hWnd,NULL,NULL,RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN );
  }
    */
    if (IsDisabled())
    {
        return;
    }

    QColor orginalColor = QtUtil::QColorFromCOLORREF(StringToColor(m_value));
    QColor clr = orginalColor;
    QColorDialog dlg(orginalColor);
    QObject::connect(&dlg, &QColorDialog::currentColorChanged, std::bind(&CPropertyItem::OnColorChange, this, std::placeholders::_1));
    if (dlg.exec() == QDialog::Accepted)
    {
        QColor clr = dlg.selectedColor();
        if (clr != orginalColor)
        {
            QString val;
            val = QStringLiteral("%1,%2,%3").arg(orginalColor.red()).arg(orginalColor.green()).arg(orginalColor.blue());
            SetValue(val.toLatin1().data(), false);

            val = QStringLiteral("%1,%2,%3").arg(clr.red()).arg(clr.green()).arg(clr.blue());
            SetValue(val.toLatin1().data());
            m_propertyCtrl->InvalidateCtrl();
        }
    }
    else
    {
        if (StringToColor(m_value) != QtUtil::ToCOLORREF(orginalColor))
        {
            QString val = QStringLiteral("%1,%2,%3").arg(clr.red()).arg(clr.green()).arg(clr.blue());
            SetValue(val.toLatin1().data());
            m_propertyCtrl->InvalidateCtrl();
        }
    }
    if (m_cButton)
    {
        m_cButton->Invalidate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnColorChange(const QColor& clr)
{
    GetIEditor()->SuspendUndo();
    QString val = QStringLiteral("%1,%2,%3").arg(clr.red()).arg(clr.green()).arg(clr.blue());
    SetValue(val.toLatin1().data());
    GetIEditor()->UpdateViews(eRedrawViewports);
    //GetIEditor()->Notify( eNotify_OnIdleUpdate );
    m_propertyCtrl->InvalidateCtrl();
    GetIEditor()->ResumeUndo();

    if (m_cButton)
    {
        m_cButton->Invalidate();
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnFileBrowseButton()
{
    CString tempValue("");
    CString ext("");
    if (m_value.IsEmpty() == false)
    {
        if (Path::GetExt(m_value) == "")
        {
            tempValue = "";
        }
        else
        {
            tempValue = m_value;
        }
    }

    m_propertyCtrl->HideBitmapTooltip();

    AssetSelectionModel selection;
    bool forceModified = false;

    if (m_type == ePropertyTexture)
    {
        // Filters for texture.
        selection = AssetSelectionModel::AssetGroupSelection("Texture");
    }
    else if (m_type == ePropertyModel)
    {
        // Filters for models.
        selection = AssetSelectionModel::AssetGroupSelection("Geometry");
    }
    else if (m_type == ePropertyGeomCache)
    {
        // Filters for geom caches.
        selection = AssetSelectionModel::AssetTypeSelection("Geom Cache");
        forceModified = true;
    }
    else
    {
        // No filtering
        selection = AssetSelectionModel::EverythingSelection();
    }

    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (selection.IsValid())
    {
        SetValue(Path::FullPathToGamePath(selection.GetResult()->GetFullPath().c_str()), true, forceModified);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnResourceSelectorButton()
{
    m_propertyCtrl->HideBitmapTooltip();
    SResourceSelectorContext x;
    x.parentWindow = m_propertyCtrl->GetSafeHwnd();
    x.typeName = Prop::GetPropertyTypeToResourceType(m_type);

    string oldValue(GetValue());
    dll_string newValue = GetIEditor()->GetResourceSelectorHost()->SelectResource(x, oldValue.c_str());
    if (strcmp(oldValue.c_str(), newValue.c_str()) != 0)
    {
        SetValue(newValue.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnTextureBrowseButton()
{
    CString strInputString(m_value);

    strInputString = Path::GetRelativePath(CString(m_value.GetBuffer()));
    strlwr((char*)strInputString.GetBuffer());
    CAssetBrowserDialog::Open(strInputString.GetBuffer(), "Textures");
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnTextureApplyButton()
{
    string strFilename;

    if (!CAssetBrowserDialog::Instance())
    {
        // just open the browser and let user choose some asset first
        CAssetBrowserDialog::Open(m_value, "Textures");
        return;
    }

    strFilename = CAssetBrowserDialog::Instance()->GetFirstSelectedFilename();

    SetValue(strFilename);
}
//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnTextureEditButton()
{
    CFileUtil::EditTextureFile(m_value, true);
}
//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnPsdEditButton()
{
    CString dccFilename;
    if (CFileUtil::CalculateDccFilename(m_value, dccFilename))
    {
        CFileUtil::EditTextureFile(dccFilename, true);
        return;
    }
    gEnv->pLog->LogError("Failed to find psd file for texture: '%s'", m_value);
}
//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAnimationBrowseButton()
{
    const EntityId entityId = reinterpret_cast<EntityId>(m_pVariable->GetUserData());
    IEntity* pEntity = gEnv->pEntitySystem->GetEntity(entityId);
    ICharacterInstance* pCharacterInstance = NULL;
    if (pEntity)
    {
        pCharacterInstance = pEntity->GetCharacter(0);
    }
    // If a proper character instance exists in the entity specified in the variable,
    // load the animation list in the animation browser.
    if (pCharacterInstance)
    {
        if (m_pAnimBrowser == NULL)
        {
            m_pAnimBrowser = new CAnimationBrowser;
            m_pAnimBrowser->AddOnDblClickCallback(functor(*this, &CPropertyItem::OnAnimationApplyButton));
        }

        m_pAnimBrowser->SetModel_SKEL(pCharacterInstance);
        m_pAnimBrowser->show();

        // Position the animation browser right below the browse button.
        if (m_cButton2)
        {
            CRect rect;
            m_cButton2->GetWindowRect(&rect);
            LONG top = rect.top + rect.Height();
            LONG left = rect.left;
            QRect geometry = m_pAnimBrowser->geometry();
            geometry.offset(left - rect.left, top - rect.top);

            // Clip window position to always appear on-screen
            ClipOrCenterRectToMonitor(&geometry, MONITOR_WORKAREA | MONITOR_CLIP);

            m_pAnimBrowser->setGeometry(geometry);
        }
    }
    // If not, just open the whole character editor so that the user can select a character.
    else
    {
        GetIEditor()->ExecuteCommand("general.open_pane 'Character Editor'");
    }
}
//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAnimationApplyButton()
{
    SAFE_DELETE(m_pAnimBrowser);

    CUIEnumerations& roGeneralProxy = CUIEnumerations::GetUIEnumerationsInstance();
    std::vector<CString> cSelectedAnimations;
    size_t nTotalAnimations(0);
    size_t nCurrentAnimation(0);

    CString combinedString = GetIEditor()->GetResourceSelectorHost()->GetGlobalSelection("animation");
    SplitString(combinedString, cSelectedAnimations, ',');

    nTotalAnimations = cSelectedAnimations.size();
    for (nCurrentAnimation = 0; nCurrentAnimation < nTotalAnimations; ++nCurrentAnimation)
    {
        CString& rstrCurrentAnimAction = cSelectedAnimations[nCurrentAnimation];
        if (rstrCurrentAnimAction.GetLength())
        {
            m_pVariable->Set(rstrCurrentAnimAction);
            return;
        }
    }
}
//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnShaderBrowseButton()
{
    CShadersDialog cShaders(QtUtil::ToQString(m_value));
    if (cShaders.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(cShaders.GetSelection()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnParticleBrowseButton()
{
    CString libraryName = "";
    CBaseLibraryItem* pItem = NULL;
    CString particleName(m_value);

    if (strcmp(particleName, "") != 0)
    {
        int  pos = 0;
        libraryName = particleName.Tokenize(".", pos);
        IEditorParticleManager* particleMgr = GetIEditor()->GetParticleManager();
        IDataBaseLibrary* pLibrary = particleMgr->FindLibrary(libraryName);
        if (pLibrary == NULL)
        {
            CString fullLibraryName = libraryName + ".xml";
            fullLibraryName.MakeLower();
            fullLibraryName = CString("Libs/Particles/") + fullLibraryName;
            GetIEditor()->SuspendUndo();
            pLibrary = particleMgr->LoadLibrary(fullLibraryName);
            GetIEditor()->ResumeUndo();
            if (pLibrary == NULL)
            {
                return;
            }
        }

        particleName.Delete(0, pos);
        pItem = (CBaseLibraryItem*)pLibrary->FindItem(particleName);
        if (pItem == NULL)
        {
            CString leafParticleName(particleName);

            int lastDotPos = particleName.ReverseFind('.');
            while (!pItem && lastDotPos > -1)
            {
                particleName.Delete(lastDotPos, particleName.GetLength() - lastDotPos);
                lastDotPos = particleName.ReverseFind('.');
                pItem = (CBaseLibraryItem*)pLibrary->FindItem(particleName);
            }

            leafParticleName.Replace(particleName, "");
            if (leafParticleName.IsEmpty() || (leafParticleName.GetLength() == 1 && leafParticleName[0] == '.'))
            {
                return;
            }
            if (leafParticleName[0] == '.')
            {
                leafParticleName.Delete(0);
            }
        }
    }

    QtViewPaneManager::instance()->OpenPane(LyViewPane::DatabaseView);

    CDataBaseDialog* pDataBaseDlg = FindViewPane<CDataBaseDialog>(LyViewPane::DatabaseView);
    if (!pDataBaseDlg)
    {
        return;
    }

    pDataBaseDlg->SelectDialog(EDB_TYPE_PARTICLE);

    CParticleDialog* particleDlg = (CParticleDialog*)pDataBaseDlg->GetCurrent();
    if (particleDlg == NULL)
    {
        return;
    }

    particleDlg->Reload();

    if (pItem && strcmp(libraryName, "") != 0)
    {
        particleDlg->SelectLibrary(QtUtil::ToQString(libraryName));
    }

    particleDlg->SelectItem(pItem);
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnParticleApplyButton()
{
    CParticleDialog* rpoParticleBrowser = CParticleDialog::GetCurrentInstance();
    if (rpoParticleBrowser)
    {
        CParticleItem* currentParticle = rpoParticleBrowser->GetSelectedParticle();
        if (currentParticle)
        {
            rpoParticleBrowser->OnAssignToSelection();
            SetValue(currentParticle->GetFullName());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::ReloadValues()
{
    m_modified = false;
    if (m_node)
    {
        ParseXmlNode(false);
    }
    if (m_pVariable)
    {
        SetVariable(m_pVariable);
    }
    for (int i = 0; i < GetChildCount(); i++)
    {
        GetChild(i)->ReloadValues();
    }
    SendToControl();
}

//////////////////////////////////////////////////////////////////////////
CString CPropertyItem::GetTip() const
{
    if (!m_tip.IsEmpty() || m_name.Left(1) == "_")
    {
        return m_tip;
    }

    CString type = Prop::GetName(m_type);

    CString tip = CString("[") + type + "] " + m_name + " = " + m_value;

    if (HasScriptDefault())
    {
        tip += " [Script Default: ";
        if (m_strScriptDefault.IsEmpty())
        {
            tip += "<blank>";
        }
        else
        {
            tip += m_strScriptDefault;
        }
        tip += "]";
    }

    if (m_pVariable)
    {
        CString description = m_pVariable->GetDescription();
        if (!description.IsEmpty())
        {
            tip += CString("\n") + description;
        }
    }
    return tip;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnEditChanged()
{
    ReceiveFromControl();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnNumberCtrlUpdate(CNumberCtrl* ctrl)
{
    ReceiveFromControl();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnFillSliderCtrlUpdate(CSliderCtrlEx* ctrl)
{
    if (m_cFillSlider)
    {
        float fValue = m_cFillSlider->GetValue();

        if (m_step != 0.f)
        {
            // Round to next power of 10 below step.
            float fRound = pow(10.f, floor(log(m_step) / log(10.f))) / m_valueMultiplier;
            fValue = int_round(fValue / fRound) * fRound;
        }
        fValue = clamp_tpl(fValue, m_rangeMin, m_rangeMax);

        CString val;
        val.Format("%g", fValue);
        SetValue(val, true, true);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAIBehaviorBrowseButton()
{
    CAIDialog aiDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    aiDlg.SetAIBehavior(QtUtil::ToQString(m_value));
    if (aiDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(aiDlg.GetAIBehavior()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAIAnchorBrowseButton()
{
    CAIAnchorsDialog aiDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    aiDlg.SetAIAnchor(QtUtil::ToQString(m_value));
    if (aiDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(aiDlg.GetAIAnchor()));
    }
}

#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAICharacterBrowseButton()
{
    CAICharactersDialog aiDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    aiDlg.SetAICharacter(QtUtil::ToQString(m_value));
    if (aiDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(aiDlg.GetAICharacter()));
    }
}
#endif

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAIPFPropertiesListBrowseButton()
{
    CAIPFPropertiesListDialog dlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    dlg.SetPFPropertiesList(QtUtil::ToQString(m_value));
    if (dlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(dlg.GetPFPropertiesList()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAIEntityClassesBrowseButton()
{
    CAIEntityClassesDialog dlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    dlg.SetAIEntityClasses(QtUtil::ToQString(m_value));
    if (dlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(dlg.GetAIEntityClasses()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnEquipBrowseButton()
{
	// KDAB TOOD: fix parent
    CEquipPackDialog EquipDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    EquipDlg.SetCurrEquipPack(QtUtil::ToQString(m_value));
    EquipDlg.SetEditMode(false);
    if (EquipDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(EquipDlg.GetCurrEquipPack()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnEAXPresetBrowseButton()
{
    CSelectEAXPresetDlg PresetDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    PresetDlg.SetCurrPreset(QtUtil::ToQString(m_value));
    if (PresetDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(PresetDlg.GetCurrPreset()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnMaterialBrowseButton()
{
    // Open material browser dialog.
    CString name = GetValue();
    IDataBaseItem* pItem = GetIEditor()->GetMaterialManager()->FindItemByName(name);
    GetIEditor()->OpenDataBaseLibrary(EDB_TYPE_MATERIAL, pItem);
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnMaterialPickSelectedButton()
{
    // Open material browser dialog.
    IDataBaseItem* pItem = GetIEditor()->GetMaterialManager()->GetSelectedItem();
    if (pItem)
    {
        SetValue(pItem->GetName());
    }
    else
    {
        SetValue("");
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOClassBrowseButton()
{
    CSmartObjectClassDialog soDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    soDlg.SetSOClass(QtUtil::ToQString(m_value));
    if (soDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(soDlg.GetSOClass()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOClassesBrowseButton()
{
    CSmartObjectClassDialog soDlg(/*m_propertyCtrl*/ QApplication::activeWindow(), true);
    soDlg.SetSOClass(QtUtil::ToQString(m_value));
    if (soDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(soDlg.GetSOClass()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOStateBrowseButton()
{
	//TODO KDAB set parenting properly once this is ported
    CSmartObjectStateDialog soDlg(QtUtil::ToQString(m_value), /*m_propertyCtrl*/ QApplication::activeWindow(), false);
    if (soDlg.exec())
    {
        SetValue(soDlg.GetSOState().toLocal8Bit().constData());
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOStatesBrowseButton()
{
	//TODO KDAB set parenting properly once this is ported
    CSmartObjectStateDialog soDlg(QtUtil::ToQString(m_value), /*m_propertyCtrl*/ QApplication::activeWindow(), true);
    if (soDlg.exec())
    {
        SetValue(soDlg.GetSOState().toLocal8Bit().constData());
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOStatePatternBrowseButton()
{
    QString sPattern = QtUtil::ToQString(m_value);
    if (m_value.Find('|') < 0)
    {
		//TODO KDAB set parenting properly once this is ported
        CSmartObjectStateDialog soDlg(QtUtil::ToQString(m_value), QApplication::activeWindow(), true, true);
        switch (soDlg.exec())
        {
        case QDialog::Accepted:
            SetValue(soDlg.GetSOState().toLocal8Bit().constData());
            return;
        case QDialog::Rejected:
            return;
        case CSmartObjectStateDialog::ADVANCED_MODE_REQUESTED:
            sPattern = soDlg.GetSOState();
            break;
        }
    }
    CSmartObjectPatternDialog soPatternDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    soPatternDlg.SetPattern(sPattern);
    if (soPatternDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(soPatternDlg.GetPattern()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOActionBrowseButton()
{
    CSmartObjectActionDialog soDlg(/*m_propertyCtrl*/ QApplication::activeWindow());// (m_propertyCtrl);
    soDlg.SetSOAction(QtUtil::ToQString(m_value));
    if (soDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(soDlg.GetSOAction()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOHelperBrowseButton()
{
    CSmartObjectHelperDialog soDlg(/*m_propertyCtrl*/ QApplication::activeWindow(), true, false, false);
    CString value;
    m_pVariable->Get(value);
    int f = value.Find(':');
    if (f > 0)
    {
        soDlg.SetSOHelper(QtUtil::ToQString(value.Left(f)), QtUtil::ToQString(value.Mid(f + 1)));
        if (soDlg.exec() == QDialog::Accepted)
        {
            SetValue(soDlg.GetSOHelper().toLatin1().data());
        }
    }
    else
    {
        QMessageBox::critical(/*m_propertyCtrl*/ QApplication::activeWindow(), QString(), QObject::tr("This field can not be edited because it needs the smart object class.\nPlease, choose a smart object class first..."));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSONavHelperBrowseButton()
{
    CSmartObjectHelperDialog soDlg(/*m_propertyCtrl*/ QApplication::activeWindow(), false, true, false);
    CString value;
    m_pVariable->Get(value);
    int f = value.Find(':');
    if (f > 0)
    {
        soDlg.SetSOHelper(QtUtil::ToQString(value.Left(f)), QtUtil::ToQString(value.Mid(f + 1)));
        if (soDlg.exec() == QDialog::Accepted)
        {
            SetValue(soDlg.GetSOHelper().toLatin1().data());
        }
    }
    else
    {
        QMessageBox::critical(/*m_propertyCtrl*/ QApplication::activeWindow(), QString(), QObject::tr("This field can not be edited because it needs the smart object class.\nPlease, choose a smart object class first..."));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOAnimHelperBrowseButton()
{
    CSmartObjectHelperDialog soDlg(/*m_propertyCtrl*/ QApplication::activeWindow(), true, false, true);
    CString value;
    m_pVariable->Get(value);
    int f = value.Find(':');
    if (f > 0)
    {
        soDlg.SetSOHelper(QtUtil::ToQString(value.Left(f)), QtUtil::ToQString(value.Mid(f + 1)));
        if (soDlg.exec() == QDialog::Accepted)
        {
            SetValue(soDlg.GetSOHelper().toLatin1().data());
        }
    }
    else
    {
        QMessageBox::critical(/*m_propertyCtrl*/ QApplication::activeWindow(), QString(), QObject::tr("This field can not be edited because it needs the smart object class.\nPlease, choose a smart object class first..."));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOEventBrowseButton()
{
    CSmartObjectEventDialog soDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    soDlg.SetSOEvent(QtUtil::ToQString(m_value));
    if (soDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(soDlg.GetSOEvent()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSOTemplateBrowseButton()
{
	//TODO soDlg needs to have its parent set to 'this' once CProperyItem is ported to Qt
    CSmartObjectTemplateDialog soDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    soDlg.SetSOTemplate(QtUtil::ToQString(m_value));
    if (soDlg.exec())
    {
        SetValue(QtUtil::ToCString(soDlg.GetSOTemplate()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnCustomActionBrowseButton()
{
    CCustomActionDialog customActionDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    customActionDlg.SetCustomAction(QtUtil::ToQString(m_value));
    if (customActionDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(customActionDlg.GetCustomAction()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnGameTokenBrowseButton()
{
    CSelectGameTokenDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    gtDlg.PreSelectGameToken(GetValue());
    if (gtDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(gtDlg.GetSelectedGameToken()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSequenceBrowseButton()
{
    CSelectSequenceDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    gtDlg.PreSelectItem(GetValue());
    if (gtDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(gtDlg.GetSelectedItem()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnSequenceIdBrowseButton()
{
    CSelectSequenceDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    uint32 id = (uint32)atoi(GetValue());
    IAnimSequence* pSeq = GetIEditor()->GetMovieSystem()->FindSequenceById(id);
    if (pSeq)
    {
        gtDlg.PreSelectItem(pSeq->GetName());
    }
    if (gtDlg.exec() == QDialog::Accepted)
    {
        pSeq = GetIEditor()->GetMovieSystem()->FindSequence(QtUtil::ToCString(gtDlg.GetSelectedItem()));
        assert(pSeq);
        if (pSeq->GetId() > 0)  // This sequence is a new one with a valid ID.
        {
            CString buf;
            buf.Format("%d", pSeq->GetId());
            SetValue(buf);
        }
        else                                        // This sequence is an old one without an ID.
        {
            QMessageBox::critical(/*m_propertyCtrl*/ QApplication::activeWindow(), QString(), QObject::tr("This is an old sequence without an ID.\nSo it cannot be used with the new ID-based linking."));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAudioMusicThemeAndMoodBrowseButton()
{
    std::vector<CString> asThemesAndMoods;

    // Make sure the iterator points to the beginning of the container
    IStringItVec* const pThemes = gEnv->pMusicSystem->GetThemes();

    if (pThemes)
    {
        stack_string sTemp;

        while (!pThemes->IsEnd())
        {
            sTemp = pThemes->Next();

            // Now the corresponding moods
            IStringItVec* const pMoods = gEnv->pMusicSystem->GetMoods(sTemp.c_str());

            if (pMoods)
            {
                sTemp += "/";
                stack_string const sTheme(sTemp);

                while (!pMoods->IsEnd())
                {
                    sTemp += pMoods->Next();
                    asThemesAndMoods.push_back(sTemp.c_str());
                    sTemp = sTheme;
                }
            }
        }

        CGenericSelectItemDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
        gtDlg.setWindowTitle(QObject::tr("Select Theme and Mood"));
        gtDlg.SetItems(asThemesAndMoods);

        gtDlg.PreSelectItem(GetValue());

        if (gtDlg.exec() == QDialog::Accepted)
        {
            SetValue(QtUtil::ToCString(gtDlg.GetSelectedItem()));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAudioMusicPatternsBrowseButton()
{
    std::vector<CString> asPatterns;

    // Make sure the iterator points to the beginning of the container
    IStringItVec* const pPatterns = gEnv->pMusicSystem->GetPatterns();

    if (pPatterns)
    {
        while (!pPatterns->IsEnd())
        {
            asPatterns.push_back(pPatterns->Next());
        }

        CGenericSelectItemDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
        gtDlg.setWindowTitle(QObject::tr("Select Pattern"));
        gtDlg.SetItems(asPatterns);

        gtDlg.PreSelectItem(GetValue());

        if (gtDlg.exec() == QDialog::Accepted)
        {
            SetValue(QtUtil::ToCString(gtDlg.GetSelectedItem()));
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAudioMusicLogicPresetsBrowseButton()
{
    // Audio: music logic presets browsing
    XmlNodeRef const oNodeRoot;
    if (oNodeRoot)
    {
        std::vector<CString> asPresets;
        CString sFileName;
        int const nNodeCount = oNodeRoot->getChildCount();

        for (int i = 0; i < nNodeCount; ++i)
        {
            XmlNodeRef const oNodeMusicLogic(oNodeRoot->getChild(i));

            if (oNodeMusicLogic && (_stricmp(oNodeMusicLogic->getTag(), "MusicLogic") == 0))
            {
                int const nNodeChildCount = oNodeMusicLogic->getChildCount();

                for (int j = 0; j < nNodeChildCount; ++j)
                {
                    XmlNodeRef const oNodeFile(oNodeMusicLogic->getChild(j));

                    if ((_stricmp(oNodeFile->getTag(), "File") == 0) && (_stricmp(oNodeFile->getAttr("type"), "all") == 0))
                    {
                        sFileName = oNodeFile->getAttr("name");

                        if (!sFileName.IsEmpty())
                        {
                            XmlNodeRef const oNodeMLRoot(gEnv->pSystem->LoadXmlFromFile(sFileName.GetString()));

                            XmlNodeRef const oNodePresets(oNodeMLRoot->findChild("Presets"));

                            if (oNodePresets)
                            {
                                int const nCountPresets = oNodePresets->getChildCount();

                                for (int k = 0; k < nCountPresets; ++k)
                                {
                                    XmlNodeRef const oNodePreset(oNodePresets->getChild(k));

                                    if (oNodePreset && (_stricmp(oNodePreset->getTag(), "Preset") == 0))
                                    {
                                        CString const sNamePreset(oNodePreset->getAttr("name"));

                                        if (!sNamePreset.IsEmpty())
                                        {
                                            stl::push_back_unique(asPresets, sNamePreset);
                                        }
                                    }
                                }
                            }
                        }

                        break;
                    }
                }

                if (!sFileName.IsEmpty())
                {
                    break;
                }
            }
        }

        if (!asPresets.empty())
        {
            CGenericSelectItemDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
            gtDlg.setWindowTitle("Select Preset");
            gtDlg.SetItems(asPresets);

            gtDlg.PreSelectItem(GetValue());

            if (gtDlg.exec() == QDialog::Accepted)
            {
                SetValue(QtUtil::ToCString(gtDlg.GetSelectedItem()));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnAudioMusicLogicEventsBrowseButton()
{
    // Audio: music logic events browsing
    XmlNodeRef const oNodeRoot;
    if (oNodeRoot)
    {
        std::vector<CString> asEvents;
        CString sFileName;
        int const nNodeCount = oNodeRoot->getChildCount();

        for (int i = 0; i < nNodeCount; ++i)
        {
            XmlNodeRef const oNodeMusicLogic(oNodeRoot->getChild(i));

            if (oNodeMusicLogic && (_stricmp(oNodeMusicLogic->getTag(), "MusicLogicInputs") == 0))
            {
                int const nNodeChildCount = oNodeMusicLogic->getChildCount();

                for (int j = 0; j < nNodeChildCount; ++j)
                {
                    XmlNodeRef const oNodeFile(oNodeMusicLogic->getChild(j));

                    if ((_stricmp(oNodeFile->getTag(), "File") == 0) && (_stricmp(oNodeFile->getAttr("type"), "all") == 0))
                    {
                        sFileName = oNodeFile->getAttr("name");

                        if (!sFileName.IsEmpty())
                        {
                            XmlNodeRef const oNodeMLRoot(gEnv->pSystem->LoadXmlFromFile(sFileName.GetString()));

                            XmlNodeRef const oNodeEvents(oNodeMLRoot->findChild("Events"));

                            if (oNodeEvents)
                            {
                                int const nCountEvents = oNodeEvents->getChildCount();

                                for (int k = 0; k < nCountEvents; ++k)
                                {
                                    XmlNodeRef const oNodeEvent(oNodeEvents->getChild(k));

                                    if (oNodeEvent && (_stricmp(oNodeEvent->getTag(), "Event") == 0))
                                    {
                                        CString const sNameEvent(oNodeEvent->getAttr("name"));

                                        if (!sNameEvent.IsEmpty())
                                        {
                                            stl::push_back_unique(asEvents, sNameEvent);
                                        }
                                    }
                                }
                            }
                        }

                        break;
                    }
                }

                if (!sFileName.IsEmpty())
                {
                    break;
                }
            }
        }

        if (!asEvents.empty())
        {
            CGenericSelectItemDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
            gtDlg.setWindowTitle(QObject::tr("Select Event"));
            gtDlg.SetItems(asEvents);

            gtDlg.PreSelectItem(GetValue());

            if (gtDlg.exec() == QDialog::Accepted)
            {
                SetValue(QtUtil::ToCString(gtDlg.GetSelectedItem()));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnMissionObjButton()
{
    CSelectMissionObjectiveDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    gtDlg.PreSelectItem(GetValue());
    if (gtDlg.exec() == QDialog::Accepted)
    {
        SetValue(QtUtil::ToCString(gtDlg.GetSelectedItem()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnUserBrowseButton()
{
    IVariable::IGetCustomItems* pGetCustomItems = static_cast<IVariable::IGetCustomItems*> (m_pVariable->GetUserData());
    if (pGetCustomItems != 0)
    {
        std::vector<IVariable::IGetCustomItems::SItem> items;
        CString dlgTitle;
        // call the user supplied callback to fill-in items and get dialog title
        bool bShowIt = pGetCustomItems->GetItems(m_pVariable, items, dlgTitle);
        if (bShowIt) // if func didn't veto, show the dialog
        {
            CGenericSelectItemDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
            if (pGetCustomItems->UseTree())
            {
                gtDlg.SetMode(CGenericSelectItemDialog::eMODE_TREE);
                const char* szSep = pGetCustomItems->GetTreeSeparator();
                if (szSep)
                {
                    QString sep (szSep);
                    gtDlg.SetTreeSeparator(sep);
                }
            }
            gtDlg.SetItems(items);
            if (dlgTitle.IsEmpty() == false)
            {
                gtDlg.setWindowTitle(QtUtil::ToQString(dlgTitle));
            }
            gtDlg.PreSelectItem(GetValue());
            if (gtDlg.exec() == QDialog::Accepted)
            {
                CString selectedItemStr = QtUtil::ToCString(gtDlg.GetSelectedItem());

                if (selectedItemStr.IsEmpty() == false)
                {
                    SetValue(selectedItemStr);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnLocalStringBrowseButton()
{
    std::vector<IVariable::IGetCustomItems::SItem> items;
    ILocalizationManager* pMgr = gEnv->pSystem->GetLocalizationManager();
    if (!pMgr)
    {
        return;
    }
    int nCount = pMgr->GetLocalizedStringCount();
    if (nCount <= 0)
    {
        return;
    }
    items.reserve(nCount);
    IVariable::IGetCustomItems::SItem item;
    SLocalizedInfoEditor sInfo;
    for (int i = 0; i < nCount; ++i)
    {
        if (pMgr->GetLocalizedInfoByIndex(i, sInfo))
        {
            item.desc = _T("English Text:\r\n");
            item.desc += Unicode::Convert<wstring>(sInfo.sUtf8TranslatedText).c_str();
            item.name = sInfo.sKey;
            items.push_back(item);
        }
    }
    CString dlgTitle;
    CGenericSelectItemDialog gtDlg(/*m_propertyCtrl*/ QApplication::activeWindow());
    const bool bUseTree = true;
    if (bUseTree)
    {
        gtDlg.SetMode(CGenericSelectItemDialog::eMODE_TREE);
        gtDlg.SetTreeSeparator("/");
    }
    gtDlg.SetItems(items);
    gtDlg.setWindowTitle(QObject::tr("Choose Localized String"));
    CString preselect = GetValue();
    if (!preselect.IsEmpty() && preselect.GetAt(0) == '@')
    {
        preselect = preselect.Mid(1);
    }
    gtDlg.PreSelectItem(QtUtil::ToQString(preselect));
    if (gtDlg.exec() == QDialog::Accepted)
    {
        preselect = "@";
        preselect += QtUtil::ToCString(gtDlg.GetSelectedItem());
        SetValue(preselect);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnLightAnimationBrowseButton()
{
    // First, check if there is any light animation defined.
    bool bLightAnimationExists = false;
    IMovieSystem* pMovieSystem = GetIEditor()->GetMovieSystem();
    for (int i = 0; i < pMovieSystem->GetNumSequences(); ++i)
    {
        IAnimSequence* pSequence = pMovieSystem->GetSequence(i);
        if (pSequence->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet)
        {
            bLightAnimationExists = pSequence->GetNodeCount() > 0;
            break;
        }
    }

    if (bLightAnimationExists) // If exists, show the selection dialog.
    {
        CSelectLightAnimationDialog dlg(/*m_propertyCtrl*/ QApplication::activeWindow());
        dlg.PreSelectItem(GetValue());
        if (dlg.exec() == QDialog::Accepted)
        {
            SetValue(QtUtil::ToCString(dlg.GetSelectedItem()));
        }
    }
    else                       // If not, remind the user of creating one in TrackView.
    {
        QMessageBox::critical(/*m_propertyCtrl*/ QApplication::activeWindow(), QString(), QObject::tr("There is no available light animation.\nPlease create one in TrackView, first."));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnExpandButton()
{
    m_propertyCtrl->Expand(this, !IsExpanded(), true);
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnFlareDatabaseButton()
{
    const QtViewPane* lensFlarePane = GetIEditor()->OpenView(CLensFlareEditor::s_pLensFlareEditorClassName);
    if (!lensFlarePane)
    {
        return;
    }

    CLensFlareEditor* editor = FindViewPane<CLensFlareEditor>(QtUtil::ToQString(CLensFlareEditor::s_pLensFlareEditorClassName));
    if (editor)
    {
        QTimer::singleShot(0, editor, SLOT(OnUpdateTreeCtrl()));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnUiElementBrowseButton()
{
    const QtViewPane* pane = GetIEditor()->OpenView("UI Editor", true);
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::OnUiElementPickSelectedButton()
{
     LyShine::EntityArray selectedElements;
     EBUS_EVENT_RESULT(selectedElements, UiEditorDLLBus, GetSelectedElements);

     if (selectedElements.size() == 1)
     {
         AZ::Entity* entity = selectedElements[0];
         SetValue(entity->GetName().c_str());
     }
     else if (selectedElements.empty())
     {
         SetValue("");
     }
}

//////////////////////////////////////////////////////////////////////////
int CPropertyItem::GetHeight()
{
    if (m_propertyCtrl->IsExtenedUI())
    {
        //m_nHeight = 20;
        switch (m_type)
        {
        case ePropertyFloatCurve:
            //m_nHeight = 52;
            return 52;
            break;
        case ePropertyColorCurve:
            //m_nHeight = 36;
            return 36;
        }
        return 20;
    }
    return m_nHeight;
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::AddChildrenForPFProperties()
{
    assert(m_type == ePropertyAiPFPropertiesList);

    for (Childs::iterator it = m_childs.begin(); it != m_childs.end(); ++it)
    {
        m_propertyCtrl->DestroyControls(*it);
    }

    RemoveAllChildren();
    m_propertyCtrl->InvalidateCtrl();

    std::set<CString> setSelectedPathTypeNames;

    CString token;
    int index = 0;
    while (!(token = m_value.Tokenize(" ,", index)).IsEmpty())
    {
        setSelectedPathTypeNames.insert(token);
    }

    int N = 1;
    char propertyN[100];
    for (std::set<CString>::iterator it = setSelectedPathTypeNames.begin(); it != setSelectedPathTypeNames.end(); ++it)
    {
        IVariable* pVar = new CVariable<CString>();
        sprintf_s(propertyN, "AgentType %d", N++);
        pVar->SetName(propertyN);
        pVar->Set(*it);

        CPropertyItemPtr pItem = new CPropertyItem(m_propertyCtrl);
        pItem->SetVariable(pVar);
        AddChild(pItem);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::AddChildrenForAIEntityClasses()
{
    assert(m_type == ePropertyAiEntityClasses);

    for (Childs::iterator it = m_childs.begin(); it != m_childs.end(); ++it)
    {
        m_propertyCtrl->DestroyControls(*it);
    }

    RemoveAllChildren();
    m_propertyCtrl->InvalidateCtrl();

    std::set<CString> setSelectedAIEntityClasses;

    CString token;
    int index = 0;
    while (!(token = m_value.Tokenize(" ,", index)).IsEmpty())
    {
        setSelectedAIEntityClasses.insert(token);
    }

    int N = 1;
    char propertyN[100];
    for (std::set<CString>::iterator it = setSelectedAIEntityClasses.begin(); it != setSelectedAIEntityClasses.end(); ++it)
    {
        IVariable* pVar = new CVariable<CString>();
        sprintf_s(propertyN, "Entity Class %d", N++);
        pVar->SetName(propertyN);
        pVar->Set(*it);

        CPropertyItemPtr pItem = new CPropertyItem(m_propertyCtrl);
        pItem->SetVariable(pVar);
        AddChild(pItem);
    }
}

static inline bool AlphabeticalBaseObjectLess(const CBaseObject* p1, const CBaseObject* p2)
{
    return p1->GetName() < p2->GetName();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::PopulateAITerritoriesList()
{
    CVariableEnum<CString>* pVariable = static_cast<CVariableEnum<CString>*>(&*m_pVariable);

    pVariable->SetEnumList(0);
#ifndef USE_SIMPLIFIED_AI_TERRITORY_SHAPE
    pVariable->AddEnumItem("<Auto>", "<Auto>");
#endif
    pVariable->AddEnumItem("<None>", "<None>");

    std::vector<CBaseObject*> vTerritories;
    GetIEditor()->GetObjectManager()->FindObjectsOfType(&CAITerritoryObject::staticMetaObject, vTerritories);
    std::sort(vTerritories.begin(), vTerritories.end(), AlphabeticalBaseObjectLess);

    for (std::vector<CBaseObject*>::iterator it = vTerritories.begin(); it != vTerritories.end(); ++it)
    {
        const CString& name = (*it)->GetName();
        pVariable->AddEnumItem(name, name);
    }

    m_enumList = pVariable->GetEnumList();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::PopulateAIWavesList()
{
    CVariableEnum<CString>* pVariable = static_cast<CVariableEnum<CString>*>(&*m_pVariable);

    pVariable->SetEnumList(0);
    pVariable->AddEnumItem("<None>", "<None>");

    CPropertyItem* pPropertyItem = GetParent()->FindItemByFullName("::AITerritoryAndWave::Territory");
    if (pPropertyItem)
    {
        CString sTerritoryName = pPropertyItem->GetValue();
#ifdef USE_SIMPLIFIED_AI_TERRITORY_SHAPE
        if (sTerritoryName != "<None>")
#else
        if ((sTerritoryName != "<Auto>") && (sTerritoryName != "<None>"))
#endif
        {
            std::vector<CAIWaveObject*> vLinkedAIWaves;

            CBaseObject* pBaseObject = GetIEditor()->GetObjectManager()->FindObject(sTerritoryName);
            if (qobject_cast<CAITerritoryObject*>(pBaseObject))
            {
                CAITerritoryObject* pTerritory = static_cast<CAITerritoryObject*>(pBaseObject);
                pTerritory->GetLinkedWaves(vLinkedAIWaves);
            }

            std::sort(vLinkedAIWaves.begin(), vLinkedAIWaves.end(), AlphabeticalBaseObjectLess);

            for (std::vector<CAIWaveObject*>::iterator it = vLinkedAIWaves.begin(); it != vLinkedAIWaves.end(); ++it)
            {
                const CString& name = (*it)->GetName();
                pVariable->AddEnumItem(name, name);
            }
        }
    }

    m_enumList = pVariable->GetEnumList();
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::CreateInPlaceComboBox(CWnd* pWndParent, DWORD style)
{
    AZ_Assert(!m_cCombo, "InPlaceComboBox already created!");
    if (!m_cCombo)
    {
        CRect nullRc(0, 0, 0, 0);
        DWORD nSorting = (m_pVariable && m_pVariable->GetFlags() & IVariable::UI_UNSORTED) ? 0 : LBS_SORT;

        m_cCombo = new CInPlaceComboBox();
        m_cCombo->SetReadOnly(true);
        m_cCombo->Create(NULL, "", style | nSorting, nullRc, pWndParent, 2);
        m_cCombo->SetUpdateCallback(functor(*this, &CPropertyItem::OnComboSelection));
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::PopulateInPlaceComboBoxFromEnumList()
{
    AZ_Warning("PropertyItem", m_cCombo, "Calling Populate on an InPlaceComboBox not yet created!");
    if (m_cCombo)
    {
        if (m_enumList)
        {
            // hold on to old value
            int nCurSel = m_cCombo->GetCurrentSelection();
            CString oldValue = m_cCombo->GetTextData();

            // clear and repopulate
            m_cCombo->ResetContent();

            for (uint i = 0; cstr sEnumName = m_enumList->GetItemName(i); i++)
            {
                m_cCombo->AddString(sEnumName);
            }

            // put selected value back in if found.
            if (nCurSel != -1)
            {
                m_cCombo->SelectString(oldValue);
            }
        }
        else
        {
            m_cCombo->SetReadOnly(false);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::PopulateInPlaceComboBox(const std::vector<CString>& items)
{
    AZ_Warning("PropertyItem", m_cCombo, "Calling Populate on an InPlaceComboBox not yet created!");
    if (m_cCombo)
    {
        // hold on to old value
        int nCurSel = m_cCombo->GetCurrentSelection();
        CString oldValue = m_cCombo->GetTextData();

        // clear and repopulate
        m_cCombo->ResetContent();
        for (const auto& item : items)
        {
            m_cCombo->AddString(item);
        }

        // put selected value back in if found.
        if (nCurSel != -1)
        {
            m_cCombo->SelectString(oldValue);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::RepositionWindow(CWnd* pWnd, CRect rc)
{
    if (!s_HDWP)
    {
        pWnd->MoveWindow(rc, FALSE);
    }
    else
    {
        s_HDWP = DeferWindowPos(s_HDWP, pWnd->GetSafeHwnd(), 0, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOACTIVATE | SWP_NOZORDER);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::RegisterCtrl(CWnd* pCtrl)
{
    if (pCtrl)
    {
        m_controls.push_back(pCtrl);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::EnableControls(bool bEnable)
{
    for (int i = 0, num = m_controls.size(); i < num; i++)
    {
        CWnd* pWnd = m_controls[i];
        if (pWnd && pWnd->GetSafeHwnd())
        {
            pWnd->EnableWindow((bEnable) ? TRUE : FALSE);
        }
    }
    //KDAB: this can be removed once CPropertyItem is ported to Qt and m_cColorSpline is added to m_controls.
    if (m_cColorSpline)
    {
        m_cColorSpline->setEnabled(bEnable);
    }
}

//////////////////////////////////////////////////////////////////////////
void CPropertyItem::EnableNotifyWithoutValueChange(bool bFlag)
{
    for (int i = 0; i < GetChildCount(); ++i)
    {
        CPropertyItem* item = GetChild(i);
        item->EnableNotifyWithoutValueChange(bFlag);
    }
    m_bForceModified = bFlag;
    if (m_pVariable)
    {
        m_pVariable->EnableNotifyWithoutValueChange(m_bForceModified);
    }
    if (m_cEdit)
    {
        m_cEdit->EnableUpdateOnKillFocus(!m_bForceModified);
    }
}
