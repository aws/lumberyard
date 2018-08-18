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

#include "StdAfx.h"
#include "IEditorParticleUtils.h"
#include "EditorParticleUtils.h"
#include "Viewport.h"
#include "EditorCoreAPI.h"
#include "IEditor.h"
#include "Objects/EntityObject.h"
#include "BaseLibraryItem.h"
#include "CryEditDoc.h"
#include "Particles/ParticleItem.h"
#include "Particles/ParticleManager.h"
#include "ViewManager.h"
#include "Objects/AxisGizmo.h"
#include "IGizmoManager.h"
#include "RenderHelpers/AxisHelper.h"
#include "qlogging.h"
#include "../Plugins/EditorUI_QT/PreviewModelView.h"
#include "ParticleParams.h"
#include "Clipboard.h"
#include "qinputdialog.h"
#include <QWidget>
#include <QKeyEvent>
#include "IAssetBrowser.h"
#include "RotateTool.h"
#include "qapplication.h"
#include "qfiledialog.h"
#include "qxmlstream.h"
#include "qsettings.h"
#include "Controls/QToolTipWidget.h"

#ifndef PI
#define PI 3.14159265358979323f
#endif

class DummyBaseObject
    : public CBaseObject
{
public:
    virtual void Display(DisplayContext& disp)
    {
        return; // no display
    }

    virtual void DeleteThis()
    {
        delete this;
    }
    virtual bool IsRotatable() const { return true; }
    virtual bool IsScalable() const { return false; }
};


struct ToolTip
{
    bool isValid;
    QString title;
    QString content;
    QString specialContent;
    QString disabledContent;
};

// internal implementation for better compile times - should also never be used externally, use IParticleEditorUtils interface for that.
class CEditorParticleUtils_Impl
    : public IEditorParticleUtils
{
    #pragma region Particle Drag & Drop
public:
    virtual void SetViewportDragOperation(void(* dropCallback)(CViewport* viewport, int dragPointX, int dragPointY, void* custom), void* custom) override
    {
        for (int i = 0; i < GetIEditor()->GetViewManager()->GetViewCount(); i++)
        {
            GetIEditor()->GetViewManager()->GetView(i)->SetGlobalDropCallback(dropCallback, custom);
        }
    }

    virtual bool AssignParticleToEntity(CParticleItem* pItem, CBaseObject* pObject, Vec3 const* pPos)
    {
        // Assign ParticleEffect field if it has one.
        // Otherwise, spawn/attach an emitter to the entity
        assert(pItem);
        assert(pObject);
        if (qobject_cast<CEntityObject*>(pObject))
        {
            CEntityObject* pEntity = (CEntityObject*)pObject;
            IVariable* pVar = 0;
            if (pEntity->GetProperties())
            {
                pVar = pEntity->GetProperties()->FindVariable("ParticleEffect");
                if (pVar && pVar->GetType() == IVariable::STRING)
                {
                    // Set selected entity's ParticleEffect field.
                    pVar->Set(pItem->GetFullName());
                    if (CEntityScript* pScript = pEntity->GetScript())
                    {
                        pScript->SendEvent(pEntity->GetIEntity(), "Restart");
                    }
                }
                else
                {
                    // Create a new ParticleEffect entity on top of selected entity, attach to it.
                    Vec3 pos;
                    if (pPos)
                    {
                        pos = *pPos;
                    }
                    else
                    {
                        pos = pObject->GetPos();
                        AABB box;
                        pObject->GetLocalBounds(box);
                        pos.z += box.max.z;
                    }

                    CreateParticleEntity(pItem, pos, pObject);
                }
                return true;
            }
        }
        return false;
    }

    virtual CBaseObject* CreateParticleEntity(CParticleItem* pItem, Vec3 const& pos, CBaseObject* pParent = nullptr) override
    {
        if (!GetIEditor()->GetDocument()->IsDocumentReady())
        {
            return nullptr;
        }

        GetIEditor()->ClearSelection();
        CBaseObject* pObject = GetIEditor()->NewObject("ParticleEntity", "ParticleEffect");
        if (pObject)
        {
            // Set pos, offset by object size.
            AABB box;
            pObject->GetLocalBounds(box);
            pObject->SetPos(pos - Vec3(0, 0, box.min.z));
            pObject->SetRotation(Quat::CreateRotationXYZ(Ang3(0, 0, 0)));
            pObject->SetFlags(OBJFLAG_IS_PARTICLE);
            if (pParent)
            {
                pParent->AttachChild(pObject);
            }
            AssignParticleToEntity(pItem, pObject, nullptr);
            GetIEditor()->SelectObject(pObject);
            GetIEditor()->SetEditMode(eEditModeMove);
        }
        return pObject;
    }

    virtual void SpawnParticleAtCoordinate(CViewport* viewport, CParticleItem* pItem, int ptx, int pty)
    {
        CUndo undo("Drag & Drop ParticleEffect");

        if (viewport)
        {
            QPoint vp(ptx, pty);
            CParticleItem* pParticles = pItem;

            // Drag and drop into one of views.
            HitContext  hit;
            if (viewport->HitTest(vp, hit))
            {
                Vec3 hitpos = hit.raySrc + hit.rayDir * hit.dist;
                if (hit.object && AssignParticleToEntity(pParticles, hit.object, &hitpos))
                {
                    ; // done
                }
                else
                {
                    // Place emitter at hit location.
                    hitpos = viewport->SnapToGrid(hitpos);
                    CreateParticleEntity(pParticles, hitpos);
                }
            }
            else
            {
                // Snap to terrain.
                bool hitTerrain;
                Vec3 pos = viewport->ViewToWorld(vp, &hitTerrain);
                if (hitTerrain)
                {
                    pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y);
                }
                pos = viewport->SnapToGrid(pos);
                CreateParticleEntity(pParticles, pos);
            }
        }
        GetIEditor()->SetEditMode(eEditModeSelect);
    }

    virtual bool AssignParticleToSelection(CParticleItem* pItem)
    {
        CUndo undo("Assign ParticleEffect to selection");

        CSelectionGroup* pSel = GetIEditor()->GetSelection();
        if (!pSel->IsEmpty())
        {
            for (int i = 0; i < pSel->GetCount(); i++)
            {
                AssignParticleToEntity(pItem, pSel->GetObject(i), nullptr);
            }
        }

        return true;
    }

    virtual CBaseLibraryItem* GetSelectedParticleEffectItem()
    {
        CSelectionGroup* pSel = GetIEditor()->GetSelection();
        for (int i = 0; i < pSel->GetCount(); i++)
        {
            CBaseObject* pObject = pSel->GetObject(i);
            if (qobject_cast<CEntityObject*>(pObject))
            {
                CEntityObject* pEntity = (CEntityObject*)pObject;
                if (pEntity->GetProperties())
                {
                    IVariable* pVar = pEntity->GetProperties()->FindVariable("ParticleEffect");
                    if (pVar)
                    {
                        QString effect;
                        pVar->Get(effect);
                        {
                            CBaseLibraryItem* pItem = (CBaseLibraryItem*)GetIEditor()->GetParticleManager()->LoadItemByName(effect);
                            if (pItem)
                            {
                                return pItem;
                            }
                        }
                    }
                }
            }
        }
        return NULL;
    }
    #pragma endregion
    #pragma region Preview Window
public:

    virtual int PreviewWindow_GetDisplaySettingsDebugFlags(CDisplaySettings* settings)
    {
        CRY_ASSERT(settings);
        return settings->GetDebugFlags();
    }

    virtual void PreviewWindow_SetDisplaySettingsDebugFlags(CDisplaySettings* settings, int flags)
    {
        CRY_ASSERT(settings);
        settings->SetDebugFlags(flags);
    }

    #pragma endregion
    #pragma region DefaultEmitter
protected:
    QVector<DefaultEmitter> emitters;
    DefaultEmitter currentEmitter;
public:
    virtual DefaultEmitterRef DefaultEmitter_Create(DefaultEmitter& data) override
    {
        for (int i = 0; i < emitters.count(); i++)
        {
            if (strcmp(emitters[i].name, data.name) == 0)
            {
                emitters[i].node = data.node;
                emitters[i].current = true;
                currentEmitter.CopyFrom(emitters[i]);
                return (DefaultEmitterRef) & currentEmitter;
            }
        }
        emitters.push_back(data);
        currentEmitter.CopyFrom(emitters.last());
        return (DefaultEmitterRef) & currentEmitter;
    }

    virtual bool DefaultEmitter_SetCurrent(const char* name, int size) override
    {
        for (int i = 0; i < emitters.count(); i++)
        {
            if (strcmp(emitters[i].name, name) == 0)
            {
                emitters[i].current = true;
                currentEmitter.CopyFrom(emitters[i]);
                return true;
            }
            else
            {
                emitters[i].current = false;
            }
        }
        return false;
    }

    virtual void DefaultEmitter_SetColor(int r, int g, int b, int a) override
    {
        XmlNodeRef node = GetIEditor()->GetSystem()->LoadXmlFromBuffer(currentEmitter.node, strlen(currentEmitter.node));
        CParticleItem* loadedEmitter = new CParticleItem();
        GetIEditor()->GetParticleManager()->PasteToParticleItem(loadedEmitter, node, true);
        if (loadedEmitter)
        {
            ParticleParams params = loadedEmitter->GetEffect()->GetParticleParams();

            float _r, _g, _b, _a;
            _r = r / 255.0f;
            _g = g / 255.0f;
            _b = b / 255.0f;
            _a = a / 255.0f;
            params.cColor.Set(Color3F(_r, _g, _b));
            params.fAlpha.Set(_a);
            loadedEmitter->GetEffect()->SetParticleParams(params);

            node = GetIEditor()->GetSystem()->CreateXmlNode("Particles");
            CBaseLibraryItem::SerializeContext ctx(node, false);
            ctx.bCopyPaste = true;

            loadedEmitter->Serialize(ctx);

            azstrcpy(currentEmitter.node, node->getXML().size(), node->getXML().c_str());
        }
    }

    virtual void DefaultEmitter_SetTexture(char* file) override
    {
        if (strlen(file) <= 0)
        {
            return;
        }
        XmlNodeRef node = GetIEditor()->GetSystem()->LoadXmlFromBuffer(currentEmitter.node, strlen(currentEmitter.node));
        CParticleItem* loadedEmitter = new CParticleItem();
        if (node)
        {
            GetIEditor()->GetParticleManager()->PasteToParticleItem(loadedEmitter, node, true);
        }
        if (loadedEmitter)
        {
            ParticleParams params = loadedEmitter->GetEffect()->GetParticleParams();
            params.sTexture = file;
            loadedEmitter->GetEffect()->SetParticleParams(params);

            node = GetIEditor()->GetSystem()->CreateXmlNode("Particles");
            CBaseLibraryItem::SerializeContext ctx(node, false);
            ctx.bCopyPaste = true;

            loadedEmitter->Serialize(ctx);

            azstrcpy(currentEmitter.node, node->getXML().size(), node->getXML().c_str());
        }
    }

    virtual DefaultEmitterRef DefaultEmitter_GetCurrentEmitter() override
    {
        return (DefaultEmitterRef) & currentEmitter;
    }

    virtual void DefaultEmitter_SaveCurrentEmitter() override
    {
        QString emitterName = QInputDialog::getText(NULL, "Select Emitter", "Save Emitter As: ", QLineEdit::Normal, currentEmitter.name);
        if (emitterName.isEmpty())
        {
            return;
        }
        azstrcpy(currentEmitter.name, emitterName.size() + 1, emitterName.toStdString().c_str());
        emitters.push_back(currentEmitter);
    }

    virtual DefaultEmitterRefArray DefaultEmitter_GetAllSavedEmitters() override
    {
        DefaultEmitter* emitterArray = new DefaultEmitter[emitters.count()];
        for (int i = 0; i < emitters.count(); i++)
        {
            emitterArray[i].current = emitters[i].current;
            emitterArray[i].node = emitters[i].node;
            emitterArray[i].name = emitters[i].name;
        }
        return (DefaultEmitterRefArray)emitterArray;
    }

    virtual int DefaultEmitter_GetCount() override
    {
        return emitters.count();
    }

    #pragma endregion
    #pragma region Shortcuts
protected:
    QVector<HotKey> hotkeys;
    bool m_hotkeysAreEnabled;
public:

    virtual bool HotKey_Import() override
    {
        QVector<QPair<QString, QString> > keys;
        QString filepath = QFileDialog::getOpenFileName(nullptr, "Select shortcut configuration to load",
                QString(), "HotKey Config Files (*.hkxml)");
        QFile file(filepath);
        if (!file.open(QIODevice::ReadOnly))
        {
            return false;
        }
        QXmlStreamReader stream(&file);
        bool result = true;

        while (!stream.isEndDocument())
        {
            if (stream.isStartElement())
            {
                if (stream.name() == "HotKey")
                {
                    QPair<QString, QString> key;
                    QXmlStreamAttributes att = stream.attributes();
                    for (QXmlStreamAttribute attr : att)
                    {
                        if (attr.name().compare("path", Qt::CaseInsensitive) == 0)
                        {
                            key.first = attr.value().toString();
                        }
                        if (attr.name().compare("sequence", Qt::CaseInsensitive) == 0)
                        {
                            key.second = attr.value().toString();
                        }
                    }
                    if (!key.first.isEmpty())
                    {
                        keys.push_back(key); // we allow blank key sequences for unassigned shortcuts
                    }
                    else
                    {
                        result = false; //but not blank paths!
                    }
                }
            }
            stream.readNext();
        }
        file.close();

        if (result)
        {
            HotKey_BuildDefaults();
            for (QPair<QString, QString> key : keys)
            {
                for (unsigned int j = 0; j < hotkeys.count(); j++)
                {
                    if (hotkeys[j].path.compare(key.first, Qt::CaseInsensitive) == 0)
                    {
                        hotkeys[j].SetPath(key.first.toStdString().c_str());
                        hotkeys[j].SetSequenceFromString(key.second.toStdString().c_str());
                    }
                }
            }
        }
        return result;
    }

    virtual void HotKey_Export() override
    {
        QString filepath = QFileDialog::getSaveFileName(nullptr, "Select shortcut configuration to load", "Editor/Plugins/ParticleEditorPlugin/settings", "HotKey Config Files (*.hkxml)");
        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly))
        {
            return;
        }

        QXmlStreamWriter stream(&file);
        stream.setAutoFormatting(true);
        stream.writeStartDocument();
        stream.writeStartElement("HotKeys");

        for (HotKey key : hotkeys)
        {
            stream.writeStartElement("HotKey");
            stream.writeAttribute("path", key.path);
            stream.writeAttribute("sequence", key.sequence.toString());
            stream.writeEndElement();
        }
        stream.writeEndElement();
        stream.writeEndDocument();
        file.close();
    }

    virtual QKeySequence HotKey_GetShortcut(const char* path) override
    {
        for (HotKey combo : hotkeys)
        {
            if (combo.IsMatch(path))
            {
                return combo.sequence;
            }
        }
        return QKeySequence();
    }

    virtual bool HotKey_IsPressed(const QKeyEvent* event, const char* path) override
    {
        if (!m_hotkeysAreEnabled)
        {
            return false;
        }
        unsigned int keyInt = 0;
        //Capture any modifiers
        Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
        if (modifiers & Qt::ShiftModifier)
        {
            keyInt += Qt::SHIFT;
        }
        if (modifiers & Qt::ControlModifier)
        {
            keyInt += Qt::CTRL;
        }
        if (modifiers & Qt::AltModifier)
        {
            keyInt += Qt::ALT;
        }
        if (modifiers & Qt::MetaModifier)
        {
            keyInt += Qt::META;
        }
        //Capture any key
        keyInt += event->key();

        QString t0 = QKeySequence(keyInt).toString();
        QString t1 = HotKey_GetShortcut(path).toString();

        //if strings match then shortcut is pressed
        if (t1.compare(t0, Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        return false;
    }

    virtual bool HotKey_IsPressed(const QShortcutEvent* event, const char* path) override
    {
        if (!m_hotkeysAreEnabled)
        {
            return false;
        }

        QString t0 = event->key().toString();
        QString t1 = HotKey_GetShortcut(path).toString();

        //if strings match then shortcut is pressed
        if (t1.compare(t0, Qt::CaseInsensitive) == 0)
        {
            return true;
        }
        return false;
    }

    virtual bool HotKey_LoadExisting() override
    {
        QSettings settings("Amazon", "Lumberyard");
        QString group = "Hotkeys/";

        HotKey_BuildDefaults();

        int size = settings.beginReadArray(group);

        for (int i = 0; i < size; i++)
        {
            settings.setArrayIndex(i);
            QPair<QString, QString> hotkey;
            hotkey.first = settings.value("name").toString();
            hotkey.second = settings.value("keySequence").toString();
            if (!hotkey.first.isEmpty())
            {
                for (unsigned int j = 0; j < hotkeys.count(); j++)
                {
                    if (hotkeys[j].path.compare(hotkey.first, Qt::CaseInsensitive) == 0)
                    {
                        hotkeys[j].SetPath(hotkey.first.toStdString().c_str());
                        hotkeys[j].SetSequenceFromString(hotkey.second.toStdString().c_str());
                    }
                }
            }
        }

        settings.endArray();
        if (hotkeys.isEmpty())
        {
            return false;
        }
        return true;
    }

    virtual void HotKey_SaveCurrent() override
    {
        QSettings settings("Amazon", "Lumberyard");
        QString group = "Hotkeys/";
        settings.remove("Hotkeys/");
        settings.sync();
        settings.beginWriteArray(group);
        int hotkeyCount = hotkeys.count();
        int saveIndex = 0;
        for (HotKey key : hotkeys)
        {
            if (!key.path.isEmpty())
            {
                settings.setArrayIndex(saveIndex++);
                settings.setValue("name", key.path);
                settings.setValue("keySequence", key.sequence.toString());
            }
        }
        settings.endArray();
        settings.sync();
    }

    virtual void HotKey_BuildDefaults() override
    {
        m_hotkeysAreEnabled = true;
        QVector<QPair<QString, QString> > keys;
        while (hotkeys.count() > 0)
        {
            hotkeys.takeAt(0);
        }

        //MENU SELECTION SHORTCUTS////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("Menus.File Menu", "Alt+F"));
        keys.push_back(QPair<QString, QString>("Menus.Edit Menu", "Alt+E"));
        keys.push_back(QPair<QString, QString>("Menus.View Menu", "Alt+V"));
        //FILE MENU SHORTCUTS/////////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("File Menu.Create new emitter", "Ctrl+N"));
        keys.push_back(QPair<QString, QString>("File Menu.Create new library", "Ctrl+Shift+N"));
        keys.push_back(QPair<QString, QString>("File Menu.Create new folder", ""));
        keys.push_back(QPair<QString, QString>("File Menu.Import", "Ctrl+I"));
        keys.push_back(QPair<QString, QString>("File Menu.Import level library", "Ctrl+Shift+I"));
        keys.push_back(QPair<QString, QString>("File Menu.Save", "Ctrl+S"));
        keys.push_back(QPair<QString, QString>("File Menu.Close", "Ctrl+Q"));
        //EDIT MENU SHORTCUTS/////////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("Edit Menu.Copy", "Ctrl+C"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Paste", "Ctrl+V"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Duplicate", "Ctrl+D"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Undo", "Ctrl+Z"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Redo", "Ctrl+Shift+Z"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Group", "Ctrl+G"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Ungroup", "Ctrl+Shift+G"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Rename", "Ctrl+R"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Reset", ""));
        keys.push_back(QPair<QString, QString>("Edit Menu.Edit Hotkeys", ""));
        keys.push_back(QPair<QString, QString>("Edit Menu.Assign to selected", "Ctrl+Space"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Insert Comment", "Ctrl+Alt+M"));
        keys.push_back(QPair<QString, QString>("Edit Menu.Enable/Disable Emitter", "Ctrl+E"));
        keys.push_back(QPair<QString, QString>("File Menu.Enable All", ""));
        keys.push_back(QPair<QString, QString>("File Menu.Disable All", ""));
        keys.push_back(QPair<QString, QString>("Edit Menu.Delete", "Del"));
        //VIEW MENU SHORTCUTS/////////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("View Menu.Reset Layout", ""));
        //PLAYBACK CONTROL////////////////////////////////////////////////////////
        keys.push_back(QPair<QString, QString>("Previewer.Play/Pause Toggle", "Space"));
        keys.push_back(QPair<QString, QString>("Previewer.Step forward through time", "c"));
        keys.push_back(QPair<QString, QString>("Previewer.Loop Toggle", "z"));
        keys.push_back(QPair<QString, QString>("Previewer.Reset Playback", "x"));
        keys.push_back(QPair<QString, QString>("Previewer.Focus", "Ctrl+F"));
        keys.push_back(QPair<QString, QString>("Previewer.Zoom In", "w"));
        keys.push_back(QPair<QString, QString>("Previewer.Zoom Out", "s"));
        keys.push_back(QPair<QString, QString>("Previewer.Pan Left", "a"));
        keys.push_back(QPair<QString, QString>("Previewer.Pan Right", "d"));

        for (QPair<QString, QString> key : keys)
        {
            unsigned int index = hotkeys.count();
            hotkeys.push_back(HotKey());
            hotkeys[index].SetPath(key.first.toStdString().c_str());
            hotkeys[index].SetSequenceFromString(key.second.toStdString().c_str());
        }
    }

    virtual void HotKey_SetKeys(QVector<HotKey> keys) override
    {
        hotkeys = keys;
    }

    virtual QVector<HotKey> HotKey_GetKeys() override
    {
        return hotkeys;
    }

    virtual QString HotKey_GetPressedHotkey(const QKeyEvent* event) override
    {
        if (!m_hotkeysAreEnabled)
        {
            return "";
        }
        for (HotKey key : hotkeys)
        {
            if (HotKey_IsPressed(event, key.path.toUtf8()))
            {
                return key.path;
            }
        }
        return "";
    }
    virtual QString HotKey_GetPressedHotkey(const QShortcutEvent* event) override
    {
        if (!m_hotkeysAreEnabled)
        {
            return "";
        }
        for (HotKey key : hotkeys)
        {
            if (HotKey_IsPressed(event, key.path.toUtf8()))
            {
                return key.path;
            }
        }
        return "";
    }
    //building the default hotkey list re-enables hotkeys
    //do not use this when rebuilding the default list is a possibility.
    virtual void HotKey_SetEnabled(bool val) override
    {
        m_hotkeysAreEnabled = val;
    }

    virtual bool HotKey_IsEnabled() const override
    {
        return m_hotkeysAreEnabled;
    }

    #pragma  endregion
    #pragma region ToolTip
protected:
    QMap<QString, ToolTip> m_tooltips;

    void ToolTip_ParseNode(XmlNodeRef node)
    {
        if (QString(node->getTag()).compare("tooltip", Qt::CaseInsensitive) != 0)
        {
            unsigned int childCount = node->getChildCount();

            for (unsigned int i = 0; i < childCount; i++)
            {
                ToolTip_ParseNode(node->getChild(i));
            }
        }

        QString title = node->getAttr("title");
        QString content = node->getAttr("content");
        QString specialContent = node->getAttr("special_content");
        QString disabledContent = node->getAttr("disabled_content");

        QMap<QString, ToolTip>::iterator itr = m_tooltips.insert(node->getAttr("path"), ToolTip());
        itr->isValid = true;
        itr->title = title;
        itr->content = content;
        itr->specialContent = specialContent;
        itr->disabledContent = disabledContent;

        unsigned int childCount = node->getChildCount();

        for (unsigned int  i = 0; i < childCount; i++)
        {
            ToolTip_ParseNode(node->getChild(i));
        }
    }

    ToolTip GetToolTip(QString path)
    {
        if (m_tooltips.contains(path))
        {
            return m_tooltips[path];
        }
        ToolTip temp;
        temp.isValid = false;
        return temp;
    }

public:
    virtual void ToolTip_LoadConfigXML(QString filepath) override
    {
        XmlNodeRef node = GetIEditor()->GetSystem()->LoadXmlFromFile(filepath.toStdString().c_str());
        ToolTip_ParseNode(node);
    }

    virtual void ToolTip_BuildFromConfig(QToolTipWidget* tooltip, QString path, QString option, QString optionalData = "", bool isEnabled = true)
    {
        AZ_Assert(tooltip, "tooltip cannot be null");

        QString title = ToolTip_GetTitle(path, option);
        QString content = ToolTip_GetContent(path, option);
        QString specialContent = ToolTip_GetSpecialContentType(path, option);
        QString disabledContent = ToolTip_GetDisabledContent(path, option);

        // Even if these items are empty, we set them anyway to clear out any data that was left over from when the tooltip was used for a different object.
        tooltip->SetTitle(title);
        tooltip->SetContent(content);

        //this only handles simple creation...if you need complex call this then add specials separate
        if (!specialContent.contains("::"))
        {
            tooltip->AddSpecialContent(specialContent, optionalData);
        }

        if (!isEnabled) // If disabled, add disabled value
        {
            tooltip->AppendContent(disabledContent);
        }
    }

    virtual QString ToolTip_GetTitle(QString path, QString option) override
    {
        if (!option.isEmpty() && GetToolTip(path + "." + option).isValid)
        {
            return GetToolTip(path + "." + option).title;
        }
        if (!option.isEmpty() && GetToolTip("Options." + option).isValid)
        {
            return GetToolTip("Options." + option).title;
        }
        return GetToolTip(path).title;
    }

    virtual QString ToolTip_GetContent(QString path, QString option) override
    {
        if (!option.isEmpty() && GetToolTip(path + "." + option).isValid)
        {
            return GetToolTip(path + "." + option).content;
        }
        if (!option.isEmpty() && GetToolTip("Options." + option).isValid)
        {
            return GetToolTip("Options." + option).content;
        }
        return GetToolTip(path).content;
    }

    virtual QString ToolTip_GetSpecialContentType(QString path, QString option) override
    {
        if (!option.isEmpty() && GetToolTip(path + "." + option).isValid)
        {
            return GetToolTip(path + "." + option).specialContent;
        }
        if (!option.isEmpty() && GetToolTip("Options." + option).isValid)
        {
            return GetToolTip("Options." + option).specialContent;
        }
        return GetToolTip(path).specialContent;
    }

    virtual QString ToolTip_GetDisabledContent(QString path, QString option) override
    {
        if (!option.isEmpty() && GetToolTip(path + "." + option).isValid)
        {
            return GetToolTip(path + "." + option).disabledContent;
        }
        if (!option.isEmpty() && GetToolTip("Options." + option).isValid)
        {
            return GetToolTip("Options." + option).disabledContent;
        }
        return GetToolTip(path).disabledContent;
    }
    #pragma endregion ToolTip
};

IEditorParticleUtils* CreateEditorParticleUtils()
{
    return new CEditorParticleUtils_Impl();
}

