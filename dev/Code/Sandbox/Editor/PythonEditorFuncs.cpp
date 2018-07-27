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
#include "IEditor.h"
#include "GameEngine.h"
#include "ViewManager.h"
#include "Util/BoostPythonHelpers.h"
#include "StringDlg.h"
#include "GenericSelectItemDialog.h"
#include "Util/Ruler.h"
#include "Util/Mailer.h"
#include "UndoCVar.h"
#include "DisplaySettings.h"

#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>

#include <QFileDialog>

namespace
{
    //////////////////////////////////////////////////////////////////////////
    const char* PyGetCVar(const char* pName)
    {
        ICVar* pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(pName);
        if (!pCVar)
        {
            Warning("PyGetCVar: Attempt to access non-existent CVar '%s'", pName ? pName : "(null)");
            throw std::logic_error((QString("\"") + pName + "\" is an invalid cvar.").toUtf8().data());
        }
        return pCVar->GetString();
    }
    //////////////////////////////////////////////////////////////////////////
    void PySetCVar(const char* pName, pSPyWrappedProperty pValue)
    {
        ICVar* pCVar = GetIEditor()->GetSystem()->GetIConsole()->GetCVar(pName);
        if (!pCVar)
        {
            Warning("PySetCVar: Attempt to access non-existent CVar '%s'", pName ? pName : "(null)");
            throw std::logic_error((QString("\"") + pName + " is an invalid cvar.").toUtf8().data());
        }

        CUndo undo("Set CVar");
        if (CUndo::IsRecording())
        {
            CUndo::Record(new CUndoCVar(pName));
        }

        if (pCVar->GetType() == CVAR_INT && pValue->type == SPyWrappedProperty::eType_Int)
        {
            pCVar->Set(pValue->property.intValue);
        }
        else if (pCVar->GetType() == CVAR_FLOAT && pValue->type == SPyWrappedProperty::eType_Float)
        {
            pCVar->Set(pValue->property.floatValue);
        }
        else if (pCVar->GetType() == CVAR_STRING && pValue->type == SPyWrappedProperty::eType_String)
        {
            pCVar->Set(pValue->stringValue.toUtf8().data());
        }
        else
        {
            Warning("PyGetCVar: Type mismatch while assigning CVar '%s'", pName ? pName : "(null)");
            throw std::logic_error("Invalid data type.");
        }

        CryLog("PySetCVar: %s set to %s", pName, pCVar->GetString());
    }

    //////////////////////////////////////////////////////////////////////////
    void PyEnterGameMode()
    {
        if (GetIEditor()->GetGameEngine())
        {
            GetIEditor()->GetGameEngine()->RequestSetGameMode(true);
        }
    }

    void PyExitGameMode()
    {
        if (GetIEditor()->GetGameEngine())
        {
            GetIEditor()->GetGameEngine()->RequestSetGameMode(false);
        }
    }

    bool PyIsInGameMode()
    {
        return GetIEditor()->IsInGameMode();
    }

    //////////////////////////////////////////////////////////////////////////
    QString PyNewObject(const char* typeName, const char* fileName, const char* name, float x, float y, float z)
    {
        CBaseObject* object = GetIEditor()->NewObject(typeName, fileName, name, x, y, z);
        if (object)
        {
            return object->GetName();
        }
        else
        {
            return "";
        }
    }

    pPyGameObject PyCreateObject(const char* typeName, const char* fileName, const char* name, float x, float y, float z)
    {
        CBaseObject* pObject = GetIEditor()->GetObjectManager()->NewObject(typeName, 0, fileName);

        if (pObject != NULL)
        {
            GetIEditor()->SetModifiedFlag();

            if (strcmp(typeName, "Brush") == 0)
            {
                GetIEditor()->SetModifiedModule(eModifiedBrushes);
            }
            else if (strcmp(typeName, "Entity") == 0)
            {
                GetIEditor()->SetModifiedModule(eModifiedEntities);
            }
            else
            {
                GetIEditor()->SetModifiedModule(eModifiedAll);
            }

            if (strcmp(name, "") != 0)
            {
                pObject->SetUniqueName(name);
            }

            pObject->SetPos(Vec3(x, y, z));
        }

        return PyScript::CreatePyGameObject(pObject);
    }

    //////////////////////////////////////////////////////////////////////////
    QString PyNewObjectAtCursor(const char* typeName, const char* fileName, const char* name)
    {
        CUndo undo("Create new object");

        Vec3 pos(0, 0, 0);

        QPoint p = QCursor::pos();
        CViewport* viewport = GetIEditor()->GetViewManager()->GetViewportAtPoint(p);
        if (viewport)
        {
            viewport->ScreenToClient(p);
            if (GetIEditor()->GetAxisConstrains() != AXIS_TERRAIN)
            {
                pos = viewport->MapViewToCP(p);
            }
            else
            {
                // Snap to terrain.
                bool hitTerrain;
                pos = viewport->ViewToWorld(p, &hitTerrain);
                if (hitTerrain)
                {
                    pos.z = GetIEditor()->GetTerrainElevation(pos.x, pos.y) + 1.0f;
                }
                pos = viewport->SnapToGrid(pos);
            }
        }

        return PyNewObject(typeName, fileName, name, pos.x, pos.y, pos.z);
    }

    //////////////////////////////////////////////////////////////////////////
    void PyStartObjectCreation(const char* typeName, const char* fileName)
    {
        CUndo undo("Create new object");
        GetIEditor()->StartObjectCreation(typeName, fileName);
    }

    //////////////////////////////////////////////////////////////////////////
    void PyRunConsole(const char* text)
    {
        GetIEditor()->GetSystem()->GetIConsole()->ExecuteString(text);
    }

    //////////////////////////////////////////////////////////////////////////
    void PyRunLua(const char* text)
    {
        GetIEditor()->GetSystem()->GetIScriptSystem()->ExecuteBuffer(text, strlen(text));
    }

    //////////////////////////////////////////////////////////////////////////
    bool GetPythonScriptPath(const char* pFile, QString& path)
    {
        bool bRelativePath = true;
        char drive[_MAX_DRIVE];
        drive[0] = '\0';
        _splitpath(pFile, drive, 0, 0, 0);
        if (strlen(drive) != 0)
        {
            bRelativePath = false;
        }

        if (bRelativePath)
        {
            // Try to open from user folder
            QString userSandboxFolder = Path::GetResolvedUserSandboxFolder();
            Path::ConvertBackSlashToSlash(userSandboxFolder);
            path = userSandboxFolder + pFile;

            // If not found try editor folder
            if (!CFileUtil::FileExists(path))
            {
                QString scriptFolder = QDir::currentPath() + QString("/Editor/Scripts/");
                Path::ConvertBackSlashToSlash(scriptFolder);
                path = scriptFolder + pFile;

                if (!CFileUtil::FileExists(path))
                {
                    QString error = QString("Could not find '%1'\n in '%2'\n or '%3'\n").arg(pFile).arg(userSandboxFolder).arg(scriptFolder);
                    PyScript::PrintError(error.toUtf8().data());
                    return false;
                }
            }
        }
        else
        {
            path = pFile;
            if (!CFileUtil::FileExists(path))
            {
                QString error = QString("Could not find '") + pFile + "'\n";
                PyScript::PrintError(error.toUtf8().data());
                return false;
            }
        }

        Path::ConvertBackSlashToSlash(path);
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    void GetPythonArgumentsVector(const char* pArguments, QStringList& inputArguments)
    {
        if (pArguments == NULL)
        {
            return;
        }

        inputArguments = QString(pArguments).split(" ", QString::SkipEmptyParts);
    }

    //////////////////////////////////////////////////////////////////////////
    void PyRunFileWithParameters(const char* pFile, const char* pArguments)
    {
        QString path;
        QStringList inputArguments;
        GetPythonArgumentsVector(pArguments, inputArguments);
        if (GetPythonScriptPath(pFile, path))
        {
            bool releasePythonLock = false;

            if (!PyScript::HasPythonLock())
            {
                PyScript::AcquirePythonLock();
                releasePythonLock = true;
            }

            char fileOp[] = "r";
            PyObject* pyFileObject = PyFile_FromString(const_cast<char*>(path.toUtf8().data()), fileOp);

            if (pyFileObject)
            {
                std::vector<const char*> argv;
                argv.reserve(inputArguments.size() + 1);
                QByteArray p = path.toUtf8();
                argv.push_back(p.data());

                QVector<QByteArray> args(inputArguments.count());

                for (auto iter = inputArguments.begin(); iter != inputArguments.end(); ++iter)
                {
                    args.push_back(iter->toLatin1());
                    argv.push_back(args.last().data());
                }

                PySys_SetArgv(argv.size(), const_cast<char**>(&argv[0]));
                PyRun_SimpleFile(PyFile_AsFile(pyFileObject), path.toUtf8().data());
                PyErr_Print();
            }
            Py_DECREF(pyFileObject);

            if (releasePythonLock)
            {
                PyScript::ReleasePythonLock();
            }
        }
    }

    void PyRunFile(const char* pFile)
    {
        PyRunFileWithParameters(pFile, nullptr);
    }

    //////////////////////////////////////////////////////////////////////////
    void PyExecuteCommand(const char* cmdline)
    {
        GetIEditor()->GetCommandManager()->Execute(cmdline);
    }

    //////////////////////////////////////////////////////////////////////////
    void PyLog(const char* pMessage)
    {
        if (strcmp(pMessage, "") != 0)
        {
            CryLogAlways(pMessage);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    bool PyMessageBox(const char* pMessage)
    {
        return QMessageBox::information(QApplication::activeWindow(), QString(), pMessage, QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Ok;
    }

    bool PyMessageBoxYesNo(const char* pMessage)
    {
        return QMessageBox::question(QApplication::activeWindow(), QString(), pMessage) == QMessageBox::Yes;
    }

    bool PyMessageBoxOK(const char* pMessage)
    {
        return QMessageBox::information(QApplication::activeWindow(), QString(), pMessage) == QMessageBox::Ok;
    }


    QString PyEditBox(const char* pTitle)
    {
        StringDlg stringDialog(pTitle);
        if (stringDialog.exec() == QDialog::Accepted)
        {
            return stringDialog.GetString();
        }
        return "";
    }

    SPyWrappedProperty PyEditBoxAndCheckSPyWrappedProperty(const char* pTitle)
    {
        StringDlg stringDialog(pTitle);
        SPyWrappedProperty value;
        stringDialog.SetString(QStringLiteral(""));
        bool isValidDataType(false);

        while (!isValidDataType && stringDialog.exec() == QDialog::Accepted)
        {
            const QString stringValue = stringDialog.GetString();

            // detect data type
            QString tempString = stringValue;
            int countComa = 0;
            int countOpenRoundBraket = 0;
            int countCloseRoundBraket = 0;
            int countDots = 0;

            int posDots = 0;
            int posComa = 0;
            int posOpenRoundBraket = 0;
            int posCloseRoundBraket = 0;

            for (int i = 0; i < 3; i++)
            {
                if (tempString.indexOf(".", posDots) > -1)
                {
                    posDots = tempString.indexOf(".", posDots) + 1;
                    countDots++;
                }
                if (tempString.indexOf(",", posComa) > -1)
                {
                    posComa = tempString.indexOf(",", posComa) + 1;
                    countComa++;
                }
                if (tempString.indexOf("(", posOpenRoundBraket) > -1)
                {
                    posOpenRoundBraket = tempString.indexOf("(", posOpenRoundBraket) + 1;
                    countOpenRoundBraket++;
                }
                if (tempString.indexOf(")", posCloseRoundBraket) > -1)
                {
                    posCloseRoundBraket = tempString.indexOf(")", posCloseRoundBraket) + 1;
                    countCloseRoundBraket++;
                }
            }

            if (countDots == 3 && countComa == 2 && countOpenRoundBraket == 1 && countCloseRoundBraket == 1)
            {
                value.type = SPyWrappedProperty::eType_Vec3;
            }
            else if (countDots == 0 && countComa == 2 && countOpenRoundBraket == 1 && countCloseRoundBraket == 1)
            {
                value.type = SPyWrappedProperty::eType_Color;
            }
            else if (countDots == 1 && countComa == 0 && countOpenRoundBraket == 0 && countCloseRoundBraket == 0)
            {
                value.type = SPyWrappedProperty::eType_Float;
            }
            else if (countDots == 0 && countComa == 0 && countOpenRoundBraket == 0 && countCloseRoundBraket == 0)
            {
                if (stringValue == "False" || stringValue == "True")
                {
                    value.type = SPyWrappedProperty::eType_Bool;
                }
                else
                {
                    bool isString(false);

                    if (stringValue.isEmpty())
                    {
                        isString = true;
                    }

                    char tempString[255];
                    azstrcpy(tempString, AZ_ARRAY_SIZE(tempString), stringValue.toUtf8().data());

                    for (int i = 0; i < stringValue.length(); i++)
                    {
                        if (!isdigit(tempString[i]))
                        {
                            isString = true;
                        }
                    }

                    if (isString)
                    {
                        value.type = SPyWrappedProperty::eType_String;
                    }
                    else
                    {
                        value.type = SPyWrappedProperty::eType_Int;
                    }
                }
            }

            // initialize value
            if (value.type == SPyWrappedProperty::eType_Vec3)
            {
                QString valueRed = stringValue;
                int iStart = valueRed.indexOf("(");
                valueRed.remove(0, iStart + 1);
                int iEnd = valueRed.indexOf(",");
                valueRed.remove(iEnd, valueRed.length());
                float fValueRed = valueRed.toDouble();

                QString valueGreen = stringValue;
                iStart = valueGreen.indexOf(",");
                valueGreen.remove(0, iStart + 1);
                iEnd = valueGreen.indexOf(",");
                valueGreen.remove(iEnd, valueGreen.length());
                float fValueGreen = valueGreen.toDouble();

                QString valueBlue = stringValue;
                valueBlue.remove(0, valueBlue.indexOf(",") + 1);
                valueBlue.remove(0, valueBlue.indexOf(",") + 1);
                valueBlue.remove(valueBlue.indexOf(")"), valueBlue.length());
                float fValueBlue = valueBlue.toDouble();

                value.property.vecValue.x = fValueRed;
                value.property.vecValue.y = fValueGreen;
                value.property.vecValue.z = fValueBlue;
                isValidDataType = true;
            }
            else if (value.type == SPyWrappedProperty::eType_Color)
            {
                QString valueRed = stringValue;
                int iStart = valueRed.indexOf("(");
                valueRed.remove(0, iStart + 1);
                int iEnd = valueRed.indexOf(",");
                valueRed.remove(iEnd, valueRed.length());
                int iValueRed = valueRed.toInt();

                QString valueGreen = stringValue;
                iStart = valueGreen.indexOf(",");
                valueGreen.remove(0, iStart + 1);
                iEnd = valueGreen.indexOf(",");
                valueGreen.remove(iEnd, valueGreen.length());
                int iValueGreen = valueGreen.toInt();

                QString valueBlue = stringValue;
                valueBlue.remove(0, valueBlue.indexOf(",") + 1);
                valueBlue.remove(0, valueBlue.indexOf(",") + 1);
                valueBlue.remove(valueBlue.indexOf(")"), valueBlue.length());
                int iValueBlue = valueBlue.toInt();

                value.property.colorValue.r = iValueRed;
                value.property.colorValue.g = iValueGreen;
                value.property.colorValue.b = iValueBlue;
                isValidDataType = true;
            }
            else if (value.type == SPyWrappedProperty::eType_Int)
            {
                value.property.intValue = stringValue.toInt();
                isValidDataType = true;
            }
            else if (value.type == SPyWrappedProperty::eType_Float)
            {
                value.property.floatValue = stringValue.toDouble();
                isValidDataType = true;
            }
            else if (value.type == SPyWrappedProperty::eType_String)
            {
                value.stringValue = stringValue;
                isValidDataType = true;
            }
            else if (value.type == SPyWrappedProperty::eType_Bool)
            {
                if (stringValue == "True" || stringValue == "False")
                {
                    value.property.boolValue = stringValue == "True";
                    isValidDataType = true;
                }
            }
            else
            {
                QMessageBox::critical(AzToolsFramework::GetActiveWindow(), QObject::tr("Invalid Data"), QObject::tr("Invalid data type."));
                isValidDataType = false;
            }
        }
        return value;
    }

    QString PyOpenFileBox()
    {
        QString path = QFileDialog::getOpenFileName();
        if (!path.isEmpty())
        {
            Path::ConvertBackSlashToSlash(path);
        }
        return path;
    }

    QString PyComboBox(QString title, QStringList values, int selectedIdx = 0)
    {
        QString result;

        if (title.isEmpty())
        {
            throw std::runtime_error("Incorrect title argument passed in. ");
            return result;
        }

        if (values.size() == 0)
        {
            throw std::runtime_error("Empty value list passed in. ");
            return result;
        }

        CGenericSelectItemDialog pyDlg;
        pyDlg.setWindowTitle(title);
        pyDlg.SetMode(CGenericSelectItemDialog::eMODE_LIST);
        pyDlg.SetItems(values);
        pyDlg.PreSelectItem(values[selectedIdx]);

        if (pyDlg.exec() == QDialog::Accepted)
        {
            result = pyDlg.GetSelectedItem();
        }

        return result;
    }

    static void PyDrawLabel(int x, int y, float size, float r, float g, float b, float a, const char* pLabel)
    {
        if (!pLabel)
        {
            throw std::logic_error("No label given.");
            return;
        }

        if (!r || !g || !b || !a)
        {
            throw std::logic_error("Invalid color parameters given.");
            return;
        }

        if (!x || !y || !size)
        {
            throw std::logic_error("Invalid position or size parameters given.");
        }
        else
        {
            float color[] = {r, g, b, a};
            gEnv->pRenderer->Draw2dLabel(x, y, size, color, false, pLabel);
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Constrain
    //////////////////////////////////////////////////////////////////////////
    const char* PyGetAxisConstraint()
    {
        AxisConstrains actualConstrain = GetIEditor()->GetAxisConstrains();
        switch (actualConstrain)
        {
        case AXIS_X:
            return "X";
        case AXIS_Y:
            return "Y";
        case AXIS_Z:
            return "Z";
        case AXIS_XY:
            return "XY";
        case AXIS_XZ:
            return "XZ";
        case AXIS_YZ:
            return "YZ";
        case AXIS_XYZ:
            return "XYZ";
        case AXIS_TERRAIN:
            return (GetIEditor()->IsTerrainAxisIgnoreObjects()) ? "TERRAIN" : "TERRAINSNAP";
        default:
            throw std::logic_error("Invalid axes.");
        }
    }

    void PySetAxisConstraint(QString pConstrain)
    {
        if (pConstrain == "X")
        {
            GetIEditor()->SetAxisConstraints(AXIS_X);
        }
        else if (pConstrain == "Y")
        {
            GetIEditor()->SetAxisConstraints(AXIS_Y);
        }
        else if (pConstrain == "Z")
        {
            GetIEditor()->SetAxisConstraints(AXIS_Z);
        }
        else if (pConstrain == "XY")
        {
            GetIEditor()->SetAxisConstraints(AXIS_XY);
        }
        else if (pConstrain == "YZ")
        {
            GetIEditor()->SetAxisConstraints(AXIS_YZ);
        }
        else if (pConstrain == "XZ")
        {
            GetIEditor()->SetAxisConstraints(AXIS_XZ);
        }
        else if (pConstrain == "XYZ")
        {
            GetIEditor()->SetAxisConstraints(AXIS_XYZ);
        }
        else if (pConstrain == "TERRAIN")
        {
            GetIEditor()->SetAxisConstraints(AXIS_TERRAIN);
            GetIEditor()->SetTerrainAxisIgnoreObjects(true);
        }
        else if (pConstrain == "TERRAINSNAP")
        {
            GetIEditor()->SetAxisConstraints(AXIS_TERRAIN);
            GetIEditor()->SetTerrainAxisIgnoreObjects(false);
        }
        else
        {
            throw std::logic_error("Invalid axes.");
        }
    }
    //////////////////////////////////////////////////////////////////////////
    // Edit Mode
    //////////////////////////////////////////////////////////////////////////
    const char* PyGetEditMode()
    {
        int actualEditMode = GetIEditor()->GetEditMode();
        switch (actualEditMode)
        {
        case eEditModeSelect:
            return "SELECT";
        case eEditModeSelectArea:
            return "SELECTAREA";
        case eEditModeMove:
            return "MOVE";
        case eEditModeRotate:
            return "ROTATE";
        case eEditModeScale:
            return "SCALE";
        case eEditModeTool:
            return "TOOL";
        default:
            throw std::logic_error("Invalid edit mode.");
        }
    }

    void PySetEditMode(QString pEditMode)
    {
        if (pEditMode == "MOVE")
        {
            GetIEditor()->SetEditMode(eEditModeMove);
        }
        else if (pEditMode == "ROTATE")
        {
            GetIEditor()->SetEditMode(eEditModeRotate);
        }
        else if (pEditMode == "SCALE")
        {
            GetIEditor()->SetEditMode(eEditModeScale);
        }
        else if (pEditMode == "SELECT")
        {
            GetIEditor()->SetEditMode(eEditModeSelect);
        }
        else if (pEditMode == "SELECTAREA")
        {
            GetIEditor()->SetEditMode(eEditModeSelectArea);
        }
        else if (pEditMode == "TOOL")
        {
            GetIEditor()->SetEditMode(eEditModeTool);
        }
        else if (pEditMode == "RULER")
        {
            CRuler* pRuler = GetIEditor()->GetRuler();
            pRuler->SetActive(!pRuler->IsActive());
        }
        else
        {
            throw std::logic_error("Invalid edit mode.");
        }
    }

    //////////////////////////////////////////////////////////////////////////
    const char* PyGetPakFromFile(const char* filename)
    {
        ICryPak* pIPak = GetIEditor()->GetSystem()->GetIPak();
        AZ::IO::HandleType fileHandle = pIPak->FOpen(filename, "rb");
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            throw std::logic_error("Invalid file name.");
        }
        const char* pArchPath = pIPak->GetFileArchivePath(fileHandle);
        pIPak->FClose(fileHandle);
        return pArchPath;
    }

    //////////////////////////////////////////////////////////////////////////
    void PyUndo()
    {
        GetIEditor()->Undo();
    }

    //////////////////////////////////////////////////////////////////////////
    void PyRedo()
    {
        GetIEditor()->Redo();
    }

    //////////////////////////////////////////////////////////////////////////
    // HideMask controls
    //////////////////////////////////////////////////////////////////////////
    void PySetHideMaskAll()
    {
        GetIEditor()->GetDisplaySettings()->SetObjectHideMask(OBJTYPE_ANY);
    }

    //////////////////////////////////////////////////////////////////////////
    void PySetHideMaskNone()
    {
        GetIEditor()->GetDisplaySettings()->SetObjectHideMask(0);
    }

    //////////////////////////////////////////////////////////////////////////
    void PySetHideMaskInvert()
    {
        uint32 hideMask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
        GetIEditor()->GetDisplaySettings()->SetObjectHideMask(~hideMask);
    }

    void PySetHideMask(const char* pName, bool bAdd)
    {
        uint32 hideMask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();
        int hideType = 0;

        if          (!azstricmp(pName, "aipoints"))
        {
            hideType = OBJTYPE_AIPOINT;
        }
        else if (!azstricmp(pName, "brushes"))
        {
            hideType = OBJTYPE_BRUSH;
        }
        else if (!azstricmp(pName, "decals"))
        {
            hideType = OBJTYPE_DECAL;
        }
        else if (!azstricmp(pName, "entities"))
        {
            hideType = OBJTYPE_ENTITY;
        }
        else if (!azstricmp(pName, "groups"))
        {
            hideType = OBJTYPE_GROUP;
        }
        else if (!azstricmp(pName, "prefabs"))
        {
            hideType = OBJTYPE_PREFAB;
        }
        else if (!azstricmp(pName, "other"))
        {
            hideType = OBJTYPE_OTHER;
        }
        else if (!azstricmp(pName, "shapes"))
        {
            hideType = OBJTYPE_SHAPE;
        }
        else if (!azstricmp(pName, "solids"))
        {
            hideType = OBJTYPE_SOLID;
        }
        else if (!azstricmp(pName, "tagpoints"))
        {
            hideType = OBJTYPE_TAGPOINT;
        }
        else if (!azstricmp(pName, "volumes"))
        {
            hideType = OBJTYPE_VOLUME;
        }
        else if (!azstricmp(pName, "geomcaches"))
        {
            hideType = OBJTYPE_GEOMCACHE;
        }
        else if (!azstricmp(pName, "roads"))
        {
            hideType = OBJTYPE_ROAD;
        }
        else if (!azstricmp(pName, "rivers"))
        {
            hideType = OBJTYPE_ROAD;
        }

        if (bAdd)
        {
            hideMask |= hideType;
        }
        else
        {
            hideMask &= ~hideType;
        }

        GetIEditor()->GetDisplaySettings()->SetObjectHideMask(hideMask);
    }

    bool PyGetHideMask(const char* pName)
    {
        uint32 hideMask = GetIEditor()->GetDisplaySettings()->GetObjectHideMask();

        if ((!azstricmp(pName, "aipoints")        && (hideMask & OBJTYPE_AIPOINT))  ||
            (!azstricmp(pName, "brushes")         && (hideMask & OBJTYPE_BRUSH))        ||
            (!azstricmp(pName, "decals")          && (hideMask & OBJTYPE_DECAL))        ||
            (!azstricmp(pName, "entities")        && (hideMask & OBJTYPE_ENTITY))       ||
            (!azstricmp(pName, "groups")          && (hideMask & OBJTYPE_GROUP))        ||
            (!azstricmp(pName, "prefabs")         && (hideMask & OBJTYPE_PREFAB))       ||
            (!azstricmp(pName, "other")               && (hideMask & OBJTYPE_OTHER))        ||
            (!azstricmp(pName, "shapes")          && (hideMask & OBJTYPE_SHAPE))        ||
            (!azstricmp(pName, "solids")          && (hideMask & OBJTYPE_SOLID))        ||
            (!azstricmp(pName, "tagpoints")       && (hideMask & OBJTYPE_TAGPOINT)) ||
            (!azstricmp(pName, "volumes")         && (hideMask & OBJTYPE_VOLUME))       ||
            (!azstricmp(pName, "geomcaches")  && (hideMask & OBJTYPE_GEOMCACHE)) ||
            (!azstricmp(pName, "roads")               && (hideMask & OBJTYPE_ROAD))         ||
            (!azstricmp(pName, "rivers")          && (hideMask & OBJTYPE_ROAD)))
        {
            return true;
        }

        return false;
    }
}

REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetCVar, general, get_cvar,
    "Gets a cvar value as a string.",
    "general.get_cvar(str cvarName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetCVar, general, set_cvar,
    "Sets a cvar value from an integer, float or string.",
    "general.set_cvar(str cvarName, [int, float, string] cvarValue)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyEnterGameMode, general, enter_game_mode,
    "Enters the editor game mode.",
    "general.enter_game_mode()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyExitGameMode, general, exit_game_mode,
    "Exits the editor game mode.",
    "general.exit_game_mode()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyIsInGameMode, general, is_in_game_mode,
    "Queries if it's in the game mode or not.",
    "general.is_in_game_mode()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyNewObject, general, new_object,
    "Creates a new object with given arguments and returns the name of the object.",
    "general.new_object(str entityTypeName, str cgfName, str entityName, float xValue, float yValue, float zValue)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyNewObjectAtCursor, general, new_object_at_cursor,
    "Creates a new object at a position targeted by cursor and returns the name of the object.",
    "general.new_object_at_cursor(str entityTypeName, str cgfName, str entityName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyStartObjectCreation, general, start_object_creation,
    "Creates a new object, which is following the cursor.",
    "general.start_object_creation(str entityTypeName, str cgfName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRunConsole, general, run_console,
    "Runs a console command.",
    "general.run_console(str consoleCommand)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRunLua, general, run_lua,
    "Runs a lua script command.",
    "general.run_lua(str luaName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRunFile, general, run_file,
    "Runs a script file. A relative path from the editor user folder or an absolute path should be given as an argument",
    "general.run_file(str fileName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRunFileWithParameters, general, run_file_parameters,
    "Runs a script file with parameters. A relative path from the editor user folder or an absolute path should be given as an argument. The arguments should be separated by whitespace.",
    "general.run_file_parameters(str fileName, str arguments)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyExecuteCommand, general, execute_command,
    "Executes a given string as an editor command.",
    "general.execute_command(str editorCommand)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyMessageBox, general, message_box,
    "Shows a confirmation message box with ok|cancel and shows a custom message.",
    "general.message_box(str message)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyMessageBoxYesNo, general, message_box_yes_no,
    "Shows a confirmation message box with yes|no and shows a custom message.",
    "general.message_box_yes_no(str message)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyMessageBoxOK, general, message_box_ok,
    "Shows a confirmation message box with only ok and shows a custom message.",
    "general.message_box_ok(str message)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyEditBox, general, edit_box,
    "Shows an edit box and returns the value as string.",
    "general.edit_box(str title)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyEditBoxAndCheckSPyWrappedProperty, general, edit_box_check_data_type,
    "Shows an edit box and checks the custom value to use the return value with other functions correctly.",
    "general.edit_box_check_data_type(str title)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyOpenFileBox, general, open_file_box,
    "Shows an open file box and returns the selected file path and name.",
    "general.open_file_box()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetAxisConstraint, general, get_axis_constraint,
    "Gets axis.",
    "general.get_axis_constraint()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetAxisConstraint, general, set_axis_constraint,
    "Sets axis.",
    "general.set_axis_constraint(str axisName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetEditMode, general, get_edit_mode,
    "Gets edit mode",
    "general.get_edit_mode()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PySetEditMode, general, set_edit_mode,
    "Sets edit mode",
    "general.set_edit_mode(str editModeName)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyGetPakFromFile, general, get_pak_from_file,
    "Finds a pak file name for a given file",
    "general.get_pak_from_file(str fileName)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyLog, general, log,
    "Prints the message to the editor console window.",
    "general.log(str message)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyUndo, general, undo,
    "Undos the last operation",
    "general.undo()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyRedo, general, redo,
    "Redoes the last undone operation",
    "general.redo()");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyDrawLabel, general, draw_label,
    "Shows a 2d label on the screen at the given position and given color.",
    "general.draw_label(int x, int y, float r, float g, float b, float a, str label)");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyCreateObject, general, create_object,
    "Creates a new object with given arguments and returns the name of the object.",
    "general.create_object(str objectClass, str objectFile, str objectName, (float, float, float) position");
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(PyComboBox, general, combo_box,
    "Shows an combo box listing each value passed in, returns string value selected by the user.",
    "general.combo_box(str title, [str] values, int selectedIndex)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetHideMaskAll, general, set_hidemask_all,
    "Sets the current hidemask to 'all'",
    "general.set_hidemask_all()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetHideMaskNone, general, set_hidemask_none,
    "Sets the current hidemask to 'none'",
    "general.set_hidemask_none()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetHideMaskInvert, general, set_hidemask_invert,
    "Inverts the current hidemask",
    "general.set_hidemask_invert()");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PySetHideMask, general, set_hidemask,
    "Assigns a specified value to a specific object type in the current hidemask",
    "general.set_hidemask(str typeName, bool typeValue)");
REGISTER_PYTHON_COMMAND_WITH_EXAMPLE(PyGetHideMask, general, get_hidemask,
    "Gets the value of a specific object type in the current hidemask",
    "general.get_hidemask(str typeName)");
