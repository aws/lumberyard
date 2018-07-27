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

#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QString>
#include <QDebug>
#include <AzCore/std/functional.h>

namespace Lyzard
{
    /** Interface class, allows us to create helper functions to read objects */
    class IJsonReadable
    {
    public:
        virtual ~IJsonReadable() { }
        virtual bool ReadFromJSON(QJsonObject source) = 0;
    };

    namespace JsonUtilities
    {
        // read an array of JSON objects from the given object, returning them via callback.
        template <typename ReadType>
        bool ReadJSONObjects(
            QString key,
            QJsonObject source,
            QObject* parent,
            AZStd::function<bool(ReadType*)> callback
        )
        {
            static_assert(std::is_base_of<IJsonReadable, ReadType>::value, "ReadType must be derived from IJsonObject");

            if (key.isEmpty())
            {
                qDebug() << "Key is empty while reading from JSON dictionary";
                return false;
            }

            QJsonValue resultValue = source[key];
            if (!resultValue.isArray())
            {
                qDebug() << "Key " << key << " is not an array.";
                return false;
            }

            QJsonArray arrayValue = resultValue.toArray();
            for (auto element : arrayValue)
            {
                if (!element.isObject())
                {
                    qDebug() << "Key " << key << "element in array is not an object.";
                    return false;
                }

                ReadType* newObject = new ReadType(parent);
                if (!newObject->ReadFromJSON(element.toObject()))
                {
                    qDebug() << "Key " << key << " key not readable from JSON.";
                    return false;
                }
                if (!callback(newObject))
                {
                    qDebug() << "Key " << key << " object read terminated by function";
                    return false;
                }
            }

            return true;
        }

        // Read a single object from a given JSON object root
        template <typename ReadType>
        ReadType* ReadJSONObject(QString key, QJsonObject source, QObject* parent)
        {
            static_assert(std::is_base_of<IJsonReadable, ReadType>::value, "ReadType must be derived from IJsonObject");
            if (key.isEmpty())
            {
                qDebug() << "ReadJSONObject - Key is empty while reading from JSON dictionary";
                return nullptr;
            }

            QJsonValue resultValue = source[key];

            if (resultValue.isNull())
            {
                return nullptr; // its okay for it to be missing entirely
            }
            if (!resultValue.isObject())
            {
                qDebug() << "ReadJSONObject - Key " << key << " is not an object.";
                return nullptr;
            }

            ReadType* newObject = new ReadType(parent);
            if (!newObject->ReadFromJSON(resultValue.toObject()))
            {
                qDebug() << "Key " << key << " key not readable from JSON.";
                delete newObject;
                return nullptr;
            }

            return newObject;
        }

        QJsonObject ReadDocument(QString fileName);
        QString ReadFromJSON(QString key, QJsonObject source, QString defaultValue, bool optional = false);

        /// the bool version reads a bool from json, expecting it as an integer: 0 or (not zero)
        bool ReadFromJSON(QString key, QJsonObject source, bool defaultValue);

        QStringList ReadFromJSON(QString key, QJsonObject source);
    }
}
