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
#ifndef CRYINCLUDE_EDITORUI_QT_ATTRIBUTEITEMLOGICCALLBACKS_H
#define CRYINCLUDE_EDITORUI_QT_ATTRIBUTEITEMLOGICCALLBACKS_H
#pragma once
#include <functional>
#include <QString>
#include <QMap>
#include <QVector>

class CAttributeItem;
class QWidget;
class AttributeItemLogicCallback;
class AttributeItemLogicCallbacks
{
public:
    static bool GetCallback(QString function, AttributeItemLogicCallback* outCallback);

private:
    //This class has only static functions
    AttributeItemLogicCallbacks(){}
    ~AttributeItemLogicCallbacks(){}

    template<typename T>
    static T GetAttributeControl(CAttributeItem* item);
    template<typename T>
    static T GetAttributeControl(QString const& relativePath, QWidget* origin);
    static CAttributeItem* GetAttributeItem(QString relativePath, QWidget* origin);

public:     //These are for internal use only
    typedef std::function<bool(CAttributeItem*, QVector<QString> const&)> InnerCallbackType;
    static QMap<QString, InnerCallbackType> InitializeCallbackMap();

    static void SetCallbacksEnabled(bool enabled){sCallbacksEnabled = enabled; }
private:
    static bool sCallbacksEnabled;
    struct PersistentCallbackData
    {
        QMap<QString, float> m_aspectRatios;
    };
    static PersistentCallbackData sPersistentCallbackData;
};

class AttributeItemLogicCallback
{
    friend class AttributeItemLogicCallbacks;
public:
    AttributeItemLogicCallback()
        : m_isEmpty(true)
    {}

    bool operator()(CAttributeItem* caller)
    {
        if (!m_isEmpty)
        {
            return m_callback(caller, m_arguments);
        }
        return false;
    }
    bool operator()(CAttributeItem* caller, QVector<QString> args)
    {
        if (!m_isEmpty)
        {
            return m_callback(caller, args);
        }
        return false;
    }

private:
    AttributeItemLogicCallback(AttributeItemLogicCallbacks::InnerCallbackType callback, QVector<QString> arguments)
        : m_callback(callback)
        , m_arguments(arguments)
        , m_isEmpty(false)
    {}

    AttributeItemLogicCallbacks::InnerCallbackType m_callback;
    QVector<QString> m_arguments;
    bool m_isEmpty;
};

#endif