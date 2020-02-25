#ifndef WORKERS_H_INCLUDE
#define WORKERS_H_INCLUDE

#include <bits/stdc++.h>
#include "work_item.h"

using namespace std;

void* thread_start_func_cpp (void* p);

extern "C"
{
void* thread_start_func (void* p);
}

class Workers
{
    private:
        int m_NumWorkers;
        vector<pthread_t *> m_Threads;
        bool m_Error;  // true means error creating Workers object
        pthread_mutex_t m_Mutex;
        pthread_cond_t m_WorkCondVar;
        pthread_cond_t m_WaitCondVar;
        pthread_attr_t m_Attr;
        queue<WorkItem *> m_WorkItems;
        WorkItem * DeQueueWorkItem();

        //Workers(int NumThreads);
    public:
        friend void *thread_start_func_cpp(void *p);

        typedef void * (*THREAD_FUNC_PTR)(void *);
        Workers(int NumThreads);
#if 0
        static Workers & Instance(int NumThread = 5)
        {
            static Workers workers(NumThread);

            return workers;
        }
#endif
        ~Workers();

        Workers(Workers &rhs) = delete;
        Workers & operator = (const Workers & rhs) = delete;

        int GetNumJobs() const
        {
            // Not thread safe
            return m_WorkItems.size();
        }

        void SpawnThread();

        void EnqueueWorkItem(WorkItem &item);

        void *ThreadStart(void *arg);

        void Wait();
};

extern "C"
{
Workers & GetThreadPool();
}
#endif
