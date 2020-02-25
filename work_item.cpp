#include <pthread.h>
#include "work_item.h"
#include "workers_pthread.h"

using namespace std;

void Integer::process()
{
    char s[180];
    snprintf(s, sizeof(s), "Integer::process: %d, ThreadID=%x\n", m_Int, pthread_self());
    cout << s;
} 

void String::process()
{
   char s[180];
   snprintf(s, sizeof(s), "String::process: %s, ThreadID=%x\n", 
            m_Str.c_str(), pthread_self());
   cout << s;
} 

void Double::process()
{
    char s[180];
    snprintf(s, sizeof(s), "Double: Consumer Thread=%x is now a producer", 
            pthread_self());

    string str = s;
    WorkItem * w = new String(str);
    GetThreadPool().EnqueueWorkItem(*w);

    snprintf(s, sizeof(s), "Double::process: %lf, ThreadID=%x\n", 
            m_Double, pthread_self());
    cout << s;
} 
