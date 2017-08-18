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
#pragma once
#ifndef CRYINCLUDE_EDITORUI_QT_UIUTILS_H
#define CRYINCLUDE_EDITORUI_QT_UIUTILS_H

#include "api.h"

// QT
#include <QString>
#include <QColor>

class CLibraryTreeViewItem;
class CLibraryTreeView;

namespace UIUtils_detail
{
    // This template allows the conversion of property values to QString generically.
    template <typename T>
    inline QString ToString(const T& value)
    {
        // Base template will handle int and float
        return QString::number(value);
    }

    // Specialization for strings
    template <>
    inline QString ToString<QString>(const QString& value)
    {
        return value;
    }

    // This template allows the conversion of property values from QString generically.
    template <typename T>
    inline T FromString(const QString& value)
    {
        // Base template handles strings
        return value;
    }

    // Specialization for int
    template <>
    inline int FromString<int>(const QString& value)
    {
        return value.toInt();
    }

    // Specialization for float
    template <>
    inline float FromString<float>(const QString& value)
    {
        return value.toFloat();
    }
}

class EDITOR_QT_UI_API UIUtils
{
public:
    static QString GetStringDialog(const QString& title, const QString& valueName, QWidget* parent = nullptr);


    // Multi-field dialog

    enum Type
    {
        TYPE_STRING,
        TYPE_INT,
        TYPE_FLOAT,
    };

    class BaseType
    {
    public:
        BaseType(const QString& valueName)
            : m_valueName(valueName) {}
        virtual ~BaseType() = default;

        const QString&  GetValueName() const { return m_valueName; }
        virtual Type    GetType() const = 0;

        // Convert this type to some canonical string form
        virtual QString ToString() const = 0;

        // Convert this from canonical string form to a value.
        virtual void FromString(const QString& value) = 0;

    private:
        QString     m_valueName;
    };

    template <typename ValueType, Type InternalTypeEnum>
    class FieldType
        : public BaseType
    {
    public:
        FieldType(const QString& valueName, const ValueType& value)
            : BaseType(valueName)
            , m_value(value){}

        using BaseType::GetValueName;
        virtual Type        GetType() const override            { return InternalTypeEnum; }
        const ValueType&    GetValue() const                    { return m_value; }
        void                SetValue(const ValueType& value)    { m_value = value; }
        QString             ToString() const                    { return UIUtils_detail::ToString(m_value); }
        void                FromString(const QString& value)    { m_value = UIUtils_detail::FromString<ValueType>(value); }

    private:
        ValueType   m_value;
    };

    typedef FieldType<QString,  TYPE_STRING>    TypeString;
    typedef FieldType<int,      TYPE_INT>       TypeInt;
    typedef FieldType<float,    TYPE_FLOAT>     TypeFloat;

    class FieldContainer
    {
    public:
        FieldContainer(){}
        ~FieldContainer()
        {
            for (unsigned int i = 0; i < m_fieldList.size(); i++)
            {
                delete m_fieldList[i];
            }
        }

        template <class Type>
        Type* AddField(Type* field)
        {
            m_fieldList.push_back(field);
            return field;
        }

        std::vector<BaseType*> m_fieldList;
    };

    static bool GetMultiFieldDialog(QWidget* parent, const QString& title, FieldContainer& container, const unsigned int& width = 384);

    //convert a colorF string to QColor
    static QColor StringToColor(const QString& value);
    //convert a QColor to colorF string
    static QString ColorToString(const QColor& color);
    static void ColorGammaToLinear(const QColor& color, float& r, float& g, float& b, float& a);
    static QColor ColorLinearToGamma(float r, float g, float b, float a);
    static bool FileExistsOutsideIgnoreCache(const QString& filepath);
};

#endif // CRYINCLUDE_EDITORUI_QT_UIUTILS_H