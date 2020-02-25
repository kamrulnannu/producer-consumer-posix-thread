#include <bits/stdc++.h>
#include "workers_pthread.h"

using namespace std;

int main()
{
    Workers & threadPool = GetThreadPool();

    threadPool.SpawnThread();

    WorkItem * I = new Integer(33);

    WorkItem * d = new Double(11.11);

    string str = "Hello World";
    WorkItem * s = new String(str);

    threadPool.EnqueueWorkItem(*I);
    threadPool.EnqueueWorkItem(*d);
    threadPool.EnqueueWorkItem(*s);

    threadPool.Wait();

    return 0;
}
