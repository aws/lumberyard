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
#include "AttributeItemLogicCallbacks.h"
#include "AttributeItem.h"
#include "AttributeView.h"
#include <QRegExp>
#include <QStringList>
#include <QDebug>

#include "VariableWidgets/QCopyableWidget.h"

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
#include "VariableWidgets/QCurveEditorImp.h"
/*
Note that not all variable widgets are implemented.
Currently only:
-QWidgetVector (for 1 float)
-QBoolWidget
-QEnumWidget
are tested
*/

#include <Util/Variable.h>
#include "Utils.h"

//! Epsilon for vector comparasion.
#define FLOAT_EPSILON 0.000001f

bool AttributeItemLogicCallbacks::sCallbacksEnabled = false;
AttributeItemLogicCallbacks::PersistentCallbackData AttributeItemLogicCallbacks::sPersistentCallbackData;

QMap<QString, AttributeItemLogicCallbacks::InnerCallbackType> AttributeItemLogicCallbacks::InitializeCallbackMap()
{
    QMap<QString, InnerCallbackType> map;

    //Holds the current function name specified by AddFunction(name)
    const char* thisFunctionName = nullptr;

#define AddFunction(name)     \
    thisFunctionName = #name; \
    map[#name] = [thisFunctionName](CAttributeItem * caller, QVector<QString> const & args)

#define StopIfDisabled() if (!AttributeItemLogicCallbacks::sCallbacksEnabled) return false

    //Returns true if the argument count is equal to n, prints an error and returns false if not
#define CheckArgCount(n) \
    (args.count() != (n)) ? (qWarning() << "Error:" << thisFunctionName << "called by" << caller->objectName() << "with" << args.count() << "arguments while excpecting" << n << "arguments", false) : true

    //Returns true if the argument count is greater or equal to n, prints an error and returns false if not
#define CheckMinArgCount(n) \
    (args.count()<(n)) ? (qWarning() << "Error:" << thisFunctionName << "called by" << caller->objectName() << "with" << args.count() << "arguments while excpecting" << n << "arguments", false) : true

    //Dumps the caller object name, the function name and all arguments
#define DumpFunctionInfo()                                                                                                         \
    do {qDebug() << "CAttributeItem*" << caller->objectName() << "called" << thisFunctionName << "with" << args.size() << "args:"; \
        for (int i = 0; i<args.count(); i++)                                                                                       \
                          qDebug() << i << args[i];                                                                                \
                          } while (false)

    //You can use the following arguments in the functions:
    // CAttributeItem* caller contains the pointer to the caller.
    // QVector<QString> const& args is a list of arguments parsed from the configuration xml.
    //
    //Some convinient functions are available:
    // GetAttributeItem can be used to convert a string path to a CAttributeItem*
    // GetAttributeControl can be used to get a specific control from the string path or CAttributeItem*
    //Note that when these functions are used you need to handle any nullptr correctly as not all objects
    //  have to be created when these callback are called.

    AddFunction(testFunction)
    {
        StopIfDisabled();
        DumpFunctionInfo();
        return false;
    };

    AddFunction(resolveVisibility)
    {
        Q_UNUSED(thisFunctionName)
        CAttributeView* attributeView = caller->getAttributeView();
        attributeView->ResolveVisibility();

        return false;
    };

    AddFunction(ResetPersistentState)
    {
        Q_UNUSED(thisFunctionName)
        //Resets the sPersistentCallbackData to the initial state
        //This is used when a new particle is loaded
        sPersistentCallbackData.m_aspectRatios.clear();
        return false;
    };

    AddFunction(aspectFixRatio)
    {
        StopIfDisabled();
        //keep the particle size sync if Keep Aspect Ratio is checked
        //the curve sync is done in SyncParticleSizeCurves() function
        if (CheckMinArgCount(3))
        {
            QBoolWidget* fixAspectCheckbox = GetAttributeControl<QBoolWidget*>(args[0], caller);
            QWidgetVector* sizeX = GetAttributeControl<QWidgetVector*>(args[1], caller);
            QWidgetVector* sizeY = GetAttributeControl<QWidgetVector*>(args[2], caller);
            CAttributeItem* fixAspectItem = GetAttributeItem(args[0], caller);
            CAttributeItem* sizeXItem = GetAttributeItem(args[1], caller);
            CAttributeItem* sizeYItem = GetAttributeItem(args[2], caller);


            //Have all valid pointers
            if (fixAspectCheckbox && sizeX && sizeY && fixAspectItem && sizeXItem && sizeYItem)
            {
                QString fixAspectVarPath = fixAspectItem->getAttributePath(true);
                float sizeXValue;
                float sizeYValue;

                IVariable* sizeYVariable = sizeY->getVar();
                IVariable* sizeXVariable = sizeX->getVar();
                CRY_ASSERT(sizeYVariable);
                CRY_ASSERT(sizeXVariable);
                sizeXVariable->Get(sizeXValue);
                sizeYVariable->Get(sizeYValue);

                float aspectRatio = -1.0f; // don't divide by zero, store negative one so we can check when we validate size changes when aspect ratio is maintained
                if (sPersistentCallbackData.m_aspectRatios.contains(fixAspectVarPath))
                {
                    aspectRatio = sPersistentCallbackData.m_aspectRatios[fixAspectVarPath];
                }

                if (fixAspectCheckbox->isChecked())
                {
                    //If invalid ratio lets try to recalc it ... This can trigger when doing the first run "sPersistentCallbackData.m_aspectRatios.contains(fixAspectVarPath)" is false or
                    //  the "Maintain Aspect Ratio" is unchecked, items are manipulated, then it is checked again with size Y equal to 0.0f.
                    //  this results in an aspect ratio of -1. when "Maintain Aspect Ratio" is turned back on and the size y value is manipulated to be valid then we need to recalc the aspect ratio.
                    if (aspectRatio < FLOAT_EPSILON)
                    {
                        if (abs(sizeYValue) > FLOAT_EPSILON)
                        {
                            aspectRatio = sizeXValue / sizeYValue;
                        }
                        sPersistentCallbackData.m_aspectRatios[fixAspectVarPath] = aspectRatio;
                    }

                    //need to know which slider was adjusted
                    if (caller == sizeXItem)
                    {
                        if (aspectRatio > FLOAT_EPSILON)
                        {
                            sizeYValue = sizeXValue / aspectRatio;
                            float val;
                            sizeYVariable->Get(val);
                            if (val != sizeYValue)
                            {
                                sizeYVariable->Set(sizeYValue);
                            }
                        }
                    }
                    else if (caller == sizeYItem)
                    {
                        if (aspectRatio > FLOAT_EPSILON)
                        {
                            sizeXValue = sizeYValue * aspectRatio;
                            float val;
                            sizeXVariable->Get(val);
                            if (val != sizeXValue)
                            {
                                sizeXVariable->Set(sizeXValue);
                            }
                        }
                    }

                    if (args.size() >= 5)
                    {
                        QVector<QString> callArgs;
                        callArgs.push_back(args[0]);
                        callArgs.push_back(args[3]);
                        callArgs.push_back(args[4]);
                        AttributeItemLogicCallback func;
                        if (AttributeItemLogicCallbacks::GetCallback("keepFloatEqual", &func))
                        {
                            func(caller, callArgs);
                        }
                    }
                }
                else
                {
                    //Update the aspect ratio becuase we are changing X or Y without "Maintain Aspect Ratio" turned on
                    if (abs(sizeYValue) > FLOAT_EPSILON)
                    {
                        sPersistentCallbackData.m_aspectRatios[fixAspectVarPath] = sizeXValue / sizeYValue;
                    }
                    else
                    {
                        sPersistentCallbackData.m_aspectRatios[fixAspectVarPath] = -1;
                    }
                }
            }
        }
        return false;
    };

    AddFunction(keepFloatEqual)
    {
        StopIfDisabled();
        if (CheckArgCount(3))
        {
            QBoolWidget* keepEqualCheckbox = GetAttributeControl<QBoolWidget*>(args[0], caller);
            if (keepEqualCheckbox && keepEqualCheckbox->isChecked())
            {
                QWidgetVector* valueA = GetAttributeControl<QWidgetVector*>(args[1], caller);
                QWidgetVector* valueB = GetAttributeControl<QWidgetVector*>(args[2], caller);
                IVariable* valueAVariable = valueA->getVar();
                IVariable* valueBVariable = valueB->getVar();

                if (valueA && valueAVariable && valueB && valueBVariable)
                {
                    float valueAValue = valueA->getGuiValue(0);
                    float valueBValue = valueB->getGuiValue(0);
                    float final = valueAValue;

                    CAttributeItem* valueBItem = GetAttributeItem(args[2], caller);
                    if (caller == valueBItem)
                    {
                        final = valueBValue;
                    }
                    if (final != valueAValue)
                    {
                        valueAVariable->Set(final);
                    }
                    if (final != valueBValue)
                    {
                        valueBVariable->Set(final);
                    }
                }
            }
        }
        return false;
    };

#undef AddFunction

    return map;
}

QMap<QString, AttributeItemLogicCallbacks::InnerCallbackType> CallbackMap = AttributeItemLogicCallbacks::InitializeCallbackMap();

bool AttributeItemLogicCallbacks::GetCallback(QString function, AttributeItemLogicCallback* outCallback)
{
    function = function.trimmed();
    if (function.size() == 0)
    {
        return false;
    }

    std::string sfunction = function.toStdString();

    //Split the function string in to the function name, and the function arguments
    int openParPosition = function.indexOf((QChar)'(');

    QString functionName = function.left(openParPosition).trimmed();
    auto callbackIt = CallbackMap.find(functionName);
    if (callbackIt == CallbackMap.end())
    {
        return false;
    }

    std::string sfunctionName = functionName.toStdString();

    QString functionArgs = function.mid(openParPosition);

    std::string sfunctionArgs = functionArgs.toStdString();

    if (functionName.size() == 0)
    {
        return false;
    }

    QVector<QString> argumentList;
    if (*functionArgs.begin() == '(' && *(functionArgs.end() - 1) == ')')
    {
        functionArgs = functionArgs.mid(1, functionArgs.size() - 2); //Remove parentheses

        std::string sfunctionArgs = functionArgs.toStdString();

        QStringList arguments = functionArgs.split(QRegExp(",", Qt::CaseSensitive, QRegExp::FixedString));

        for (int i = 0; i < arguments.size(); i++)
        {
            std::string arg = arguments.at(i).trimmed().toStdString();
            argumentList.push_back(arguments.at(i).trimmed());
        }
    }

    InnerCallbackType callback = *callbackIt;

    *outCallback = AttributeItemLogicCallback(callback, argumentList);
    return true;
}

template <typename T>
T AttributeItemLogicCallbacks::GetAttributeControl(CAttributeItem* item)
{
    //QCopyableWidget* columnWidget = item->findChild<QCopyableWidget*>("",Qt::FindDirectChildrenOnly);
    //T control = columnWidget->findChild<T>("",Qt::FindDirectChildrenOnly);
    T control = item->findChild<T>("", Qt::FindChildrenRecursively);
    return control;
}

template<typename T>
T AttributeItemLogicCallbacks::GetAttributeControl(QString const& relativePath, QWidget* origin)
{
    CAttributeItem* item = GetAttributeItem(relativePath, origin);
    if (item == nullptr)
    {
        return nullptr;
    }
    T control = GetAttributeControl<T>(item);
    if (control == nullptr)
    {
        QWidget* w = GetAttributeControl<QWidget*>(item);
        if (w == nullptr)
        {
            qWarning() << "Path" << relativePath << "relative to" << origin->objectName() << "does not point to a control.";
        }
        else
        {
            qWarning() << "Path" << relativePath << "relative to" << origin->objectName() << "does not point to the correct control type.";
        }
    }
    return control;
}

CAttributeItem* AttributeItemLogicCallbacks::GetAttributeItem(QString relativePath, QWidget* origin)
{
    QString originalPath = relativePath; //Used to print debug info
    QWidget* currentPosition = origin->parentWidget();
    while (relativePath.startsWith("[") && currentPosition != nullptr)
    {
        relativePath = relativePath.mid(1);
        QWidget* tmpPar = currentPosition->parentWidget();
        while (tmpPar)
        {
            currentPosition = qobject_cast<CAttributeItem*>(tmpPar);
            if (currentPosition)
            {
                break;
            }
            tmpPar = tmpPar->parentWidget();
        }
    }

    QStringList pathFolders = relativePath.split(QRegExp("]", Qt::CaseSensitive, QRegExp::FixedString));

    int index = 0;
    while (currentPosition != nullptr)
    {
        if (index >= pathFolders.count())
        {
            break;
        }
        currentPosition = currentPosition->findChild<QWidget*>(pathFolders.at(index));
        index++;
    }
    CAttributeItem* item = qobject_cast<CAttributeItem*>(currentPosition);
    if (currentPosition == nullptr)
    {
        qWarning() << "Path" << originalPath << "Could not be found relative to" << origin->objectName() << ".";
    }
    else if (item == nullptr)
    {
        qWarning() << "Path" << originalPath << "relative to" << origin->objectName() << "does not point to an CAttributeItem.";
    }
    return item;
}
