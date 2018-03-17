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
#include "AttributeItem.h"
#include <AttributeItem.moc>
#include "AttributeGroup.h"

#include "AttributeView.h"

// EditorCore
#include <Util/Variable.h>
#include <Util/VariablePropertyType.h>

#include <IRenderer.h>
#include <ParticleParams.h>

// QT
#include <QLabel>
#include <QLayout>

#include "Utils.h"
#include "VariableWidgets/QWidgetVector.h"
#include "VariableWidgets/QWidgetInt.h"
#include "VariableWidgets/QColumnWidget.h"
#include "VariableWidgets/QCollapsePanel.h"
#include "VariableWidgets/QCollapseWidget.h"
#include "VariableWidgets/QColorCurve.h"
#include "VariableWidgets/QCurveEditorImp.h"
#include "VariableWidgets/QColorWidgetImp.h"
#include "VariableWidgets/QFileSelectResourceWidget.h"
#include "VariableWidgets/QStringWidget.h"
#include "VariableWidgets/QBoolWidget.h"
#include "VariableWidgets/QEnumWidget.h"
#include "DefaultViewWidget.h"

CAttributeItem::CAttributeItem(QWidget* parent, CAttributeView* attributeView, IVariable* var, const QString& attributePath,
    const CAttributeViewConfig::config::group* configGroup)
    : QWidget(parent)
    , m_var(var)
    , m_attributeView(attributeView)
    , m_nodeDepth(1)
    , m_attributePath(attributePath)
    , m_configGroup(configGroup)
    , m_configItem(nullptr)
    , m_advanced(false)
    , m_hasAdvancedChildren(false)
    , m_defaultLabel(nullptr)
    , m_widget(nullptr)
{
    m_isGroup = false;

    m_layout = new QVBoxLayout(this);

    setContentsMargins(QMargins(0, 0, 0, 0));
    m_layout->setMargin(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    setAutoFillBackground(true);

    // Replace all whitespace with "_"
    const static QRegExp pattern("[ \t]+");
    m_attributePath = m_attributePath.replace(pattern, "_");

    // set layout spacing
    m_layout->setSpacing(0);

    setMouseTracking(true);
    if (var)
    {
        CreateDataType(var);
    }
    if (configGroup)
    {
        CreateDragableGroups(configGroup);
    }

    m_layout->setAlignment(Qt::AlignTop);

    connect(this, &CAttributeItem::SignalUndoPoint, attributeView, &CAttributeView::OnAttributeItemUndoPoint);

    //Remove the next line if you don't want horizontal resizing to break when the left of two side by side panels is open.
    //setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Maximum);
}

CAttributeItem::CAttributeItem(QWidget* parent, CAttributeView* attributeView, IVariable* var, const QString& attributePath, unsigned int nodeDepth, const CAttributeViewConfig::config::group* configGroup, const CAttributeViewConfig::config::item* configItem)
    : QWidget(parent)
    , m_attributeView(attributeView)
    , m_var(var)
    , m_nodeDepth(nodeDepth + 1)
    , m_attributePath(attributePath)
    , m_configGroup(configGroup)
    , m_configItem(configItem)
    , m_advanced(false)
    , m_hasAdvancedChildren(false)
    , m_defaultLabel(nullptr)
    , m_widget(nullptr)
{
    m_isGroup = false;
    m_layout = new QVBoxLayout(this);

    setContentsMargins(QMargins(0, 0, 0, 0));
    m_layout->setMargin(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    setAutoFillBackground(true);

    // Replace all whitespace with "_"
    const static QRegExp pattern("[ \t]+");
    m_attributePath = m_attributePath.replace(pattern, "_");

    // set layout spacing
    m_layout->setSpacing(0);

    setMouseTracking(true);

    if (configItem)
    {
        setOverrideVarName(m_attributeView->GetConfigItemName(configItem->as));
        if (configItem->advanced.compare("y") == 0 ||
            configItem->advanced.compare("yes") == 0 ||
            configItem->advanced.compare("true") == 0 ||
            configItem->advanced.compare("1") == 0)
        {
            m_advanced = true;
            QWidget* w = parent;
            while (w)
            {
                CAttributeItem* item = qobject_cast<CAttributeItem*>(w);
                if (item)
                {
                    item->m_hasAdvancedChildren = true;
                    break;
                }
                w = w->parentWidget();
            }
        }

        setObjectName(configItem->as);

        m_updatecallbacks = "";
        AttributeItemLogicCallbacks::GetCallback(m_attributeView->GetConfigItemCallback(configItem->as), &m_uiLogicUpdateCallback);
        m_updatecallbacks = m_attributeView->GetConfigItemCallback(configItem->as);

        for (auto relation : m_attributeView->GetConfigItemRelations(configItem->as))
        {
            m_VariableRelations.insert(relation.scr, QPair<QString, QString>(relation.slot, relation.dst));
        }
    }
    else
    {
        //it's variable without config item, use variable alias in config to find available alias
        if (var)
        {
            setOverrideVarName(m_attributeView->GetDefaultConfig()->GetVariableAlias(var->GetName()));
        }
    }
    if (var)
    {
        CreateDataType(var);
    }
    if (configGroup)
    {
        CreateCategory(nullptr, nullptr, configGroup);
    }

    //Make sure the Items are not spread out over the panels
    if (m_nodeDepth <= 1)
    {
        m_layout->setAlignment(Qt::AlignTop);
    }

    connect(this, &CAttributeItem::SignalUndoPoint, attributeView, &CAttributeView::OnAttributeItemUndoPoint);

    //Remove the next line if you don't want horizontal resizing to break when the left of two side by side panels is open.
    //setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Maximum);
}

CAttributeItem::CAttributeItem(QWidget* parent, CAttributeView* attributeView, QString message)
    : QWidget(parent)
    , m_attributeView(attributeView)
    , m_var(nullptr)
    , m_nodeDepth(1)
    , m_attributePath("")
    , m_configGroup(nullptr)
    , m_configItem(nullptr)
    , m_advanced(false)
    , m_hasAdvancedChildren(false)
{
    m_layout = new QVBoxLayout(this);

    setContentsMargins(QMargins(0, 0, 0, 0));
    m_layout->setMargin(0);
    m_layout->setContentsMargins(0, 0, 0, 0);

    setAutoFillBackground(true);

    // set layout spacing
    m_layout->setSpacing(0);

    setMouseTracking(true);
    m_widget = new QWidget(this);
    m_widget->setObjectName("Widget");
    QVBoxLayout* layout = new QVBoxLayout(m_widget);

    m_defaultLabel = new QLabel(this);
    m_defaultLabel->setAlignment(Qt::AlignCenter);
    m_defaultLabel->setText(message);
    m_defaultLabel->setFixedHeight(100);

    layout->addWidget(m_defaultLabel, Qt::AlignCenter);
    layout->setAlignment(Qt::AlignCenter);

    m_widget->setLayout(layout);

    m_layout->addWidget(m_widget, Qt::AlignCenter);

    connect(this, &CAttributeItem::SignalUndoPoint, attributeView, &CAttributeView::OnAttributeItemUndoPoint);
}

CAttributeItem::~CAttributeItem()
{
}

IVariable* CAttributeItem::getVar()
{
    return m_var;
}

const IVariable* CAttributeItem::getVar() const
{
    return m_var;
}

QString CAttributeItem::getVarName() const
{
    if (m_overrideVarName.isEmpty() == false)
    {
        return m_overrideVarName;
    }

    return m_var ? QString(m_var->GetName()) : "IVariable=NULL";
}

CAttributeView* CAttributeItem::getAttributeView()
{
    return m_attributeView;
}

void CAttributeItem::setOverrideVarName(QString varName)
{
    m_overrideVarName = varName;
}

bool CAttributeItem::isDefaultValue()
{
    for (CAttributeItem* child : m_children)
    {
        if (!child->isDefaultValue())
        {
            return false;
        }
    }

    if (m_var == nullptr)
    {
        return true;
    }
    else
    {
        return m_var->HasDefaultValue();
    }
}

void CAttributeItem::ResetValueToDefault()
{
    for (CAttributeItem* child : m_children)
    {
        child->ResetValueToDefault();
    }

    if (m_var != nullptr)
    {
        m_var->ResetToDefault();
    }

    emit SignalResetToDefault();
}

void CAttributeItem::ResolveVisibility(const QVector<QString>& visibilityVars)
{
    for (CAttributeItem* child : m_children)
    {
        child->ResolveVisibility(visibilityVars);
    }

    bool isVisible = true;


    // when a group is set to hidden, we hide all members
    if (m_configGroup != NULL)
    {
        // Use GetConfigVisibility Function to get settings from config
        QStringList visibilityList = m_attributeView->GetConfigGroupVisibility(m_configGroup->name).split(";", QString::SplitBehavior::SkipEmptyParts);
        CheckVisibility(visibilityList, isVisible);

    }
    if (m_configItem != NULL)
    {
        // Use GetConfigVisibility Function to get settings from config
        QStringList visibilityList = m_attributeView->GetConfigItemVisibility(m_configItem->as).split(";", QString::SplitBehavior::SkipEmptyParts);
        CheckVisibility(visibilityList, isVisible);
    }

    this->setVisible(isVisible);
}


void CAttributeItem::CheckVisibility(QStringList visibilityList, bool& isVisible)
{
    //for the gpu emitter shape override only
    IVariable* emitterTypeVar = m_attributeView->getVariableFromName("Emitter.Emitter_Type");
    bool overrideShape = false;
    if (emitterTypeVar)
    {
        QString val = "";
        val = emitterTypeVar->GetDisplayValue();
        overrideShape = val.compare("GPU", Qt::CaseInsensitive) == 0;
    }

    for (QString visibilityGroup : visibilityList)
    {
        QStringList IdAndValue = visibilityGroup.split(":", QString::SplitBehavior::SkipEmptyParts);
        // IdAndValue should contain two QStrings. First QString is the varName, Second QString is the values.
        if (IdAndValue.size() != 2)
        {
            continue;
        }
        QString varName = IdAndValue.first();
        if (overrideShape && (varName.compare("Emitter.Emitter_Shape") == 0))
        {
            varName = "Emitter.Emitter_Gpu_Shape";
        }
        IVariable* emitterType = m_attributeView->getVariableFromName(varName);
        QString currentKey = emitterType->GetDisplayValue();

        QStringList listWithSupportedItems = IdAndValue[1].split(",", QString::SplitBehavior::SkipEmptyParts);
        CompareVisibility(isVisible, currentKey, listWithSupportedItems, "all");
        if (!isVisible)
        {
            break;
        }
    }
}

void CAttributeItem::CompareVisibility(bool& isVisible, const QString compareKey, QStringList listWithSupportedItems, QString ignoredKey)
{
    // If visibility is not clarified, set it to visible.
    if (listWithSupportedItems.isEmpty())
    {
        return;
    }
    foreach(const QString &str, listWithSupportedItems)
    {
        // compare gives back 0 when we have a match, -n when we have less characters than visibilityKey and n when we have n more characters than visibilityKey
        int compareGroupKey = str.compare(compareKey, Qt::CaseSensitivity::CaseInsensitive);
        int compareGroupKeyAll = str.compare(ignoredKey, Qt::CaseSensitivity::CaseInsensitive);

        // Value matches and the item is visible, return;
        if (compareGroupKey == 0 ||
            compareGroupKeyAll == 0)
        {
            return;
        }
    }

    isVisible = false;
}


QString CAttributeItem::GetVisibilityValues()
{
    if (!m_configItem)
    {
        return "";
    }
    return m_attributeView->GetConfigItemVisibility(m_configItem->as);
}

bool CAttributeItem::showAdvanced(bool showAdvanced)
{
    if (layout()) //Optimization, don't update the layout while propagating showAdvanced to the children
    {
        layout()->setEnabled(false);
    }
    int visibleCount = 0;
    for (CAttributeItem* child : m_children)
    {
        if (child->showAdvanced(showAdvanced))
        {
            visibleCount++;
        }
    }

    if (layout())
    {
        layout()->setEnabled(true);
    }

    if (isAdvanced() && m_nodeDepth > 1)
    {
        setVisible(showAdvanced);
        return showAdvanced;
    }
    return true;
}

bool CAttributeItem::isChildOf(CAttributeItem* item) const
{
    if (this == item)
    {
        return true;
    }

    for (CAttributeItem* child : m_children)
    {
        if (child->isChildOf(item))
        {
            return true;
        }
    }

    return false;
}

void CAttributeItem::ForEachRelation(std::function<bool(QString dst, QString slot)> func)
{
    CAttributeItem* relationSource = this;
    if (!m_configItem)
    {
        //Get parent responsible for the relations
        QWidget* w = parentWidget();
        while (w != nullptr)
        {
            if (qobject_cast<CAttributeItem*>(w) != nullptr)
            {
                if (qobject_cast<CAttributeItem*>(w)->hasXMLConfigItem())
                {
                    break;
                }
            }
            w = w->parentWidget();
        }
        relationSource = qobject_cast<CAttributeItem*>(w);
        if (relationSource == nullptr)
        {
            return;
        }
    }

    QList<QPair<QString, QString> > values = relationSource->m_VariableRelations.values(getAttributeView()->GetVariablePath(m_var));
    for (int i = 0; i < values.size(); ++i)
    {
        if (func(values.at(i).first, values.at(i).second))
        {
            break;
        }
    }
}

QMap<QString, QString> CAttributeItem::GetRelations()
{
    QMap<QString, QString> relations;
    ForEachRelation([&relations](QString slot, QString dst)
    {
        relations.insert(slot, dst);
        return false;
    });
    return relations;
}

const QMultiMap<QString, QPair<QString, QString> > CAttributeItem::GetVariableRelations()
{
    return m_VariableRelations;
}

void CAttributeItem::addCallback(Callback cb)
{
    m_callbacks.push_back(cb);
}

void CAttributeItem::updateCallbacks(const bool& recursive)
{
    // Call callbacks
    for (Callback cb : m_callbacks)
    {
        cb();
    }

    if (!recursive)
    {
        return;
    }

    for (CAttributeItem* child : m_children)
    {
        child->updateCallbacks(true);
    }
}

QString CAttributeItem::getAttributePath(const bool& withLibraryName) const
{
    if (withLibraryName)
    {
        return m_attributeView->getCurrentLibraryName() + ":" + m_attributePath;
    }

    return m_attributePath;
}

const CAttributeItem::AttributeList& CAttributeItem::getChildren() const
{
    return m_children;
}

void CAttributeItem::ForEachChild(std::function<void(CAttributeItem* child)> func)
{
    for (int i = 0; i < m_children.size(); i++)
    {
        func(m_children[i]);
    }
}

CAttributeItem* CAttributeItem::findItemByPath(const QString& path)
{
    if (path == getAttributePath(false))
    {
        return this;
    }

    CAttributeItem* item = nullptr;

    for (CAttributeItem* child : m_children)
    {
        item = child->findItemByPath(path);
        if (item)
        {
            break;
        }
    }

    return item;
}

CAttributeItem* CAttributeItem::findItemByVar(IVariable* var)
{
    if (var == m_var)
    {
        return this;
    }

    CAttributeItem* item = nullptr;

    for (CAttributeItem* child : m_children)
    {
        item = child->findItemByVar(var);
        if (item)
        {
            break;
        }
    }

    return item;
}

// Utility functions
QString CAttributeItem::getVarName(IVariable* var, bool addDots)
{
    if (m_overrideVarName.isEmpty() == false)
    {
        return m_overrideVarName;
    }

    assert(var && "Invalid variable pointer");
    QString title(var->GetName());

    // Sanity check
    assert(!title.isEmpty() && "Variable is missing a name!");

    return title + (addDots ? "........................" : "");
}

void CAttributeItem::addToLayout(const Prop::Description* desc, IVariable* var, QWidget* widget)
{
    QCollapseWidget* collapseWidget = var->GetNumVariables() > 0 ? CreateCategory(desc, var, nullptr) : NULL;
    if (collapseWidget)
    {
        ((QColumnWidget*)widget)->SetCollapsible(true);
        collapseWidget->getMainLayout()->addWidget(widget, Qt::AlignRight);
    }
    else
    {
        ((QColumnWidget*)widget)->SetCollapsible(false);
        m_layout->addWidget(widget, Qt::AlignRight);
    }
}

void CAttributeItem::CreateDataType(IVariable* var)
{
    IVariable::EType type = var->GetType();
    IVariable::EDataType dataType = (IVariable::EDataType)var->GetDataType();

    Prop::Description desc(var);

    switch (desc.m_type)
    {
    case ePropertyString:
        CreateString(&desc, var);
        break;
    case ePropertyFloat:
        CreateFloat(&desc, var);
        break;
    case ePropertyInt:
        CreateInt(&desc, var);
        break;
    case ePropertyBool:
        CreateBool(&desc, var);
        break;
    case ePropertyVector2:
        CreateVector2(&desc, var);
        break;
    case ePropertyVector:
        CreateVector3(&desc, var);
        break;
    case ePropertyVector4:
        CreateVector4(&desc, var);
        break;
    case ePropertySelection:
        CreateEnum(&desc, var);
        break;
    case ePropertyFloatCurve:
        CreateCurveFloat(&desc, var);
        break;
    case ePropertyColorCurve:
        CreateCurveColor(&desc, var);
        break;
    case ePropertyColor:
        CreateColor(&desc, var);
        break;
    case ePropertyTexture:
    case ePropertyModel:
    case ePropertyMaterial:
    case ePropertyAudioTrigger:
        CreateSelectResource(&desc, var);
        break;
    default:
    {
        if (m_nodeDepth == 1)
        {
            if (type == IVariable::ARRAY)
            {
                CreateCategory(&desc, var, nullptr);
            }
        }
        else
        {
            // Check if its a type and not a simple container
            // If its not a simple container, it means its an unimplemented type
            QString extraInfo;
            if (desc.m_type != ePropertyInvalid)
            {
                extraInfo = QString().sprintf(" - Unimplemented type# %d", desc.m_type);
            }

            addToLayout(&desc, var, new QColumnWidget(this, getVarName(var, false) + extraInfo, NULL));
        }
        break;
    }
    }
}

void CAttributeItem::CreateChildAttributes(QWidget* parent, QVBoxLayout* layout, IVariable* pVar, const CAttributeViewConfig::config::group* grp)
{
    if (pVar)
    {
        CreateChildAttributes_Variable(pVar, parent, layout);
    }
    else if (grp)
    {
        CreateChildAttributes_ConfigGroup(grp, parent, layout);
    }
}

void CAttributeItem::CreateChildAttributes_ConfigGroup(const CAttributeViewConfig::config::group* grp, QWidget* parent, QVBoxLayout* layout)
{
    for (auto & id : grp->order)
    {
        switch (id.type)
        {
        case CAttributeViewConfig::config::group::order_type::Group:
        {
            // add group
            QString attributePath = m_attributePath;
            CAttributeItem* widget = new CAttributeItem(parent, m_attributeView, nullptr, attributePath, m_nodeDepth, &grp->groups[id.index], nullptr);
            m_children.push_back(widget);

            if (layout)
            {
                layout->addWidget(widget);
            }
        } break;
        case CAttributeViewConfig::config::group::order_type::Item:
        {
            auto& item = grp->items[id.index];
            if (m_attributeView->getConfigVariableMap().contains(item.as) == false)
            {
                // attribute not found, make a placeholder
                QString extraInfo;
                extraInfo = QString().sprintf("(@ %s)", grp->items[id.index].as.toUtf8().data());
                m_layout->addWidget(new QColumnWidget(this, extraInfo, NULL));
            }
            else
            {
                // attribute found, make a sub attribute item
                auto var = m_attributeView->getConfigVariableMap()[item.as];

                QString attributePath = getAttributeView()->GetVariablePath(var);
                if (m_attributeView->isVariableIgnored(attributePath))
                {
                    continue;
                }
                CAttributeItem* widget = new CAttributeItem(parent, m_attributeView, var, attributePath, m_nodeDepth, nullptr, &item);
                m_children.push_back(widget);

                if (layout)
                {
                    layout->addWidget(widget);
                }
            }
        } break;
        }
    }
}

void CAttributeItem::CreateChildAttributes_Variable(IVariable* pVar, QWidget* parent, QVBoxLayout* layout)
{
    assert(pVar);
    if (!pVar)
    {
        return;
    }

    for (int i = 0; i < pVar->GetNumVariables(); i++)
    {
        IVariable* var = pVar->GetVariable(i);
        assert(var->GetName() != nullptr && var->GetName().length() > 0);

        QString attributePath = getAttributeView()->GetVariablePath(var);
        if (m_attributeView->isVariableIgnored(attributePath))
        {
            continue;
        }

        CAttributeViewConfig::config::item const* item = nullptr;
        if (m_configItem)
        {
            auto itemIt = m_configItem->items.find(attributePath);
            if (itemIt != m_configItem->items.end())
            {
                item = &*itemIt;
            }
        }

        CAttributeItem* widget = new CAttributeItem(parent, m_attributeView, var, attributePath, m_nodeDepth, nullptr, item);
        m_children.push_back(widget);

        if (layout)
        {
            layout->addWidget(widget, Qt::AlignRight);
        }
    }
}

QCollapseWidget* CAttributeItem::CreateCategory(const Prop::Description* desc, IVariable* var, const CAttributeViewConfig::config::group* grp)
{
    QString title;

    if (var)
    {
        title = getVarName(var, false);
    }
    if (grp)
    {
        title = grp->name;
    }

    if (m_nodeDepth > 1)
    {
        if (var)
        {
            // use collapsed widgets for variables
            QCollapseWidget* widget = new QCollapseWidget(this, this);
            m_layout->addWidget(widget);
            CreateChildAttributes(widget, widget->getChildLayout(), var, grp);
            return widget;
        }
        if (grp)
        {
            // use a simple label for groups
            QLabel* lblGroup = new QLabel(this);
            QVBoxLayout* layout = new QVBoxLayout();
            QWidget* widget = new QWidget();

            // make the label bold
            QFont fnt = lblGroup->font();
            lblGroup->setObjectName("AttributeCategoryLabel");
            fnt.setBold(true);
            lblGroup->setFont(fnt);

            lblGroup->setText(title);
            widget->setLayout(layout);
            layout->setContentsMargins(0, 0, 0, 0);
            CreateChildAttributes(widget, layout, var, grp);
            m_layout->addWidget(lblGroup);
            m_layout->addWidget(widget);
            m_isGroup = true;
            setObjectName(title);
        }
    }

    if (m_nodeDepth == 1)
    {
        QVBoxLayout* layout = new QVBoxLayout();

        QWidget* widget = new QWidget();
        m_layout->addWidget(widget, Qt::AlignRight);
        CreateChildAttributes(widget, layout, var, grp);
        widget->setLayout(layout);
        widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    }

    return NULL;
}
bool CAttributeItem::InsertDragableGroup(const CAttributeViewConfig::config::item* item, const CAttributeViewConfig::config::group* grp, QWidget* insertWidget, int index)
{
    // Disable Call Back State
    //m_attributeView->OnStartReload();
    QVector<CAttributeItem*>::iterator iter = m_children.begin();

    while (iter != m_children.end())
    {
        if ((*iter)->m_configGroup != nullptr &&  grp != nullptr && (*iter)->m_configGroup->name == grp->name)
        {
            // Already has the group in the view
            return false;
        }
        if ((*iter)->m_configItem != nullptr && item != nullptr && (*iter)->m_configItem->as == item->as)
        {
            // Already has the group in the view
            return false;
        }
        iter++;
    }

    AttributeItemLogicCallbacks::SetCallbacksEnabled(false);
    emit m_attributeView->SignalIgnoreAttributeRefresh(true);

    if (!m_widget)
    {
        m_widget = new QWidget(this);
        m_widget->setObjectName("Widget");

        QVBoxLayout* layout = new QVBoxLayout(m_widget);
        m_layout->addWidget(m_widget, Qt::AlignRight);
        m_widget->setLayout(layout);
        m_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    }

    int insertIndex = 0;
    iter = m_children.begin();


    while (iter != m_children.end())
    {
        if (*iter == insertWidget)
        {
            iter += index;
            insertIndex += index;
            break;
        }
        iter++;
        insertIndex++;
    }

    QVBoxLayout* layout = static_cast<QVBoxLayout*>(m_widget->layout());

    if (item)
    {
        if (m_attributeView->getConfigVariableMap().contains(item->as) == false)
        {
            // attribute not found, make a placeholder
            QString extraInfo;
            extraInfo = QString().sprintf("(@ %s)", item->as.toUtf8().data());
            layout->addWidget(new QColumnWidget(this, extraInfo, NULL));
        }
        else
        {
            // attribute found, make a sub attribute item
            auto var = m_attributeView->getConfigVariableMap()[item->as];

            QString attributePath = getAttributeView()->GetVariablePath(var);
            if (m_attributeView->isVariableIgnored(attributePath))
            {
                return false;
            }
            CAttributeItem* itemWidget = new CAttributeItem(nullptr, m_attributeView, var, attributePath, m_nodeDepth, nullptr, item);
            m_children.insert(iter, itemWidget);
            CAttributeGroup* dragableGroup = new CAttributeGroup(this, itemWidget);
            itemWidget->setParent(dragableGroup);
            dragableGroup->SetConfigItem(item);
            if (layout)
            {
                layout->insertWidget(insertIndex, dragableGroup);
            }
        }
    }
    if (grp)
    {
        // add group
        QString attributePath = m_attributePath;
        if (!attributePath.isEmpty())
        {
            attributePath.append(".");
            attributePath.append(grp->name);
        }
        CAttributeItem* groupWidget = new CAttributeItem(nullptr, m_attributeView, nullptr, attributePath, m_nodeDepth, grp, nullptr);
        m_children.insert(iter, groupWidget);
        CAttributeGroup* dragableGroup = new CAttributeGroup(this, groupWidget);
        groupWidget->setParent(dragableGroup);
        dragableGroup->SetConfigGroup(grp);
        if (layout)
        {
            layout->insertWidget(insertIndex, dragableGroup);
        }
    }

    // Disable Call Back State
    AttributeItemLogicCallbacks::SetCallbacksEnabled(true);
    emit m_attributeView->SignalIgnoreAttributeRefresh(false);
    emit m_attributeView->SignalRefreshAttributes();
    return true;
}


QCollapseWidget* CAttributeItem::CreateDragableGroups(const CAttributeViewConfig::config::group* grp)
{
    assert(m_nodeDepth == 1); // The function only applys for nodeDepth == 1
    QVBoxLayout* layout = new QVBoxLayout();
    QWidget* widget = new QWidget();
    m_layout->addWidget(widget, Qt::AlignRight);

    for (auto & id : grp->order)
    {
        switch (id.type)
        {
        case CAttributeViewConfig::config::group::order_type::Group:
        {
            // add group

            CAttributeGroup* dragableGroup = new CAttributeGroup(this);
            QString attributePath = m_attributePath;
            if (!attributePath.isEmpty())
            {
                attributePath.append(".");
                attributePath.append(&grp->groups[id.index].name);
            }
            CAttributeItem* groupWidget = new CAttributeItem(dragableGroup, m_attributeView, nullptr, attributePath, m_nodeDepth, &grp->groups[id.index], nullptr);
            dragableGroup->SetContent(groupWidget);
            m_children.push_back(groupWidget);
            dragableGroup->setObjectName(QString("DragGroup_") + grp->groups[id.index].name);
            dragableGroup->SetConfigGroup(&grp->groups[id.index]);

            if (layout)
            {
                layout->addWidget(dragableGroup);
            }
        } break;
        case CAttributeViewConfig::config::group::order_type::Item:
        {
            auto& item = grp->items[id.index];
            if (m_attributeView->getConfigVariableMap().contains(item.as) == false)
            {
                // attribute not found, make a placeholder
                QString extraInfo;
                extraInfo = QString().sprintf("(@ %s)", grp->items[id.index].as.toUtf8().data());
                m_layout->addWidget(new QColumnWidget(this, extraInfo, NULL));
            }
            else
            {
                // attribute found, make a sub attribute item
                auto var = m_attributeView->getConfigVariableMap()[item.as];

                QString attributePath = getAttributeView()->GetVariablePath(var);
                if (m_attributeView->isVariableIgnored(attributePath))
                {
                    continue;
                }
                CAttributeGroup* dragableGroup = new CAttributeGroup(this);
                CAttributeItem* itemWidget = new CAttributeItem(dragableGroup, m_attributeView, var, attributePath, m_nodeDepth, nullptr, &item);
                dragableGroup->SetContent(itemWidget);
                m_children.push_back(itemWidget);
                dragableGroup->SetConfigItem(&item);
                dragableGroup->setObjectName(QString("DragGroup_") + item.name);

                if (layout)
                {
                    layout->addWidget(dragableGroup);
                }
            }
        } break;
        }
    }
    widget->setLayout(layout);
    widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    m_widget = widget;
    m_widget->setObjectName("Widget");

    return NULL;
}

void CAttributeItem::RemoveDragableGroup(CAttributeGroup* group)
{
    if (m_widget)
    {
        for (int i = 0; i < m_children.size(); i++)
        {
            if (m_children[i] == group->GetContent())
            {
                m_children.remove(i);
                break;
            }
        }
        group->hide();
        m_widget->layout()->removeWidget(group);
    }
}

void CAttributeItem::ZoomToAttribute(QPoint pos)
{
    m_attributeView->ZoomToAttribute(pos);
}

void CAttributeItem::GetEmitterPath(QString& fullname)
{
    emit m_attributeView->SignalGetCurrentTabName(fullname);
}

void CAttributeItem::CreateString(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyString);
    assert(var);

    QStringWidget* widget = new QStringWidget(this);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::CreateInt(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyInt);
    assert(var);

    QWidgetInt* widget = new QWidgetInt(this);
    widget->setVar(var);

    // set initial value
    const int val = var->GetDisplayValue().toInt();
    widget->Set(val, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::CreateBool(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyBool);
    assert(var);

    QBoolWidget* widget = new QBoolWidget(this);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::CreateFloat(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyFloat);

    assert(var);

    float value = 0;
    var->Get(value);

    QWidgetVector* widget = new QWidgetVector(this, 1);
    widget->setVar(var);
    widget->SetComponent(0, value,
        desc->m_rangeMin, desc->m_rangeMax,
        desc->m_bHardMin, desc->m_bHardMax, desc->m_step);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::CreateVector2(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyVector2);
    assert(var);

    // Fetch data as string instead of Vec2_tpl to address the case where there might be a mismatch between the data-type (TRangeParam) from
    // which the value is being fetched, and a Vec2_tpl
    QString str;
    var->Get(str);
    Vec2 v;
    int numScanned = azsscanf(str.toUtf8().data(), "%f,%f", &v.x, &v.y);
    if (numScanned != 2)
    {
        return;
    }

    QWidgetVector* widget = new QWidgetVector(this, 2);
    widget->setVar(var);

    // set initial value(s)
    widget->SetComponent(0, v.x, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);
    widget->SetComponent(1, v.y, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::CreateVector3(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyVector);
    assert(var);

    Vec3 t;
    var->Get(t);
    QWidgetVector* widget = new QWidgetVector(this, 3);

    // set variable to modify
    widget->setVar(var);

    // set initial value(s)
    widget->SetComponent(0, t.x, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);
    widget->SetComponent(1, t.y, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);
    widget->SetComponent(2, t.z, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::CreateVector4(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyVector4);
    assert(var);

    Vec4 t;
    var->Get(t);
    QWidgetVector* widget = new QWidgetVector(this, 4);

    // set variable to modify
    widget->setVar(var);

    // set initial value(s)
    widget->SetComponent(0, t.x, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);
    widget->SetComponent(1, t.y, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);
    widget->SetComponent(2, t.z, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);
    widget->SetComponent(3, t.w, desc->m_rangeMin, desc->m_rangeMax, desc->m_bHardMin, desc->m_bHardMax, desc->m_step);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::CreateEnum(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertySelection);
    assert(var);

    QEnumWidget* widget = new QEnumWidget(this);
    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));

}

void CAttributeItem::CreateCurveFloat(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyFloatCurve);
    assert(var);

    QCurveEditorImp* widget = new QCurveEditorImp(this);
    widget->setVar(var);
    widget->setMinimumHeight(96);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::CreateCurveColor(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyColorCurve);
    assert(var);

    QColorCurve* widget = new QColorCurve(this, new QGradientSwatchWidget(SCurveEditorContent(), { QGradientStop(0.0, QColor(255, 255, 255, 255)), QGradientStop(1.0, QColor(255, 255, 255, 255)) }, this),
        new QGradientSwatchWidget(SCurveEditorContent(), { QGradientStop(0.0, QColor(255, 255, 255, 255)), QGradientStop(1.0, QColor(255, 255, 255, 255)) }, this));
    widget->setVar(var);
    //widget->setMinimumHeight(36);
    widget->setContentsMargins(0, 0, 0, 5);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::CreateColor(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyColor);
    assert(var);

    QColorWidgetImp* widget = new QColorWidgetImp(this);
    widget->setVar(var);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}


void CAttributeItem::CreateSelectResource(const Prop::Description* desc, IVariable* var)
{
    assert(desc);
    assert(desc->m_type == ePropertyTexture
        || desc->m_type == ePropertyModel
        || desc->m_type == ePropertyMaterial
        || desc->m_type == ePropertyAudioTrigger);
    assert(var);

    QFileSelectResourceWidget* widget = new QFileSelectResourceWidget(this, m_attributeView, desc->m_type);
    widget->setVar(var);

    addToLayout(desc, var, new QColumnWidget(this, getVarName(var, false), widget));
}

void CAttributeItem::resetItem()
{
    QLayoutItem* item = nullptr;
    while ((item = m_widget->layout()->takeAt(0)) != nullptr)
    {
        m_widget->layout()->removeWidget(item->widget());
        m_widget->layout()->removeItem(item);
        item->widget()->setParent(nullptr);
        delete item->widget();
        delete item;
    }
    m_children.clear();
    m_defaultLabel = new QLabel(this);
    m_defaultLabel->setAlignment(Qt::AlignCenter);
    m_defaultLabel->setText("Fill the panel by dragging parameter in.");
    m_defaultLabel->setMinimumHeight(100);

    if (m_widget)
    {
        m_widget->layout()->addWidget(m_defaultLabel);
        m_widget->layout()->setAlignment(Qt::AlignCenter);
        m_layout->addWidget(m_widget);
    }
    else
    {
        m_widget = new QWidget(this);
        QVBoxLayout* layout = new QVBoxLayout(m_widget);
        layout->addWidget(m_defaultLabel, Qt::AlignCenter);
        layout->setAlignment(Qt::AlignCenter);
    }

    m_defaultLabel->show();
}

bool CAttributeItem::eventFilter(QObject* o, QEvent* e)
{
    QWidget* widget = static_cast<QWidget*>(o);
    if (widget && !widget->hasFocus() && e->type() == QEvent::Wheel)
    {
        event(e);
        return true;
    }

    return false;
}

void CAttributeItem::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void CAttributeItem::disable()
{
    setDisabled(true);
    for (unsigned int i = 0; i < m_children.count(); i++)
    {
        m_children[i]->disable();
    }
}

void CAttributeItem::enable()
{
    setDisabled(false);
    for (unsigned int i = 0; i < m_children.count(); i++)
    {
        m_children[i]->enable();
    }
}

bool CAttributeItem::isGroup()
{
    return m_isGroup;
}

QString CAttributeItem::getUpdateCallbacks()
{
    return m_updatecallbacks;
}

bool CAttributeItem::hasCallback()
{
    return m_updatecallbacks.size() > 0;
}
