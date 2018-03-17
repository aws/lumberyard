#include "stdafx.h"
#include "DockableParticleLibraryPanel.h"

//Editor
#include <IEditor.h>
#include <EditorDefs.h>
#include <BaseLibraryManager.h>
#include <Clipboard.h>
#include <ILogFile.h>
#include <IEditorParticleUtils.h> //for shortcut support on menu actions
#include <Include/IEditorParticleManager.h>
#include <ParticleParams.h>
#include <../Editor/Particles/ParticleItem.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

//EditorUI_QT
#include <../EditorUI_QT/LibraryTreeViewItem.h>
#include <../EditorUI_QT/LibraryTreeView.h>
#include <../EditorUI_QT/DockableLibraryTreeView.h>

#include <QAction>

#include <QT/DockableParticleLibraryPanel.moc>
#define QTUI_EDITOR_ACTIONGROUP_FUNCTIONNAME "ActionGroup"

QAction* DockableParticleLibraryPanel::GetParticleAction(ParticleActions action, QString fullNameOfItem, QString displayAlias, bool overrideSafety /*= false*/, QWidget* owner /*= nullptr*/, Qt::ConnectionType connection /*= Qt::AutoConnection*/)
{
    QStringList splitName = fullNameOfItem.split('.');
    QString libName = splitName.takeFirst();
    QString itemPath = splitName.join('.');
    CLibraryTreeViewItem* item = nullptr;
    if (!fullNameOfItem.isEmpty())
    {
        item = GetTreeItemFromPath(fullNameOfItem);
    }
    QAction* act = nullptr;
    if (owner)
    {
        act = new QAction(displayAlias, owner);
    }
    else
    {
        act = new QAction(displayAlias, this);
    }

#define DISABLEIF(x) if (x) {act->setDisabled(true); return act; }

    QVector<CLibraryTreeViewItem*> selection = GetActionItems(fullNameOfItem);
    switch (action)
    {
    case DockableParticleLibraryPanel::ParticleActions::GROUP:
    {
        act->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Group"));
        DISABLEIF(!ValidateAll(selection, ParticleValidationFlag(NO_NULL | NO_VIRTUAL_ITEM | SHARE_PARENTS)));
        connect(act, &QAction::triggered, this, [fullNameOfItem, overrideSafety, this](){ ActionGroup(fullNameOfItem, overrideSafety); }, connection);
        break;
    }
    case DockableParticleLibraryPanel::ParticleActions::UNGROUP:
    {
        act->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Ungroup"));
        DISABLEIF(!ValidateAll(selection, ParticleValidationFlag(NO_NULL | NO_VIRTUAL_ITEM | IS_GROUP)));
        connect(act, &QAction::triggered, this, [fullNameOfItem, overrideSafety, this](){ ActionUngroup(fullNameOfItem, overrideSafety); }, connection);
        break;
    }
    case DockableParticleLibraryPanel::ParticleActions::COPY:
    {
        act->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Copy"));
        DISABLEIF(!ValidateAll(selection, ParticleValidationFlag(NO_NULL | NO_VIRTUAL_ITEM)));
        connect(act, &QAction::triggered, this, [fullNameOfItem, overrideSafety, this](){ ActionCopy(fullNameOfItem, overrideSafety); }, connection);
        break;
    }
    case DockableParticleLibraryPanel::ParticleActions::PASTE:
    {
        act->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Paste"));
        CClipboard clipboard(this);
        DISABLEIF(clipboard.IsEmpty());
        XmlNodeRef node = clipboard.Get();
        DISABLEIF(!node);
        DISABLEIF(selection.count() != 1);
        DISABLEIF(!Validate(selection.first(), ParticleValidationFlag(NO_NULL)));
        connect(act, &QAction::triggered, this, [fullNameOfItem, overrideSafety, this](){ ActionPaste(fullNameOfItem, overrideSafety); }, connection);
        break;
    }
    case DockableParticleLibraryPanel::ParticleActions::ENABLE:
    {
        DISABLEIF(!ValidateAll(selection, ParticleValidationFlag(NO_NULL)));
        connect(act, &QAction::triggered, this, [fullNameOfItem, overrideSafety, this](){ ActionEnable(fullNameOfItem, overrideSafety); }, connection);
        break;
    }
    case DockableParticleLibraryPanel::ParticleActions::DISABLE:
    {
        DISABLEIF(!ValidateAll(selection, ParticleValidationFlag(NO_NULL)));
        connect(act, &QAction::triggered, this, [fullNameOfItem, overrideSafety, this](){ ActionDisable(fullNameOfItem, overrideSafety); }, connection);
        break;
    }
    default:
    {
        act->setParent(nullptr);
        SAFE_DELETE(act);
        act = nullptr;
        break;
    }
    }
    return act;
}

void DockableParticleLibraryPanel::RegisterActions()
{
    DockableLibraryPanel::RegisterActions();
    CRY_ASSERT(GetIEditor());
    CRY_ASSERT(GetIEditor()->GetParticleUtils());
    IEditorParticleUtils* utils = GetIEditor()->GetParticleUtils();

    QAction* action;

    //COPY////////////////////////////////////////////////////////////////////
    action = new QAction(this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Copy"));
    connect(action, &QAction::triggered, this, [this]()
        {
            ActionCopy();
        });
    addAction(action);
    //PASTE///////////////////////////////////////////////////////////////////
    action = new QAction(this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Paste"));
    connect(action, &QAction::triggered, this, [this]()
        {
            ActionPaste();
        });
    addAction(action);
    //GROUPING////////////////////////////////////////////////////////////////
    action = new QAction(this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Group"));
    connect(action, &QAction::triggered, this, [this]()
        {
            this->ActionGroup();
        });
    addAction(action);
    //UNGROUPING//////////////////////////////////////////////////////////////
    action = new QAction(this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Ungroup"));
    connect(action, &QAction::triggered, this, [this]()
        {
            this->ActionUngroup();
        });
    addAction(action);
    //ENABLE-DISABLE//////////////////////////////////////////////////////////
    action = new QAction(this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("Edit Menu.Enable/Disable Emitter"));
    connect(action, &QAction::triggered, this, [this]()
        {
            this->ActionToggleEnabled();
        });
    addAction(action);
    //ADD PARTICLE////////////////////////////////////////////////////////////
    action = GetMenuAction(DockableLibraryPanel::ItemActions::ADD_ITEM, "", tr("Add Particle"), false, this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("File Menu.Create new emitter"));
    addAction(action);
    //ENABLE-DISABLE//////////////////////////////////////////////////////////
    action = GetMenuAction(DockableLibraryPanel::ItemActions::ADD_FOLDER, "", tr("Add Folder"), false, this);
    action->setShortcut(GetIEditor()->GetParticleUtils()->HotKey_GetShortcut("File Menu.Create new folder"));
    addAction(action);
}

QString DockableParticleLibraryPanel::PromptImportLibrary()
{
    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection("Particles", true);
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (selection.IsValid())
    {
        return selection.GetResult()->GetName().c_str();
    }
    return "";
}

bool DockableParticleLibraryPanel::Validate(CLibraryTreeViewItem* item, ParticleValidationFlag flags)
{
    if (flags & ParticleValidationFlag::NO_NULL)
    {
        if (!item)
        {
            return false;
        }
    }
    if (flags & ParticleValidationFlag::NO_VIRTUAL_ITEM)
    {
        CRY_ASSERT(item);
        if (item->IsVirtualItem())
        {
            return false;
        }
    }
    if (flags & ParticleValidationFlag::IS_GROUP)
    {
        CRY_ASSERT(item);
        CParticleItem* pParticle = static_cast<CParticleItem*>(item->GetItem());
        ParticleParams params = pParticle->GetEffect()->GetParticleParams();
        if (!params.bGroup)
        {
            return false;
        }
    }
    return true;
}

bool DockableParticleLibraryPanel::ValidateAll(QVector<CLibraryTreeViewItem*> selection, ParticleValidationFlag flags)
{
    if (selection.isEmpty())
    {
        return false;
    }
    CBaseLibraryItem* parent = selection.first()->GetItem();
    if (parent)
    {
        parent = static_cast<CParticleItem*>(parent)->GetParent();
    }
    for (unsigned int i = 0, _e = selection.count(); i < _e; i++)
    {
        if (!Validate(selection[i], flags))
        {
            return false;
        }
        if (flags & ParticleValidationFlag::SHARE_PARENTS)
        {
            CBaseLibraryItem* selectedParent = selection.first()->GetItem();
            if (selectedParent)
            {
                selectedParent = static_cast<CParticleItem*>(selectedParent)->GetParent();
            }
            if (parent != selectedParent)
            {
                return false;
            }
        }
    }
    return true;
}

void DockableParticleLibraryPanel::ActionCopy(const QString& overrideSelection /*= ""*/, const bool overrideSafety /*= false*/)
{
    QVector<CLibraryTreeViewItem*> items = GetActionItems(overrideSelection);

    emit SignalCopyItems(items);
}

void DockableParticleLibraryPanel::ActionPaste(const QString& overrideSelection /*= ""*/, const bool overrideSafety /*= false*/)
{
    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(GetIEditor()->GetParticleManager());
    QVector<CLibraryTreeViewItem*> items = GetActionItems(overrideSelection);
    if (items.count() >= 1)
    {
        if (items.first())
        {
            if (items.first()->IsVirtualItem())
            {
                //Get the destination path
                QString fullPath = items.first()->GetFullPath();
                QStringList pathList = fullPath.split('.');
                QString destlib = pathList.takeFirst();
                DockableLibraryTreeView* destLibDock = m_libraryTreeViews[destlib];
                QString destpathWithoutLib = pathList.join('.');
                //Get the clipboard for access to the copied item's name
                CClipboard clipboard(this);
                if (clipboard.IsEmpty())
                {
                    return;
                }
                XmlNodeRef node = clipboard.Get();
                if (!node)
                {
                    return;
                }

                if (strcmp(node->getTag(), "Childs") == 0)
                {
                    //declared here so they can be reused
                    QString newItemName;
                    QString newItemFullName;
                    QString newItemFullNameWithLib;
                    QStringList itemsToPasteTo;
                    int copiedItems = node->getChildCount();
                    //Create new items in the folder
                    for (int i = 0; i < copiedItems; ++i)
                    {
                        XmlNodeRef copy = node->getChild(i);
                        //This is the copied item's name
                        newItemName = copy->getAttr("Name");
                        //Create the full path (without library name)
                        newItemFullName = destpathWithoutLib + "." + newItemName;
                        //Create the full path(with library name)
                        newItemFullNameWithLib = destlib + "." + newItemFullName;
                        if (m_libraryManager->FindItemByName(newItemFullNameWithLib) != nullptr)
                        {
                            newItemFullName = m_libraryManager->MakeUniqueItemName(newItemFullName);
                            newItemFullNameWithLib = destlib + "." + newItemFullName;
                        }
                        //add a new item with that name
                        CBaseLibraryItem* item = destLibDock->AddItem(newItemFullName);
                        if (!item)
                        {
                            continue;
                        }
                        emit SignalItemAdded(item, newItemFullName);
                        item->SetName(newItemFullName.toUtf8().data());
                        itemsToPasteTo.push_back(newItemFullNameWithLib);
                    }

                    //Send the list of items to paste to
                    emit SignalPasteItemsToFolder(destLibDock->GetLibrary(), itemsToPasteTo);
                    destLibDock->SyncFromEngine();
                }
                else
                {
                    //This is the copied item's name
                    QString newItemName = node->getAttr("Name");
                    //Create the full path (without library name)
                    QString newItemFullName = pathList.join(".") + "." + newItemName;
                    //Create the full path(with library name)
                    QString newItemFullNameWithLib = destlib + "." + newItemFullName;
                    if (m_libraryManager->FindItemByName(newItemFullNameWithLib) != nullptr)
                    {
                        newItemFullName = m_libraryManager->MakeUniqueItemName(newItemFullName);
                    }
                    //add a new item with that name
                    CBaseLibraryItem* item = destLibDock->AddItem(newItemFullName);
                    if (!item)
                    {
                        return;
                    }
                    emit SignalItemAdded(item, newItemFullName);
                    item->SetName(newItemFullName.toUtf8().data());
                    //sync from the engine to ensure the library tree view is accurate
                    destLibDock->SyncFromEngine();
                    // Library get modified after item added and synchronized. We will get treeitem agian
                    // in case the data is corrupted
                    CLibraryTreeViewItem* treeItem = destLibDock->GetItemFromName(newItemFullName);
                    //Using true so that the override prompt doesn't come up.
                    emit SignalItemPasted(treeItem->GetItem(), true);
                }
            }
            else
            {
                emit SignalItemPasted(items.first()->GetItem(), overrideSafety);
            }
        }
    }
}

void DockableParticleLibraryPanel::ActionGroup(const QString& overrideSelection /*= ""*/, const bool overrideSafety /*= false*/)
{
    QVector<CLibraryTreeViewItem*> items = GetActionItems(overrideSelection);
    //copy the items received here to the group as children
    emit SignalCopyItems(items, true);

    if (items.isEmpty())
    {
        return;
    }
    //check that all items share the same parent
    //if they don't we shouldn't be in here
    CBaseLibraryItem* firstItem = items.first()->GetItem();
    if (!firstItem)
    {
        return;
    }
    QString firstParent = firstItem->GetGroupName();
    for (auto item : items)
    {
        if (!item)
        {
            return;
        }
        else if (item->IsVirtualItem())
        {
            return;
        }
        else if (firstParent.compare(item->GetItem()->GetGroupName(), Qt::CaseInsensitive) != 0) //different parents
        {
            return;
        }
    }
    //delete selected items
    //add item to library as a group with a generic name
    QString fullParentPath = firstItem->GetLibrary()->GetName();
    fullParentPath.append(".");
    fullParentPath.append(firstParent);

    QString libName = items.first()->GetLibraryName();
    if (!m_libraryTreeViews.contains(libName))
    {
        return;
    }

    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(GetIEditor()->GetParticleManager());
    DockableLibraryTreeView* library = m_libraryTreeViews[libName];
    CRY_ASSERT(library);
    QString defaultGroupName = "group";
    QString groupName = firstParent.isEmpty() ? defaultGroupName : firstParent + "." + defaultGroupName;
    groupName = mngr->MakeUniqueItemName(groupName, libName);
    if (groupName.isEmpty())
    {
        return; //user cancelled
    }

    //one lib modify undo and disable other undo
    EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;
    EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, AZStd::string(libName.toUtf8().data()));
    EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
    EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

    //Add UndoGroup and record Library Undo and  after groupname validation check
    QAction* action = DockableLibraryPanel::GetMenuAction(ItemActions::DESTROY, overrideSelection, "add_action", true);
    CRY_ASSERT(action && "library missing requested action");
    action->trigger();
    action->setParent(nullptr);
    SAFE_DELETE(action);

    m_lastAddedItem = library->AddLibItem(groupName, true);

    if (GetLastAddedItemName().isEmpty())
    {
        //revert the deletion
        CParticleItem* parentItem = static_cast<CParticleItem*>(GetIEditor()->GetParticleManager()->FindItemByName(fullParentPath));
        CRY_ASSERT(parentItem);
        emit SignalItemPasted(parentItem);
        return;
    }
    //select the parent for the add
    CParticleItem* parentItem = static_cast<CParticleItem*>(GetIEditor()->GetParticleManager()->FindItemByName(GetLastAddedItemName()));
    ParticleParams params(parentItem->GetEffect()->GetParticleParams());
    params.bGroup = true;
    params.fCount = 0;
    params.bContinuous = false;
    parentItem->GetEffect()->SetParticleParams(params);
    //pass the copied items to the group
    emit SignalItemPasted(parentItem);
    ActionRename(m_lastAddedItem);
}

void DockableParticleLibraryPanel::ActionUngroup(const QString& overrideSelection /*= ""*/, const bool overrideSafety /*= false*/)
{
    //if item is group, move the children to this items parent, and then delete group
    QVector<CLibraryTreeViewItem*> items = GetActionItems(overrideSelection);
    if (items.isEmpty() || items.count() > 1) //no multi-emitter ungrouping, ungroup is on group node only
    {
        return;
    }
    //check that all items are valid for this operation
    CBaseLibraryItem* firstItem = items.first()->GetItem();
    if (!firstItem)
    {
        return;
    }
    QString firstParent = firstItem->GetGroupName();
    CLibraryTreeViewItem* item = items.first();
    if (item->IsVirtualItem())
    {
        return;
    }
    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(GetIEditor()->GetParticleManager());
    CParticleItem* pParticle = static_cast<CParticleItem*>(item->GetItem());



    CRY_ASSERT(pParticle);
    CParticleItem* parentParticle = pParticle->GetParent();
    ParticleParams params = pParticle->GetEffect()->GetParticleParams();
    ParticleParams parentParams;
    if (parentParticle)
    {
        parentParams = parentParticle->GetEffect()->GetParticleParams();
    }
    if (params.bGroup)
    {
        QString lib = item->GetLibraryName();

        //one lib modify undo and disable other undo
        EditorUIPlugin::ScopedLibraryModifyUndoPtr modifyUndo;
        EBUS_EVENT_RESULT(modifyUndo, EditorLibraryUndoRequestsBus, AddScopedLibraryModifyUndo, AZStd::string(lib.toUtf8().data()));
        EditorUIPlugin::ScopedSuspendUndoPtr suspendUndo;
        EBUS_EVENT_RESULT(suspendUndo, EditorLibraryUndoRequestsBus, AddScopedSuspendUndo);

        QString itemName = item->GetItem()->GetFullName();
        item = GetTreeItemFromPath(itemName);
        QString itemNameWithoutLib = itemName.right(itemName.length() - itemName.indexOf('.') - 1);
        CRY_ASSERT(item);
        QVector<QPair<QString, QString> > desiredNamesToUniqueNames;
        //loop through children of item and move them up one level, making unique name for each item.
        //any item that makeuniquename != desired name save the new name/desired name pair for later use
        for (unsigned int i = 0, _e = item->childCount(); i < _e; i++)
        {
            CLibraryTreeViewItem* treeitem = static_cast<CLibraryTreeViewItem*>(item->child(i));
            QString newName = treeitem->GetFullPath();
            newName.replace(itemName, itemName.left(itemName.lastIndexOf('.')));
            QString newNameNoLib = newName.right(newName.length() - newName.indexOf('.') - 1);
            QString newUniqueName = m_libraryManager->MakeUniqueItemName(newNameNoLib, treeitem->GetLibraryName());
            RenameLibraryItem(treeitem->GetFullPath(), newUniqueName);
            if (newName.compare(itemName, Qt::CaseInsensitive) == 0)
            {
                desiredNamesToUniqueNames.append(QPair<QString, QString>(newNameNoLib, newUniqueName));
            }
        }
        //delete group
        RebuildFromEngineData();
        ResetSelection();
        ActionDelete(itemName, true);
        RebuildFromEngineData();

        //if any desired names == itemName then replace the unique names with the desired name
        for (unsigned int i = 0, _e = desiredNamesToUniqueNames.count(); i < _e; i++)
        {
            RenameLibraryItem(lib + '.' + desiredNamesToUniqueNames[i].second, desiredNamesToUniqueNames[i].first);
        }
    }
}

void DockableParticleLibraryPanel::ActionEnable(const QString& overrideSelection /*= ""*/, const bool overrideSafety /*= false*/)
{
    QVector<CLibraryTreeViewItem*> selection = GetActionItems(overrideSelection);

    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(GetIEditor()->GetParticleManager());
    
    for (unsigned int i = 0, _e = selection.count(); i < _e; i++)
    {
        selection[i]->SetEnabled(true);
        selection[i]->setCheckState(LIBRARY_TREEVIEW_INDICATOR_COLUMN, Qt::CheckState::Checked);
    }
}

void DockableParticleLibraryPanel::ActionDisable(const QString& overrideSelection /*= ""*/, const bool overrideSafety /*= false*/)
{
    CBaseLibraryManager* mngr = static_cast<CBaseLibraryManager*>(GetIEditor()->GetParticleManager());

    QVector<CLibraryTreeViewItem*> selection = GetActionItems(overrideSelection);
    
    for (unsigned int i = 0, _e = selection.count(); i < _e; i++)
    {
        selection[i]->SetEnabled(false);
        selection[i]->setCheckState(LIBRARY_TREEVIEW_INDICATOR_COLUMN, Qt::CheckState::Unchecked);
    }
}

void DockableParticleLibraryPanel::ActionToggleEnabled(const QString& overrideSelection /*= ""*/, const bool overrideSafety /*= false*/)
{
    QVector<CLibraryTreeViewItem*> selection = GetActionItems(overrideSelection);

    if (selection.count() > 0)
    {
        if (selection.first()->IsEnabled())
        {
            ActionDisable(overrideSelection, overrideSafety);
        }
        else
        {
            ActionEnable(overrideSelection, overrideSafety);
        }
    }
}
