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

#ifndef CRYINCLUDE_EDITOR_MANNEQUIN_MANNTAGEDITORDIALOG_H
#define CRYINCLUDE_EDITOR_MANNEQUIN_MANNTAGEDITORDIALOG_H
#pragma once


#include "Controls/TagSelectionControl.h"

#include <QDialog>
#include <QScopedPointer>

class QGroupBox;
class QLineEdit;
class QComboBox;
class QCheckBox;

class CTagDefinition;
struct SMannequinContexts;

namespace Ui {
    class CMannTagEditorDialog;
}


class CMannTagEditorDialog
    : public QDialog
{
    Q_OBJECT
public:
    CMannTagEditorDialog(IAnimationDatabase* animDB, FragmentID fragmentID, QWidget* pParent = nullptr);
    virtual ~CMannTagEditorDialog();

    const QString& FragmentName() const { return m_fragmentName; }

protected:
    BOOL OnInitDialog();
    void OnOK();

    const QString GetCurrentADBFileSel() const;
    void ProcessFragmentADBChanges();
    void ProcessFragmentTagChanges();
    void ProcessFlagChanges();
    void ProcessScopeTagChanges();

    void InitialiseFragmentTags(const CTagDefinition* pTagDef);
    void AddTagDef(const QString& filename);
    void AddTagDefInternal(const QString& tagDefDisplayName, const QString& normalizedFilename);
    void AddFragADB(const QString& filename);
    void AddFragADBInternal(const QString& fragFileDisplayName, const QString& normalizedFilename);
    void ResetFragmentADBs();
    bool ContainsTagDefDisplayName(const QString& tagDefDisplayNameToSearch) const;
    const QString GetSelectedTagDefFilename() const;
    void SelectTagDefByTagDef(const CTagDefinition* pTagDef);
    void SelectTagDefByFilename(const QString& filename);

protected slots:
    void OnEditTagDefs();
    void OnCbnSelchangeFragfileCombo();
    void OnBnClickedCreateAdbFile();

private:
    void InitialiseFragmentADBsRec(const QString& baseDir);
    void InitialiseFragmentTagsRec(const QString& baseDir);
    void InitialiseFragmentADBs();

    QLineEdit*                          m_nameEdit;
    QCheckBox*                              m_btnIsPersistent;
    QCheckBox*                              m_btnAutoReinstall;
    QGroupBox*                              m_scopeTagsGroupBox;
    CTagSelectionControl    m_scopeTagsControl;
    QComboBox*                          m_tagDefComboBox;
    QComboBox*                          m_FragFileComboBox;
    int                                     m_nOldADBFileIndex;

    SMannequinContexts* m_contexts;
    IAnimationDatabase* m_animDB;
    FragmentID                      m_fragmentID;
    QString                             m_fragmentName;

    struct SFragmentTagEntry
    {
        QString displayName;
        QString filename;

        SFragmentTagEntry(const QString& displayName_, const QString& filename_)
            : displayName(displayName_)
            , filename(filename_)    {}
    };
    typedef std::vector<SFragmentTagEntry> TEntryContainer;
    TEntryContainer m_entries;
    TEntryContainer m_vFragADBFiles;

    QScopedPointer<Ui::CMannTagEditorDialog> ui;
};

#endif // CRYINCLUDE_EDITOR_MANNEQUIN_MANNTAGEDITORDIALOG_H
