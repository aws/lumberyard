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

//EditorUI_QT
#include <../EditorUI_QT/DockableLibraryPanel.h>

class DockableLibraryPanel;
class QAction;

class DockableParticleLibraryPanel
    : public DockableLibraryPanel
{
    Q_OBJECT
public:
    enum class ParticleActions
    {
        GROUP = 0,
        COPY,
        PASTE,
        UNGROUP,
        ENABLE,
        DISABLE
    };

    DockableParticleLibraryPanel(QWidget* parent)
        : DockableLibraryPanel(parent)
    {
        //we remap to ensure parent actions will not get double mapped.
        RemapHotKeys();
    }
    ~DockableParticleLibraryPanel() {}

    QAction* GetParticleAction(ParticleActions action, QString fullNameOfItem, QString displayAlias, bool overrideSafety = false, QWidget* owner = nullptr, Qt::ConnectionType connection = Qt::AutoConnection);

protected:
    virtual void RegisterActions();

    QString PromptImportLibrary() override;

private:
    enum ParticleValidationFlag
    {
        NO_NULL = 1 << 0,
        NO_VIRTUAL_ITEM = 1 << 1,
        IS_GROUP = 1 << 2,
        SHARE_PARENTS = 1 << 3 //always true with Validate, can fail with ValidateAll
    };
    bool Validate(CLibraryTreeViewItem* item, ParticleValidationFlag flags);
    bool ValidateAll(QVector<CLibraryTreeViewItem*>, ParticleValidationFlag flags);
    //////////////////////////////////////////////////////////////////////////
    //ACTIONS - void ActionFunctionName(const QString& overrideSelection = "", const bool overrideSafety = false)
    void ActionCopy(const QString& overrideSelection = "", const bool overrideSafety = false);
    void ActionPaste(const QString& overrideSelection = "", const bool overrideSafety = false);
    void ActionGroup(const QString& overrideSelection = "", const bool overrideSafety = false);
    void ActionUngroup(const QString& overrideSelection = "", const bool overrideSafety = false);
    void ActionEnable(const QString& overrideSelection = "", const bool overrideSafety = false);
    void ActionDisable(const QString& overrideSelection = "", const bool overrideSafety = false);
    void ActionToggleEnabled(const QString& overrideSelection = "", const bool overrideSafety = false);
    //END ACTIONS
    //////////////////////////////////////////////////////////////////////////
};
