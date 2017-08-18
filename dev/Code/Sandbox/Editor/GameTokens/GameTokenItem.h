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

#ifndef CRYINCLUDE_EDITOR_GAMETOKENS_GAMETOKENITEM_H
#define CRYINCLUDE_EDITOR_GAMETOKENS_GAMETOKENITEM_H
#pragma once

#include <QTreeView>
#include <QPainter>
#include "BaseLibraryItem.h"
#include <IFlowSystem.h>

struct IGameToken;
struct IGameTokenSystem;

class GameTokenTreeView
    : public QTreeView
{
public:
    explicit GameTokenTreeView(QWidget* parent = 0)
        : QTreeView(parent){}
    ~GameTokenTreeView() {}

protected:
    void paintEvent(QPaintEvent* event)
    {
        if (model() && model()->rowCount() > 0)
        {
            QTreeView::paintEvent(event);
        }
        else
        {
            const QMargins margins(2, 2, 2, 2);
            QPainter painter(viewport());
            QString text(tr("There are no items to show."));
            QRect textRect = painter.fontMetrics().boundingRect(text).marginsAdded(margins);
            textRect.moveCenter(viewport()->rect().center());
            textRect.moveTop(viewport()->rect().top());
            painter.drawText(textRect, Qt::AlignCenter, text);
        }
    }
};

/*! CGameTokenItem contain definition of particle system spawning parameters.
 *
 */
class CRYEDIT_API CGameTokenItem
    : public CBaseLibraryItem
{
public:
    CGameTokenItem();
    ~CGameTokenItem();

    virtual EDataBaseItemType GetType() const { return EDB_TYPE_GAMETOKEN; };

    virtual void SetName(const QString& name);
    void Serialize(SerializeContext& ctx);

    //! Called after particle parameters where updated.
    void Update();
    QString GetTypeName() const;
    QString GetValueString() const;
    void SetValueString(const char* sValue);
    //! Retrieve value, return false if value cannot be restored
    bool GetValue(TFlowInputData& data) const;
    //! Set value, if bUpdateGTS is true, also the GameTokenSystem's value is set
    void SetValue(const TFlowInputData& data, bool bUpdateGTS = false);

    void SetLocalOnly(bool localOnly) { m_localOnly = localOnly; };
    bool GetLocalOnly() const { return m_localOnly; };
    bool SetTypeName(const char* typeName);
    void SetDescription(const QString& sDescription) { m_description = sDescription; };
    QString GetDescription() const { return m_description; };

private:
    IGameTokenSystem* m_pTokenSystem;
    TFlowInputData m_value;
    QString m_description;
    QString m_cachedFullName;
    bool m_localOnly;
    // we cache the fullname otherwise our d'tor cannot delete the item from the GameTokenSystem
    // because before the d'tor is called the library smart ptr is set to zero sometimes
    // (see CBaseLibrary::RemoveAllItems())
    // this whole inconsistency exists because we have cyclic dependencies
    // CBaseLibraryItem and CBaseLibrary using smart pointers
};

Q_DECLARE_METATYPE(CGameTokenItem*);

#endif // CRYINCLUDE_EDITOR_GAMETOKENS_GAMETOKENITEM_H


