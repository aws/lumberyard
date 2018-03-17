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

#include <QObject>
#include <QThread>
#include <QWidget>

#include <functional>

class AwsResultWrapper
{
public:

    AwsResultWrapper() = default;

    AwsResultWrapper(const QString& objectName, std::function<void()> applyResult)
        : m_objectName(objectName)
        , m_applyLambda(applyResult)
    {}

    AwsResultWrapper(const AwsResultWrapper& rhs) = default;
    ~AwsResultWrapper() = default;

    void Apply(const QString& currentObjectName)
    {
        if (currentObjectName == m_objectName)
        {
            m_applyLambda();
        }
    }

    const QString& GetObjectName() const { return m_objectName; }

private:

    QString m_objectName;
    std::function<void()> m_applyLambda;
};

class QBackgroundLambdaBase
    : public QObject
{
    Q_OBJECT

public:

    QBackgroundLambdaBase() {}
    virtual ~QBackgroundLambdaBase() {}

public slots:
    void Run() { Execute(); }

signals:
    void finished();
    void OnError(const QString& objectName, const QString& error);
    void OnSuccess(const QString& objectName);
    void OnSuccessApply(AwsResultWrapper result);

protected:
    virtual void Execute() = 0;
};

class QAwsObjectBackgroundLambda
    : public QBackgroundLambdaBase
{
public:
    QAwsObjectBackgroundLambda(const QString& objectName, std::function<void(QString&)> lambda)
        : m_objectName(objectName)
        , m_lambda(lambda)
    {}

    virtual ~QAwsObjectBackgroundLambda() {}

protected:

    virtual void Execute()
    {
        QString errorMessage;
        m_lambda(errorMessage);
        if (!errorMessage.isEmpty())
        {
            OnError(m_objectName, errorMessage);
        }
        else
        {
            OnSuccess(m_objectName);
        }

        finished();
    }

private:

    QString m_objectName;
    std::function<void(QString&)> m_lambda;
};

template< typename T >
class QAwsApplyResultBackgroundLambda
    : public QBackgroundLambdaBase
{
public:

    using GetLambdaType = std::function<T (QString&)>;
    using ApplyLambdaType = std::function<void(const T&)>;

    QAwsApplyResultBackgroundLambda(const QString& objectName, GetLambdaType getLambda, ApplyLambdaType applyLambda)
        : m_objectName(objectName)
        , m_getLambda(getLambda)
        , m_applyLambda(applyLambda)
    {}

    virtual ~QAwsApplyResultBackgroundLambda() {}

protected:

    virtual void Execute()
    {
        QString errorMessage;
        T result = m_getLambda(errorMessage);
        if (!errorMessage.isEmpty())
        {
            OnError(m_objectName, errorMessage);
        }
        else
        {
            auto lambdaCopy = m_applyLambda;
            auto signalLambda = [=](){lambdaCopy(result); };
            AwsResultWrapper wrapper(m_objectName, signalLambda);
            OnSuccessApply(wrapper);
        }

        finished();
    }

private:

    QString m_objectName;
    GetLambdaType m_getLambda;
    ApplyLambdaType m_applyLambda;
};

void RunBackgroundLambda(QBackgroundLambdaBase* backgroundTask);
