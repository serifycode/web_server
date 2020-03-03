//弱指针保存connection，析构时shutdown
#ifndef ENTRY_H
#define ENTRY_H


#include "TcpConnection.h"
#include "copyable.h"
#include <memory>

class Entry : public copyable
{
public:
    explicit Entry(const WeakTcpConnectionPtr& weakcon)
        :weakcon_(weakcon)
    {}

    ~Entry()
    {
        TcpConnectionPtr conn=weakcon_.lock();
        if(conn)
            conn->Shutdown();//跨线程
    }
public:
    WeakTcpConnectionPtr weakcon_;
};

typedef std::shared_ptr<Entry> EntryPtr;
typedef std::weak_ptr<Entry> WeakEntryPtr;


#endif