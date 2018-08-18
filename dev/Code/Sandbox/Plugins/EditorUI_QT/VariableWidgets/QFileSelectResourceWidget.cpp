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
#include "QFileSelectResourceWidget.h"
#include "Utils.h"
#include "UIUtils.h"

#include <QPushButton>
#include <QIcon>
#include <QEvent>
#include <QKeyEvent>
#include <QString>

#include <Controls/QBitmapPreviewDialogImp.h>

#include "AttributeView.h"

// EditorCore
#include <BaseLibrary.h>
#include <Util/smartptr.h>
#include <Util/Variable.h>
#include <Util/VariablePropertyType.h>
#include <IEditor.h>
#include <IEditorParticleUtils.h>
#include <Util/FileUtil.h>
#include <Util/PathUtil.h>
#include <QtViewPaneManager.h>

// Interface
#include <Include/IBaseLibraryManager.h>
#include <Include/IResourceSelectorHost.h>
#include <Include/IAssetBrowser.h>
#include <Include/IEditorMaterialManager.h>
#include <Material/MaterialManager.h>

#include <BaseLibraryManager.h>
#include <QKeyEvent>
#include <VariableWidgets/ui_QFileSelectWidget.h>

//to disable shortcuts while in line edit
#include <Include/IAssetTagging.h>
#include <QMessageBox>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>


QFileSelectResourceWidget::QFileSelectResourceWidget(CAttributeItem* parent, CAttributeView* attributeView,
    int propertyType)
    : QFileSelectWidget(parent)
    , CBaseVariableWidget(parent)
    , m_ignoreSetVar(false)
    , m_propertyType(propertyType)
    , m_hasFocus(false)
    , m_attributeView(attributeView)
{
    setMouseTracking(true);

    QPushButton* btn = NULL;

    // Set type specific buttons
    switch (m_propertyType)
    {
    case ePropertyTexture:
    {
        ui->path->lineEdit()->setPlaceholderText("Add a texture");

        // Open texture file in editor
        btn = addButton("Open Input Bindings Editor", tr("Open File"), 1, 0, 1, 6);
        connect(btn, &QPushButton::clicked, this, &QFileSelectResourceWidget::OpenSourceFile);
        m_btns.push_back(btn);
        m_btns.back()->installEventFilter(this);

        // select Texture file from browser
        QIcon icon("Editor/UI/Icons/mann_folder.png");
        setMainButton("", tr("Browse"), &icon);
        
        break;
    }
    case ePropertyMaterial:
    {
        //CreateDefaultResourcePreview();
        // Open AssetBrowser
        ui->path->lineEdit()->setPlaceholderText("Add a material");
        QIcon icon("Editor/UI/Icons/mann_folder.png");
        setMainButton("", tr("Browse"), &icon);

         // Open material editor
        btn = addButton("Open Input Bindings Editor", tr("Open File"), 1, 0, 1, 6);
        connect(btn, &QPushButton::clicked, this, &QFileSelectResourceWidget::OpenSourceFile);
        m_btns.push_back(btn);
        m_btns.back()->installEventFilter(this);

        break;
    }
    case ePropertyModel:
    {
        ui->path->lineEdit()->setPlaceholderText("Add a model");

        // Open Resource browser
        QIcon icon("Editor/UI/Icons/mann_folder.png");
        setMainButton("", tr("Open AssetBrowser"), &icon);
        break;
    }
    case ePropertyAudioTrigger:
    {
        ui->path->lineEdit()->setPlaceholderText("Add a sound");

        // Open Resource browser
        QIcon icon("Editor/UI/Icons/mann_folder.png");
        setMainButton("", tr("Open AssetBrowser"), &icon);
        break;
    }
    default:
        break;
    }
    ui->buttonOpenFileDialog->installEventFilter(this);
    ui->path->installEventFilter(this);

    m_tooltip = new QToolTipWidget(this);
}

void QFileSelectResourceWidget::setPath(const QString& path)
{
    SelfCallFence(m_ignoreSetVar);

    QString newPath = path;

    newPath = newPath.toLower();

    if (newPath.size() > MAX_PATH)
    {
        newPath.resize(MAX_PATH);
    }

    newPath.replace("\\\\", "/");

    if (m_propertyType == ePropertyMaterial && newPath.endsWith(".mtl"))
    {
        newPath.chop(4);
    }

     //display a shortened path
    //HardCoded project absolute path for test 
    QString relativePath = Path::GetRelativePath(newPath.toUtf8().data(), false);
    QString formattedPath = relativePath;
    relativePath = formattedPath.trimmed();
    //Get relative part.
    QString oldPath;
    m_var->Get(oldPath);
    if (oldPath != relativePath)
    {
        m_var->Set(relativePath);
        emit m_parent->SignalUndoPoint();
    }
    QFileSelectWidget::setPath(QString(relativePath));
    addUniqueItem(QString(relativePath), QString(relativePath));
}

void QFileSelectResourceWidget::onOpenSelectDialog()
{
    SResourceSelectorContext x;
    x.typeName = Prop::GetPropertyTypeToResourceType((PropertyType)m_propertyType);
    //get the path from the variable for use in the shortcut as the display path is shortened and invalid.
    QString currPath = "";
    getVar()->Get(currPath);
    QString newPath;

    // remove shortened path prefix - ".../"
    currPath.replace(QString(".../"), QString(""));

    switch (m_propertyType)
    {
    case ePropertyTexture:
    {
        AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Texture");
        AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
        if (selection.IsValid())
        {
            newPath = selection.GetResult()->GetFullPath().c_str();
        }
        break;
    }
    case ePropertyMaterial:
    {
        AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("All");
        AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
        if (selection.IsValid())
        {
            newPath = selection.GetResult()->GetFullPath().c_str();
        }
        break;
    }
    case ePropertyModel:
    {
        AssetSelectionModel selection = AssetSelectionModel::AssetGroupSelection("Geometry");
        AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
        if (selection.IsValid())
        {
            newPath = selection.GetResult()->GetRelativePath().c_str();
        }
        break;
    }
    case ePropertyAudioTrigger:
        newPath = GetIEditor()->GetResourceSelectorHost()->SelectResource(x, currPath);
        break;
    default:
        break;
    }

    setPath(newPath);
}

void QFileSelectResourceWidget::onReturnPressed()
{
    QFileSelectWidget::onReturnPressed();

    QString path = getDisplayPath();
    bool ItemAdded = addUniqueItem(path, path);
    if (ItemAdded)
    {
        setPath(path);
    }
}

void QFileSelectResourceWidget::onSelectedIndexChanged(int index)
{
    QString dataPath = ui->path->itemData(index).toString();
    bool isNewAdded = false;
    //If the item data is empty and the item text is not empty. We are trying to add new item
    if (ui->path->itemData(index).toString().isEmpty() && !ui->path->itemText(index).isEmpty())
    {
        dataPath = getDisplayPath();
        ui->path->removeItem(index);
    }
    setPath(dataPath);
}

QString QFileSelectResourceWidget::pathFilter(const QString& path)
{
    return path;
}

void QFileSelectResourceWidget::setVar(IVariable* var)
{
    onVarChanged(var);
}

void QFileSelectResourceWidget::onVarChanged(IVariable* var)
{
    if (m_ignoreSetVar)
    {
        return;
    }
    QString newPath;
    var->Get(newPath);

    newPath = newPath.toLower();

    if (newPath.size() > MAX_PATH)
    {
        newPath.resize(MAX_PATH);
    }

    newPath.replace("\\\\", "/");

    if (m_propertyType == ePropertyMaterial && newPath.endsWith(".mtl"))
    {
        newPath.chop(4);
    }

    QSignalBlocker blocker(this);
    setPath(newPath);
}

bool QFileSelectResourceWidget::eventFilter(QObject* obj, QEvent* event)
{
    for (unsigned int i = 0; i < m_btns.count(); i++)
    {
        if (obj == (QObject*)m_btns[i])
        {
            if (event->type() == QEvent::ToolTip)
            {
                QHelpEvent* e = (QHelpEvent*)event;

                GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_var) + "." + m_btns[i]->toolTip(), "Button", "", isEnabled());

                m_tooltip->TryDisplay(e->globalPos(), m_btns[i], QToolTipWidget::ArrowDirection::ARROW_RIGHT);

                return true;
            }
        }
    }
    if (obj == (QObject*)ui->buttonOpenFileDialog)
    {
        if (event->type() == QEvent::ToolTip)
        {
            QHelpEvent* e = (QHelpEvent*)event;

            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_var) + "." + ui->buttonOpenFileDialog->toolTip(), "Button", "", isEnabled());

            m_tooltip->TryDisplay(e->globalPos(), ui->buttonOpenFileDialog, QToolTipWidget::ArrowDirection::ARROW_RIGHT);

            return true;
        }
    }
    if (obj == (QObject*)ui->path)
    {
        if (event->type() == QEvent::FocusIn)
        {
            //disable hotkeys while focused on path
            GetIEditor()->GetParticleUtils()->HotKey_SetEnabled(false);
        }
        if (event->type() == QEvent::FocusOut)
        {
            //disable hotkeys while focused on path
            GetIEditor()->GetParticleUtils()->HotKey_SetEnabled(true);
        }
        if (event->type() == QEvent::MouseButtonPress)
        {
            m_tooltip->hide();
        }
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = (QKeyEvent*)event;
            Qt::KeyboardModifiers mods = keyEvent->modifiers();
            //The tooltip has functionality with the following modifiers. We dont want to hide the tooltip in these case.
            //  Alt: show alpha channel of the texture;
            //  shift : show RGBA channel of the texture.
            if ((mods & Qt::KeyboardModifier::AltModifier) || 
                (mods & Qt::KeyboardModifier::ShiftModifier && !(mods & Qt::KeyboardModifier::ControlModifier)))
            {
                return true;
            }
            m_tooltip->hide();
        }
        if (event->type() == QEvent::ToolTip)
        {
            QHelpEvent* e = (QHelpEvent*)event;

            QString filePath;
            getVar()->Get(filePath);
            GetIEditor()->GetParticleUtils()->ToolTip_BuildFromConfig(m_tooltip, m_parent->getAttributeView()->GetVariablePath(m_var), QString(filePath), QString(m_parent->getVar()->GetDisplayValue()), isEnabled());

            m_tooltip->TryDisplay(e->globalPos(), ui->path, QToolTipWidget::ArrowDirection::ARROW_RIGHT);

            return true;
        }
        if (event->type() == QEvent::MouseButtonDblClick)
        {
            QMouseEvent* mev = static_cast<QMouseEvent*>(event);
            if (mev->button() == Qt::LeftButton)
            {
                ui->path->lineEdit()->selectAll();
                return true;
            }
        }
    }
    if (event->type() == QEvent::Leave)
    {
        if (m_tooltip->isVisible())
        {
            m_tooltip->hide();
        }
    }
    return QFileSelectWidget::eventFilter(obj, event);
}

void QFileSelectResourceWidget::OpenSourceFile()
{
    QString filePath;
    getVar()->Get(filePath);
    
    switch (m_propertyType)
    {
    case ePropertyTexture:
    {
        while (filePath[0] == '/' || filePath[0] == '\\')
        {
            filePath = filePath.mid(1);
        }
        filePath.replace("\\", "/");
        filePath = filePath.mid(0, filePath.lastIndexOf('.'));
        QString folderPath = filePath;
        folderPath = folderPath.mid(0, filePath.lastIndexOf('/'));
        QDir dir(folderPath);
        if (!dir.exists())
        {
            folderPath = GetIEditor()->GetAssetTagging()->GetProjectName() + "/" + folderPath;
            dir.setPath(folderPath);
        }

        QString filename;
        filename = filePath.mid(filePath.lastIndexOf('/') + 1, filePath.length() - filePath.lastIndexOf('/'));
        filename.append(".*");

        QStringList files;
        files = dir.entryList(QStringList(filename), QDir::Files | QDir::NoSymLinks);
        // Looking for file with psd type
        if (files.size() > 0)
        {
            QString finalPath = dir.absolutePath() + "/" + files.first(); // Contribute Full path first
            GetIEditor()->GetFileUtil()->EditTextureFile(finalPath.toUtf8().data(), false);
            break;
        }
        // Not found, print error message
        GetIEditor()->GetEnv()->pLog->LogError("Failed to find souce file for texture: '%s'", filePath.toUtf8().data());
        QMessageBox::warning(this, "Warning", "Failed to find source file for texture: '" + filePath + "'", "Ok");
        break;
    }
    case ePropertyMaterial:
    {
        CMaterial *mat = GetIEditor()->GetMaterialManager()->LoadMaterial(filePath.toUtf8().data(), false);
        GetIEditor()->GetMaterialManager()->SetSelectedItem(mat);
        QtViewPaneManager::instance()->OpenPane(LyViewPane::MaterialEditor);
        break;
    }
    case ePropertyModel:
    case ePropertyAudioTrigger:
    default:
        break;
    }
}

#include <VariableWidgets/QFileSelectResourceWidget.moc>
