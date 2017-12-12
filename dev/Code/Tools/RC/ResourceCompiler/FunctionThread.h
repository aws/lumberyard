#ifndef FUNCTIONTHREAD_H
#define FUNCTIONTHREAD_H

#include <QThread>

#if !defined(AZ_PLATFORM_WINDOWS)
#define STILL_ACTIVE 259
#endif

class FunctionThread : public QThread
{
public:
    FunctionThread(unsigned int (*function)(void* param), void* param)
    : m_function(function),
    m_param(param)
    {
    }

    unsigned int returnCode() const
    {
        return m_returnCode;
    }

    static FunctionThread* CreateThread(int,int, unsigned int (*function)(void* param), void* param, int, int)
    {
        auto t = new FunctionThread(function, param);
        t->start();
        return t;
    }

protected:
    void run() override
    {
        m_returnCode = m_function(m_param);
    }

private:
    unsigned int m_returnCode = STILL_ACTIVE;
    unsigned int (*m_function)(void* param);
    void* m_param;
};

#endif
