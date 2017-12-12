#include "stdafx.h"

#include "ReflectedPropertyCtrl.h"
#include <AzToolsFramework/UI/PropertyEditor/ComponentEditorHeader.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AZCore/Component/ComponentApplicationBus.h>
#include "ReflectedPropertyItem.h"
#include "PropertyCtrl.h"
#include "ReflectedVar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>

ReflectedPropertyControl::ReflectedPropertyControl(QWidget *parent /*= nullptr*/, Qt::WindowFlags windowFlags /*= Qt::WindowFlags()*/)
    : QWidget(parent, windowFlags)
    , AzToolsFramework::IPropertyEditorNotify()
    , m_filterLineEdit(nullptr)
    , m_filterWidget(nullptr)
    , m_titleLabel(nullptr)
    , m_bEnableCallback(true)
    , m_updateVarFunc(nullptr)
    , m_updateObjectFunc(nullptr)
    , m_selChangeFunc(nullptr)
    , m_undoFunc(nullptr)
    , m_bStoreUndoByItems(true)
    , m_bForceModified(false)
    , m_groupProperties(false)
    , m_sortProperties(false)
    , m_bSendCallbackOnNonModified(true)
    , m_initialized(false)
{
    EBUS_EVENT_RESULT(m_serializeContext, AZ::ComponentApplicationBus, GetSerializeContext);
    AZ_Assert(m_serializeContext, "Serialization context not available");
    qRegisterMetaType<IVariable*>("IVariablePtr");

    m_editor = new AzToolsFramework::ReflectedPropertyEditor(nullptr);
    m_editor->SetAutoResizeLabels(true);

    m_titleLabel = new QLabel;
    m_titleLabel->hide();

    m_filterWidget = new QWidget;
    QLabel *label = new QLabel(tr("Search"));
    m_filterLineEdit = new QLineEdit;
    QHBoxLayout *filterlayout = new QHBoxLayout(m_filterWidget);
    filterlayout->addWidget(label);
    filterlayout->addWidget(m_filterLineEdit);
    connect(m_filterLineEdit, &QLineEdit::textChanged, this, &ReflectedPropertyControl::RestrictToItemsContaining);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_titleLabel, 0, Qt::AlignHCenter);
    mainLayout->addWidget(m_filterWidget);
    mainLayout->addWidget(m_editor, 1);

    SetShowFilterWidget(false);
}

void ReflectedPropertyControl::Setup(bool showScrollbars /*= true*/, int labelWidth /*= 150*/)
{
    if (!m_initialized)
    {
        m_editor->Setup(m_serializeContext, this, showScrollbars, labelWidth);
        m_editor->SetSelectionEnabled(true);
        m_initialized = true;
    }
}



ReflectedPropertyItem* ReflectedPropertyControl::AddVarBlock(CVarBlock *varBlock, const char *szCategory /*= nullptr*/)
{
    AZ_Assert(m_initialized, "ReflectedPropertyControl not initialized. Setup must be called first.");
    
    if (!varBlock)
        return nullptr;

    m_pVarBlock = varBlock;

    if (!m_root)
    {
        m_root = new ReflectedPropertyItem(this, nullptr);
        m_rootContainer.reset(new CPropertyContainer(szCategory ? AZStd::string(szCategory) : AZStd::string()));
        m_rootContainer->SetAutoExpand(true);
        m_editor->AddInstance(m_rootContainer.get());
    }

    
    AZStd::vector<IVariable*> variables(varBlock->GetNumVariables());

    //Copy variables into vector
    int n = 0;
    std::generate(variables.begin(), variables.end(), [&n, varBlock]{return varBlock->GetVariable(n++); });

    //filter list based on search string
    if (!m_filterString.isEmpty())
    {
        auto it = std::copy_if(variables.begin(), variables.end(), variables.begin(), [this](IVariable *var){return QString(var->GetName()).toLower().contains(m_filterString); });
        variables.resize(std::distance(variables.begin(), it));  // shrink container to new size
    }

    //sorting if needed.  sort first when grouping to make grouping easier
    if (m_sortProperties || m_groupProperties)
    {
        std::sort(variables.begin(), variables.end(), [](IVariable *var1, IVariable *var2) {return QString::compare(var1->GetName(), var2->GetName(), Qt::CaseInsensitive) <=0; } );
    }

    CPropertyContainer *parentContainer = m_rootContainer.get();
    ReflectedPropertyItem *parentItem = m_root;
    QString currentGroupName;
    for (auto var : variables)
    {      
        if (m_groupProperties)
        {
            //check to see if this item starts with same letter as last. If not, create a new group for it.
            const QString groupName = var->GetName().toUpper().left(1);
            if (groupName != currentGroupName)
            {
                currentGroupName = groupName;
                //make new group be the parent for this item
                parentItem = new ReflectedPropertyItem(this, parentItem);

                parentContainer = new CPropertyContainer(groupName.toLatin1().data());
                parentContainer->SetAutoExpand(false);
                m_rootContainer->AddProperty(parentContainer);
            }
        }
        ReflectedPropertyItemPtr childItem = new ReflectedPropertyItem(this, parentItem);
        childItem->SetVariable(var);
        CReflectedVar *reflectedVar = childItem->GetReflectedVar();
        parentContainer->AddProperty(reflectedVar);
    }
    m_editor->QueueInvalidation(AzToolsFramework::Refresh_EntireTree);

    return parentItem;
}

//////////////////////////////////////////////////////////////////////////
static void AddVariable(CVariableBase& varArray, CVariableBase& var, const char* varName, const char* humanVarName, const char* description, IVariable::OnSetCallback func, void* pUserData, char dataType = IVariable::DT_SIMPLE)
{
    if (varName)
    {
        var.SetName(varName);
    }
    if (humanVarName)
    {
        var.SetHumanName(humanVarName);
    }
    if (description)
    {
        var.SetDescription(description);
    }
    var.SetDataType(dataType);
    var.SetUserData(QVariant::fromValue<void *>(pUserData));
    if (func)
    {
        var.AddOnSetCallback(func);
    }
    varArray.AddVariable(&var);
}

void ReflectedPropertyControl::CreateItems(XmlNodeRef node)
{
    CVarBlockPtr out;
    CreateItems(node, out, nullptr);
}

void ReflectedPropertyControl::CreateItems(XmlNodeRef node, CVarBlockPtr& outBlockPtr, IVariable::OnSetCallback func, bool splitCamelCaseIntoWords)
{
    SelectItem(0);

    outBlockPtr = new CVarBlock;
    for (int i = 0, iGroupCount(node->getChildCount()); i < iGroupCount; ++i)
    {
        XmlNodeRef groupNode = node->getChild(i);

        CSmartVariableArray group;
        group->SetName(groupNode->getTag());
        group->SetHumanName(groupNode->getTag());
        group->SetDescription("");
        group->SetDataType(IVariable::DT_SIMPLE);
        outBlockPtr->AddVariable(&*group);

        for (int k = 0, iChildCount(groupNode->getChildCount()); k < iChildCount; ++k)
        {
            XmlNodeRef child = groupNode->getChild(k);

            const char* type;
            if (!child->getAttr("type", &type))
            {
                continue;
            }

            // read parameter description from the tip tag and from associated console variable
            QString strDescription;
            child->getAttr("tip", strDescription);
            QString strTipCVar;
            child->getAttr("TipCVar", strTipCVar);
            if (!strTipCVar.isEmpty())
            {
                strTipCVar.replace("*", child->getTag());
                if (ICVar* pCVar = gEnv->pConsole->GetCVar(strTipCVar.toLatin1().data()))
                {
                    if (!strDescription.isEmpty())
                    {
                        strDescription += QString("\r\n");
                    }
                    strDescription = pCVar->GetHelp();

#ifdef FEATURE_SVO_GI
                    // Hide or unlock experimental items
                    if ((pCVar->GetFlags() & VF_EXPERIMENTAL) && !gSettings.sExperimentalFeaturesSettings.bTotalIlluminationEnabled && strstr(groupNode->getTag(), "Total_Illumination"))
                    {
                        continue;
                    }
#endif
                }
            }
            QString humanReadableName;
            child->getAttr("human", humanReadableName);
            if (humanReadableName.isEmpty())
            {
                humanReadableName = child->getTag();

                if (splitCamelCaseIntoWords)
                {
                    for (int i = 1; i < humanReadableName.length() - 1; i++)
                    {
                        // insert spaces between words
                        if ((humanReadableName[i - 1].isLower() && humanReadableName[i].isUpper()) || (humanReadableName[i + 1].isLower() && humanReadableName[i - 1].isUpper() && humanReadableName[i].isUpper()))
                        {
                            humanReadableName.insert(i++, ' ');
                        }

                        // convert single upper cases letters to lower case
                        if (humanReadableName[i].isUpper() && humanReadableName[i + 1].isLower())
                        {
                            humanReadableName[i] = humanReadableName[i].toLower();
                        }
                    }
                }
            }

            void* pUserData = (void*)((i << 16) | k);

            if (!_stricmp(type, "int"))
            {
                CSmartVariable<int> intVar;
                AddVariable(group, intVar, child->getTag(), humanReadableName.toLatin1().data(), strDescription.toLatin1().data(), func, pUserData);
                int nValue(0);
                if (child->getAttr("value", nValue))
                {
                    intVar->Set(nValue);
                }

                int nMin(0), nMax(0);
                if (child->getAttr("min", nMin) && child->getAttr("max", nMax))
                {
                    intVar->SetLimits(nMin, nMax);
                }
            }
            else if (!stricmp(type, "float"))
            {
                CSmartVariable<float> floatVar;
                AddVariable(group, floatVar, child->getTag(), humanReadableName.toLatin1().data(), strDescription.toLatin1().data(), func, pUserData);
                float fValue(0.0f);
                if (child->getAttr("value", fValue))
                {
                    floatVar->Set(fValue);
                }

                float fMin(0), fMax(0);
                if (child->getAttr("min", fMin) && child->getAttr("max", fMax))
                {
                    floatVar->SetLimits(fMin, fMax);
                }
            }
            else if (!stricmp(type, "vector"))
            {
                CSmartVariable<Vec3> vec3Var;
                AddVariable(group, vec3Var, child->getTag(), humanReadableName.toLatin1().data(), strDescription.toLatin1().data(), func, pUserData);
                Vec3 vValue(0, 0, 0);
                if (child->getAttr("value", vValue))
                {
                    vec3Var->Set(vValue);
                }
            }
            else if (!stricmp(type, "bool"))
            {
                CSmartVariable<bool> bVar;
                AddVariable(group, bVar, child->getTag(), humanReadableName.toLatin1().data(), strDescription.toLatin1().data(), func, pUserData);
                bool bValue(false);
                if (child->getAttr("value", bValue))
                {
                    bVar->Set(bValue);
                }
            }
            else if (!stricmp(type, "texture"))
            {
                CSmartVariable<QString> textureVar;
                AddVariable(group, textureVar, child->getTag(), humanReadableName.toLatin1().data(), strDescription.toLatin1().data(), func, pUserData, IVariable::DT_TEXTURE);
                const char* textureName;
                if (child->getAttr("value", &textureName))
                {
                    textureVar->Set(textureName);
                }
            }
            else if (!stricmp(type, "material"))
            {
                CSmartVariable<QString> materialVar;
                AddVariable(group, materialVar, child->getTag(), humanReadableName.toLatin1().data(), strDescription.toLatin1().data(), func, pUserData, IVariable::DT_MATERIAL);
                const char* materialName;
                if (child->getAttr("value", &materialName))
                {
                    materialVar->Set(materialName);
                }
            }
            else if (!stricmp(type, "color"))
            {
                CSmartVariable<Vec3> colorVar;
                AddVariable(group, colorVar, child->getTag(), humanReadableName.toLatin1().data(), strDescription.toLatin1().data(), func, pUserData, IVariable::DT_COLOR);
                ColorB color;
                if (child->getAttr("value", color))
                {
                    ColorF colorLinear = ColorGammaToLinear(QColor(color.r, color.g, color.b));
                    Vec3 colorVec3(colorLinear.r, colorLinear.g, colorLinear.b);
                    colorVar->Set(colorVec3);
                }
            }
        }
    }

    AddVarBlock(outBlockPtr);

    InvalidateCtrl();
}

void ReflectedPropertyControl::ReplaceVarBlock(IVariable *categoryItem, CVarBlock *varBlock)
{
    assert(m_root);
    ReflectedPropertyItem *pCategoryItem = m_root->findItem(categoryItem);
    if (pCategoryItem)
    {
        pCategoryItem->ReplaceVarBlock(varBlock);
        m_editor->QueueInvalidation(AzToolsFramework::Refresh_EntireTree);
    }
}

void ReflectedPropertyControl::ReplaceRootVarBlock(CVarBlock* newVarBlock)
{
    const AZStd::string category = m_rootContainer->m_varName;
    RemoveAllItems();
    AddVarBlock(newVarBlock, category.c_str());
}

void ReflectedPropertyControl::UpdateVarBlock(CVarBlock *pVarBlock)
{
    UpdateVarBlock(m_root, pVarBlock, m_pVarBlock);
    m_editor->QueueInvalidation(AzToolsFramework::Refresh_AttributesAndValues);
}

void ReflectedPropertyControl::UpdateVarBlock(ReflectedPropertyItem* pPropertyItem, IVariableContainer *pSourceContainer, IVariableContainer *pTargetContainer)
{
    for (int i = 0; i < pPropertyItem->GetChildCount(); ++i)
    {
        ReflectedPropertyItem *pChild = pPropertyItem->GetChild(i);

        if (pChild->GetType() != ePropertyInvalid)
        {
            const QString pPropertyVariableName = pChild->GetVariable()->GetName();

            IVariable* pTargetVariable = pTargetContainer->FindVariable(pPropertyVariableName.toLatin1().data());
            IVariable* pSourceVariable = pSourceContainer->FindVariable(pPropertyVariableName.toLatin1().data());

            if (pSourceVariable && pTargetVariable)
            {
                pTargetVariable->SetFlags(pSourceVariable->GetFlags());
                pTargetVariable->SetDisplayValue(pSourceVariable->GetDisplayValue());
                pTargetVariable->SetUserData(pSourceVariable->GetUserData());

                UpdateVarBlock(pChild, pSourceVariable, pTargetVariable);
            }
        }
    }
}

ReflectedPropertyItem* ReflectedPropertyControl::FindItemByVar(IVariable* pVar)
{
    return m_root->findItem(pVar);
}

ReflectedPropertyItem* ReflectedPropertyControl::GetRootItem()
{
    return m_root;
}

int ReflectedPropertyControl::GetContentHeight() const
{
    return m_editor->GetContentHeight();
}

void ReflectedPropertyControl::SetRootName(const QString& rootName)
{
    if (m_root)
    {
        //KDAB_PROPERTYCTRL_PORT_TODO
        //m_root->SetName(rootName);
    }
}


void ReflectedPropertyControl::AddCustomPopupMenuPopup(const QString& text, const Functor1<int>& handler, const QStringList& items)
{
    m_customPopupMenuPopups.push_back(SCustomPopupMenu(text, handler, items));
}


void ReflectedPropertyControl::AddCustomPopupMenuItem(const QString& text, const SCustomPopupItem::Callback handler)
{
    m_customPopupMenuItems.push_back(SCustomPopupItem(text, handler));
}

//////////////////////////////////////////////////////////////////////////
template<typename T>
void ReflectedPropertyControl::RemoveCustomPopup(const QString& text, T& customPopup)
{
    for (auto itr = customPopup.begin(); itr != customPopup.end(); ++itr)
    {
        if (text == itr->m_text)
        {
            customPopup.erase(itr);
            break;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void ReflectedPropertyControl::RemoveCustomPopupMenuItem(const QString& text)
{
    RemoveCustomPopup(text, m_customPopupMenuItems);
}

//////////////////////////////////////////////////////////////////////////
void ReflectedPropertyControl::RemoveCustomPopupMenuPopup(const QString& text)
{
    RemoveCustomPopup(text, m_customPopupMenuPopups);
}

void ReflectedPropertyControl::RestrictToItemsContaining(const QString &searchName)
{
   m_filterString = searchName.toLower();
   RecreateAllItems();
}

void ReflectedPropertyControl::SetUpdateCallback(const ReflectedPropertyControl::UpdateVarCallback& callback)
{
    m_updateVarFunc = callback;
}

void ReflectedPropertyControl::SetSavedStateKey(AZ::u32 key)
{
    m_editor->SetSavedStateKey(key);
}

void ReflectedPropertyControl::RemoveAllItems()
{
    m_editor->ClearInstances();
    m_rootContainer.reset();
    m_root.reset();
}

void ReflectedPropertyControl::ClearVarBlock()
{
    RemoveAllItems();
    m_pVarBlock = 0;
}

void ReflectedPropertyControl::RecreateAllItems()
{
    RemoveAllItems();
    AddVarBlock(m_pVarBlock);
}


void ReflectedPropertyControl::SetGroupProperties(bool group)
{
    m_groupProperties = group;
    RecreateAllItems();
}

void ReflectedPropertyControl::SetSortProperties(bool sort)
{
    m_sortProperties = sort;
    RecreateAllItems();
}


void ReflectedPropertyControl::AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode)
{
    if (!pNode)
        return;

    CReflectedVar *pReflectedVar = GetReflectedVarFromCallbackInstance(pNode);
    ReflectedPropertyItem *item = m_root->findItem(pReflectedVar);
    AZ_Assert(item, "No item found in property modification callback");
    item->OnReflectedVarChanged();

    OnItemChange(item);
}


void ReflectedPropertyControl::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode* pNode, const QPoint& pos)
{
    if (!pNode)
        return;

    CReflectedVar *pReflectedVar = GetReflectedVarFromCallbackInstance(pNode);
    ReflectedPropertyItem *pItem = m_root->findItem(pReflectedVar);
    AZ_Assert(pItem, "No item found in Context Menu callback");

    // Popup Menu with Event selection.
    QMenu menu;
    UINT i = 0;

    const int ePPA_CustomItemBase = 10; // reserved from 10 to 99
    const int ePPA_CustomPopupBase = 100; // reserved from 100 to x*100+100 where x is size of m_customPopupMenuPopups

    for (auto itr = m_customPopupMenuItems.cbegin(); itr != m_customPopupMenuItems.cend(); ++itr, ++i)
    {
        QAction *action = menu.addAction(itr->m_text);
        action->setData(i);
    }

    for (UINT j = 0; j < m_customPopupMenuPopups.size(); ++j)
    {
        SCustomPopupMenu* pMenuInfo = &m_customPopupMenuPopups[j];
        QMenu* pSubMenu = menu.addMenu(pMenuInfo->m_text);

        for (UINT k = 0; k < pMenuInfo->m_subMenuText.size(); ++k)
        {
            const UINT uID = ePPA_CustomPopupBase + ePPA_CustomPopupBase * j + k;
            QAction *action = pSubMenu->addAction(pMenuInfo->m_subMenuText[k]);
            action->setData(uID);
        }
    }

    QAction *result = menu.exec(pos);
    if (!result)
    {
        return;
    }
    const int res = result->data().toInt();
    if (res >= ePPA_CustomItemBase && res < m_customPopupMenuItems.size() + ePPA_CustomItemBase)
    {
        m_customPopupMenuItems[res - ePPA_CustomItemBase].m_callback();
    }
    else if (res >= ePPA_CustomPopupBase && res < ePPA_CustomPopupBase + ePPA_CustomPopupBase * m_customPopupMenuPopups.size())
    {
        const int menuid = res / ePPA_CustomPopupBase - 1;
        const int option = res % ePPA_CustomPopupBase;
        m_customPopupMenuPopups[menuid].m_callback(option);
    }
}

void ReflectedPropertyControl::PropertySelectionChanged(AzToolsFramework::InstanceDataNode *pNode, bool selected)
{
    if (!pNode)
        return;

    CReflectedVar *pReflectedVar = GetReflectedVarFromCallbackInstance(pNode);
    ReflectedPropertyItem *pItem = m_root->findItem(pReflectedVar);
    AZ_Assert(pItem, "No item found in selectoin change callback");

    if (m_selChangeFunc != NULL)
    {
        //keep same logic as MFC where pass null if it's deselection
        m_selChangeFunc(selected ? pItem->GetVariable() : nullptr);
    }
}

CReflectedVar * ReflectedPropertyControl::GetReflectedVarFromCallbackInstance(AzToolsFramework::InstanceDataNode *pNode)
{
    if (!pNode)
        return nullptr;
    const AZ::SerializeContext::ClassData *classData = pNode->GetClassMetadata();
    if (classData->m_azRtti && classData->m_azRtti->IsTypeOf(CReflectedVar::TYPEINFO_Uuid()))
        return reinterpret_cast<CReflectedVar *>(pNode->GetInstance(0));
    else
        return GetReflectedVarFromCallbackInstance(pNode->GetParent());
    return nullptr;
}


AzToolsFramework::PropertyRowWidget* ReflectedPropertyControl::FindPropertyRowWidget(ReflectedPropertyItem* item)
{
    if (!item)
    {
        return nullptr;
    }
    const AzToolsFramework::ReflectedPropertyEditor::WidgetList& widgets = m_editor->GetWidgets();
    for (auto instance : widgets)
    {
        if (instance.second->label() == item->GetPropertyName())
        {
            return instance.second;
        }
    }
    return nullptr;
}


void ReflectedPropertyControl::OnItemChange(ReflectedPropertyItem *item)
{
    if (!item->IsModified() || !m_bSendCallbackOnNonModified)
        return;

#ifdef KDAB_PORT
    // Called when item, gets modified.
    if (m_updateFunc != 0 && m_bEnableCallback)
    {
        m_bEnableCallback = false;
        m_updateFunc(item->GetXmlNode());
        m_bEnableCallback = true;
    }
#endif
    // variable updates/changes can trigger widgets being shown/hidden; better to delay triggering the update
    // callback until after the current event queue is processed, so that we aren't changing other widgets
    // as a ton of them are still being created.
    if (m_updateVarFunc != 0 && m_bEnableCallback)
    {
        QMetaObject::invokeMethod(this, "DoUpdateCallback", Qt::QueuedConnection, Q_ARG(IVariable*, item->GetVariable()));
    }
    if (m_updateObjectFunc != 0 && m_bEnableCallback)
    {
        // KDAB: This callback has same signature as DoUpdateCallback. I think the only reason there are 2 is because some 
        // EntityObject registers callback and some derived objects want to register their own callback. the normal UpdateCallback
        // can only be registered for item at a time so the original authors added a 2nd callback function, so we ported it this way.
        // This can probably get cleaned up to only on callback function with multiple receivers.
        QMetaObject::invokeMethod(this, "DoUpdateObjectCallback", Qt::QueuedConnection, Q_ARG(IVariable*, item->GetVariable()));
    }
}


void ReflectedPropertyControl::DoUpdateCallback(IVariable *var)
{
    if (m_updateVarFunc == 0)
        return;

    m_bEnableCallback = false;
    m_updateVarFunc(var);
    m_bEnableCallback = true;
}

void ReflectedPropertyControl::DoUpdateObjectCallback(IVariable *var)
{
    if (m_updateObjectFunc == 0)
        return;

    m_bEnableCallback = false;
    m_updateObjectFunc(var);
    m_bEnableCallback = true;
}

void ReflectedPropertyControl::InvalidateCtrl(bool queued)
{
    if (queued)
    {
        m_editor->QueueInvalidation(AzToolsFramework::Refresh_AttributesAndValues);
    }
    else
    {
        m_editor->InvalidateAttributesAndValues();
    }
}

void ReflectedPropertyControl::RebuildCtrl(bool queued)
{
    if (queued)
    {
        m_editor->QueueInvalidation(AzToolsFramework::Refresh_EntireTree);
    }
    else
    {
        m_editor->InvalidateAll();
    }
}

bool ReflectedPropertyControl::CallUndoFunc(ReflectedPropertyItem *item)
{
    if (!m_undoFunc)
        return false;

    m_undoFunc(item->GetVariable());
    return true;

}

void ReflectedPropertyControl::ClearSelection()
{
    m_editor->SelectInstance(nullptr);
}

void ReflectedPropertyControl::SelectItem(ReflectedPropertyItem* item)
{
    AzToolsFramework::PropertyRowWidget *widget = FindPropertyRowWidget(item);
    if (widget)
    {
        m_editor->SelectInstance(m_editor->GetNodeFromWidget(widget));
    }
}

ReflectedPropertyItem* ReflectedPropertyControl::GetSelectedItem()
{
    AzToolsFramework::PropertyRowWidget *widget = m_editor->GetWidgetFromNode(m_editor->GetSelectedInstance());
    if (widget)
    {
       return m_root->findItem(widget->label().toLatin1().data());
    }
    return nullptr;
}


QVector<ReflectedPropertyItem*> ReflectedPropertyControl::GetSelectedItems()
{
    return QVector<ReflectedPropertyItem*>{GetSelectedItem()};
}

void ReflectedPropertyControl::SetDisplayOnlyModified(bool displayOnlyModified)
{
    //Nothing to do here
    //SetDisplayOnlyModified was a way to handle editing properties for multiple things at once.
    //In this case you didn't want to show a value since the values could be different, that is until the user
    //modified the value, then all selected objects got the same value.
    //The ReflectedPropertyCtrl handles this differently by showing the value of the last item selected, so we'll
    //just use that. 
}

void ReflectedPropertyControl::CopyItem(XmlNodeRef rootNode, ReflectedPropertyItem* pItem, bool bRecursively)
{
    //KDAB_PROPERTYCTRL_PORT_TODO
}


void ReflectedPropertyControl::ReloadValues()
{
    if (m_root)
        m_root->ReloadValues();

    InvalidateCtrl();
}

void ReflectedPropertyControl::SetShowFilterWidget(bool showFilter)
{
    m_filterWidget->setVisible(showFilter);
}

void ReflectedPropertyControl::SetUndoCallback(UndoCallback &callback)
{
    m_undoFunc = callback;
}

void ReflectedPropertyControl::ClearUndoCallback()
{
    m_undoFunc = 0;
}

bool ReflectedPropertyControl::FindVariable(IVariable *categoryItem) const
{
    assert(m_root);
    ReflectedPropertyItem *pItem = m_root->findItem(categoryItem);
    return pItem != nullptr;
}

void ReflectedPropertyControl::EnableUpdateCallback(bool bEnable)
{
    m_bEnableCallback = bEnable;
}

void ReflectedPropertyControl::SetGrayed(bool grayed)
{
    //KDAB_PROPERTYCTRL_PORT_TODO
    //control should be grayed out but not disabled? 
}

void ReflectedPropertyControl::SetReadOnly(bool readonly)
{
    setEnabled(!readonly);
}


void ReflectedPropertyControl::SetMultiSelect(bool multiselect)
{
    //KDAB_PROPERTYCTRL_PORT_TODO
}

void ReflectedPropertyControl::SetSplitter(int width)
{
    //KDAB_PROPERTYCTRL_PORT_TODO
}

void ReflectedPropertyControl::EnableNotifyWithoutValueChange(bool bFlag)
{
    m_bForceModified = bFlag;

    //KDAB_PROPERTYCTRL_PORT_TODO
    //The old CPropertyCtrl used this flag to force updates when a property was edited but the value didn't change
    //It affected number controls (slider and spinbox) for when value was edited (maybe changing 1 to 1.0?)
    //It affected all controls by forcing an update when losing focus.
    //Not sure whether we still need to handle this or how to port it if so. Perhaps an event filter on all editing widgets to get focus out event?
}

void ReflectedPropertyControl::SetTitle(const QString &title)
{
    m_titleLabel->setText(title);
    m_titleLabel->setHidden(title.isEmpty());
}

void ReflectedPropertyControl::ExpandAll()
{
    m_editor->ExpandAll();
}

void ReflectedPropertyControl::CollapseAll()
{
    m_editor->CollapseAll();
}

void ReflectedPropertyControl::Expand(ReflectedPropertyItem* item, bool expand)
{
    item->Expand(expand);
}

void ReflectedPropertyControl::ExpandAllChildren(ReflectedPropertyItem* item, bool recursive)
{
    item->ExpandAllChildren(recursive);
}

TwoColumnPropertyControl::TwoColumnPropertyControl(QWidget *parent /*= nullptr*/)
    : QWidget(parent)
{
    QHBoxLayout *mainlayout = new QHBoxLayout(this);

    QWidget *leftContainer = new QWidget;
    m_leftLayout = new QVBoxLayout(leftContainer);
    m_leftLayout->setContentsMargins(0, 0, 0, 0);
    m_leftScrollArea = new QScrollArea;
    m_leftScrollArea->setWidget(leftContainer);
    m_leftScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainlayout->addWidget(m_leftScrollArea);

    QWidget *rightContainer = new QWidget;
    m_rightLayout = new QVBoxLayout(rightContainer);
    m_rightLayout->setContentsMargins(0, 0, 0, 0);
    m_rightScrollArea = new QScrollArea;
    m_rightScrollArea->setWidget(rightContainer);
    m_rightScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainlayout->addWidget(m_rightScrollArea);
}

void TwoColumnPropertyControl::Setup(bool showScrollbars /*= true*/, int labelWidth /*= 150*/)
{
}

void TwoColumnPropertyControl::AddVarBlock(CVarBlock *varBlock, const char *szCategory /*= nullptr*/)
{
    m_pVarBlock = varBlock;
    for (int i = 0; i < m_pVarBlock->GetNumVariables(); ++i)
    {
        CVarBlock *vb = new CVarBlock;
        PropertyCard *ctrl = new PropertyCard;
        m_varBlockList.append(vb);
        m_controlList.append(ctrl);
        IVariable *var = m_pVarBlock->GetVariable(i);
        vb->AddVariable(var);
        ctrl->AddVarBlock(vb);

        if (var->GetFlags() & IVariable::UI_ROLLUP2)
        {
            m_rightLayout->addWidget(ctrl);
        }
        else
        {
            m_leftLayout->addWidget(ctrl);
        }
        connect(ctrl, &PropertyCard::OnExpansionContractionDone, this, [this, ctrl]()
        {
            QMetaObject::invokeMethod(this, "UpdateLayouts", Qt::QueuedConnection);
        });
    }
    m_leftLayout->addStretch(1);
    m_rightLayout->addStretch(1);
}

void TwoColumnPropertyControl::UpdateLayouts()
{
    //resize all of the controls to the size of their content, whether expanded or contracted
    for (auto control : m_controlList)
    {
        control->setFixedHeight(control->sizeHint().height());
    }
    m_leftLayout->update();
    m_leftLayout->activate();
    m_rightLayout->update();
    m_rightLayout->activate();

    const QSize availableSize = m_leftScrollArea->size();

    const int leftContentHeight = m_leftScrollArea->widget()->sizeHint().height();
    const int extraLeftScrollBarSpace = leftContentHeight <= availableSize.height() ? 0 : m_leftScrollArea->verticalScrollBar()->width();
    m_leftScrollArea->widget()->setFixedSize(availableSize.width() - extraLeftScrollBarSpace, leftContentHeight);

    const int rightContentHeight = m_rightScrollArea->widget()->sizeHint().height();
    const int extraRightScrollBarSpace = rightContentHeight <= availableSize.height() ? 0 : m_rightScrollArea->verticalScrollBar()->width();
    m_rightScrollArea->widget()->setFixedSize(availableSize.width() - extraRightScrollBarSpace, rightContentHeight);
}

void TwoColumnPropertyControl::resizeEvent(QResizeEvent *event)
{
    UpdateLayouts();
}

void TwoColumnPropertyControl::ReplaceVarBlock(IVariable *categoryItem, CVarBlock *varBlock)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ReplaceVarBlock(categoryItem, varBlock);
    }
}

void TwoColumnPropertyControl::RemoveAllItems()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->RemoveAllItems();
    }
}

bool TwoColumnPropertyControl::FindVariable(IVariable *categoryItem) const
{
    for (auto ctrl : m_controlList)
    {
        if (ctrl->GetControl()->FindVariable(categoryItem))
        {
            return true;
        }
    }

    return false;
}

void TwoColumnPropertyControl::InvalidateCtrl()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->InvalidateCtrl();
    }
}

void TwoColumnPropertyControl::RebuildCtrl()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->RebuildCtrl();
    }
}

void TwoColumnPropertyControl::SetStoreUndoByItems(bool bStoreUndoByItems)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->SetStoreUndoByItems(bStoreUndoByItems);
    }
}

void TwoColumnPropertyControl::SetUndoCallback(ReflectedPropertyControl::UndoCallback callback)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->SetUndoCallback(callback);
    }
}

void TwoColumnPropertyControl::ClearUndoCallback()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ClearUndoCallback();
    }
}

void TwoColumnPropertyControl::EnableUpdateCallback(bool bEnable)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->EnableUpdateCallback(bEnable);
    }
}

void TwoColumnPropertyControl::SetUpdateCallback(ReflectedPropertyControl::UpdateVarCallback callback)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->SetUpdateCallback(callback);
    }
}

void TwoColumnPropertyControl::SetGrayed(bool grayed)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->SetGrayed(grayed);
    }
}


void TwoColumnPropertyControl::SetSavedStateKey(const QString &key)
{
    for (int i = 0; i < m_controlList.count(); ++i)
    {
        m_controlList[i]->GetControl()->SetSavedStateKey(AZ::Crc32((key + QString::number(i)).toLatin1().data()));
    }
}

void TwoColumnPropertyControl::ExpandAllChildren(ReflectedPropertyItem* item, bool recursive)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ExpandAllChildren(item, recursive);
    }
}

void TwoColumnPropertyControl::ExpandAllChildren(bool recursive)
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ExpandAllChildren(ctrl->GetControl()->GetRootItem(), recursive);
    }
}

void TwoColumnPropertyControl::ReloadItems()
{
    for (auto ctrl : m_controlList)
    {
        ctrl->GetControl()->ReloadValues();
    }
}



PropertyCard::PropertyCard(QWidget* parent /*= nullptr*/)
{
    // create header bar
    m_header = new AzToolsFramework::ComponentEditorHeader(this);
    m_header->SetExpandable(true);

    // create property editor
    m_propertyEditor = new ReflectedPropertyControl(this);
    m_propertyEditor->Setup(false);
    m_propertyEditor->GetEditor()->SetHideRootProperties(true);
    m_propertyEditor->setProperty("ComponentDisabl", true); // used by stylesheet

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_header);
    mainLayout->addWidget(m_propertyEditor);
    setLayout(mainLayout);

    connect(m_header, &AzToolsFramework::ComponentEditorHeader::OnExpanderChanged, this, &PropertyCard::OnExpanderChanged);
    connect(m_propertyEditor->GetEditor(), &AzToolsFramework::ReflectedPropertyEditor::OnExpansionContractionDone, this, &PropertyCard::OnExpansionContractionDone);

    SetExpanded(true);
}

void PropertyCard::AddVarBlock(CVarBlock *varBlock)
{
    if (varBlock->GetNumVariables() > 0)
    {
        m_header->SetTitle(varBlock->GetVariable(0)->GetName());
        m_propertyEditor->AddVarBlock(varBlock);
    }
}

ReflectedPropertyControl* PropertyCard::GetControl()
{
    return m_propertyEditor;
}

void PropertyCard::SetExpanded(bool expanded)
{
    m_header->SetExpanded(expanded);
    m_propertyEditor->setVisible(expanded);
}

bool PropertyCard::IsExpanded() const
{
    return m_header->IsExpanded();
}

void PropertyCard::OnExpanderChanged(bool expanded)
{
    SetExpanded(expanded);
    emit OnExpansionContractionDone();
}

#include <Controls/ReflectedPropertyControl/ReflectedPropertyCtrl.moc>
