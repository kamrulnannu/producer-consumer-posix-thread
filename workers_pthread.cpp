#include <string.h>
#include <errno.h>
#include "workers_pthread.h"

using namespace std;

static Workers pool(5);

Workers & GetThreadPool()
{
    return pool;
}

Workers::Workers(int NumThread) :
    m_NumWorkers(NumThread),
    m_Error(false)
{
    char szErrMsg[] = " ";
    //char *szErrMsg=" ";
    
    m_Threads.resize(NumThread);

    int rCode = pthread_mutex_init(&m_Mutex, NULL);
    if (rCode)
    {
        //strerror_r(rCode, szErrMsg, sizeof(szErrMsg));
        //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
        cerr << "Workers::Workers: pthread_mutex_init failed, RC="<<rCode
             << ", emsg=" << szErrMsg<<endl;
        m_Error = true;
        return;
    }

    rCode = pthread_cond_init(&m_WorkCondVar, NULL);
    if (rCode)
    {
        //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
        //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
        cerr << "Workers::Workers: pthread_cond_init failed for WorkQ, RC="<<rCode
             << ", emsg=" << szErrMsg<<endl;
        m_Error = true;
        return;
    }

    rCode = pthread_cond_init(&m_WaitCondVar, NULL);
    if (rCode)
    {
        //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
        //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
        cerr << "Workers::Workers: pthread_cond_init failed for Wait, RC="<<rCode
             << ", emsg=" << szErrMsg<<endl;
        m_Error = true;
        return;
    }

    rCode = pthread_attr_init(&m_Attr);
    if (rCode)
    {
        //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
        //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
        cerr << "Workers::Workers: pthread_attr_init failed, RC="<<rCode
             << ", emsg=" << szErrMsg<<endl;
        m_Error = true;
        return;
    }
}

void Workers::SpawnThread()
{
    // Spawn thread
    char szErrMsg [] =" ";
    for (auto i = 0; (i < m_NumWorkers); ++i)
    {
        pthread_t *id = new pthread_t;
        //int rCode = pthread_create(id, &m_Attr, (THREAD_FUNC_PTR)&Workers::ThreadStart, this);
        int rCode = pthread_create(id, &m_Attr, thread_start_func, this);

        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::SpawnThread: pthread_create failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<endl;
            exit(1);
        }
        else
        {
            m_Threads[i] = id;
        }
    }
}


Workers::~Workers()
{
    for (auto x : m_Threads)
        delete x;
}

void Workers::EnqueueWorkItem(WorkItem &item)
{
    //char szErrMsg[180];
    char *szErrMsg;
    int rCode = pthread_mutex_lock(&m_Mutex);

    if (rCode)
    {
        //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
        //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
        cerr << "Workers::EnqueueWorkItem: pthread_mutex_lock failed, RC="<<rCode
             << ", emsg=" << szErrMsg<<endl;
        exit(1);
    }

    m_WorkItems.push(&item);

    rCode = pthread_mutex_unlock(&m_Mutex);

    if (rCode)
    {
        //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
        //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
        cerr << "Workers::EnqueueWorkItem: pthread_mutex_unlock failed, RC="<<rCode
             << ", emsg=" << szErrMsg<<endl;
        exit(1);
    }

    /*
     * Condition happened: Data available in Work Q to process
     * Now wake up sleeping threads to process the item in Q.
     *
     * pthread_cond_signal: It'll unblock or wake-up at least one thread
     * or
     * pthread_cond_broadcast: It will unblock or wake-up all threads
     */

    // rCode = pthread_cond_signal(&m_WorkCondVar); // Wake-up at least one
    // thread
    rCode = pthread_cond_broadcast(&m_WorkCondVar); // wake up all threads
    if (rCode)
    {
        //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
        //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
        cerr << "Workers::EnqueueWorkItem: pthread_cond_broadcast failed, RC="<<rCode
             << ", emsg=" << szErrMsg<<endl;
        exit(1);
    }
}

void * thread_start_func_cpp(void *arg)
{
    if (arg == nullptr)
    {
        cerr << "thread_start_func_cpp: NULL ARG\n";
        exit(1);
    }

    Workers * threads = (Workers *)arg;
    Workers &threadRef = *threads;

    char szErrMsg[] = " ";
    //char *szErrMsg;
    while (true)
    {
        int rCode = pthread_mutex_lock(&threadRef.m_Mutex);
        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::ThreadStart: pthread_mutex_lock failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
            exit(1);
        }

        /*
         * Here means, we got lock
         */

        // if (std::empty(m_WorkItems))
        while (std::empty(threadRef.m_WorkItems)) 
        {
            /*
             * NOTE: Here I am using loop, because another woke-up threads
             * Q is empty. Wait until broadcasted or signaled which will
             * happen when another thread Enqueue item in Q.
             */

            /*
             * The following call will release m_Mutex atomically and wait for
             * for broadcast/signal by another thread via EnqueueWorkItem() when 
             * an item is pushed into work-Q.
             */
            rCode = pthread_cond_wait(&threadRef.m_WorkCondVar, &threadRef.m_Mutex);
            if (rCode)
            {
                //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
                //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
                cerr << "Workers::ThreadStart: pthread_cond_wait failed, RC="<<rCode
                     << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
                exit(1);
            }

            /*
             * We are boradcasted/signaled/waked-up by another thread via
             * EqueueWorkItem(), as data is in Q to process. As part of this
             * wake-up, it will re-get the lock (m_Mutex) within
             * pthread_cond_wait() and will be out of this loop if this 
             * data is not consumed by another thread.
             *
             * NOTE: We are using loop to check empty-Q, because when this
             * thread woke-up, another thread(s) may have woke-up earlier consumed the
             * Enqueued data and emptyed the Q.
             */
        }

        /*
         * There are items in Q and we got lock(m_Mutex) either via
         * pthread_mutex_lock or within pthread_cond_wait() after broadcasted/signaled
         */

        WorkItem * item = threadRef.m_WorkItems.front();
        threadRef.m_WorkItems.pop();

        rCode = pthread_mutex_unlock(&threadRef.m_Mutex);
        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::ThreadStart: pthread_mutex_unlock failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
            exit(1);
        }

        if (item)
        {
            item->process();

            delete item;
        }

        /*
         * Now I'll get lock again to check if Q is empty.
         * If Q is empty, it will broadcast a signal to return to caller of
         * Wait().
         */
        rCode = pthread_mutex_lock(&threadRef.m_Mutex);
        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::ThreadStart-1: pthread_mutex_lock failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
            exit(1);
        }
        bool broadcast = false;
        if (std::empty(threadRef.m_WorkItems))
        {
            broadcast = true;
        }
        rCode = pthread_mutex_unlock(&threadRef.m_Mutex);
        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::ThreadStart-1: pthread_mutex_unlock failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
            exit(1);
        }

        if (broadcast)
        {
            // Broadcast to retrun from Workers::Wait()
            rCode = pthread_cond_broadcast(&threadRef.m_WaitCondVar);
            if (rCode)
            {
                //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
                //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
                cerr << "Workers::ThreadStart: pthread_cond_broadcast failed for m_WaitCondVar, RC="<<rCode
                     << ", emsg=" << szErrMsg<<endl;
                exit(1);
            }
        }

    } // while(true)

    return nullptr;
}

void * Workers::ThreadStart(void *arg)
{
    if (arg == nullptr)
    {
        cerr << "ThreadStart: NULL ARG\n";
        //exit(1);
    }

    //Workers * threads = (Workers *)arg;
    //Workers &threaddRef = *threads;

    char szErrMsg[] = " ";
    //char *szErrMsg;
    while (true)
    {
        int rCode = pthread_mutex_lock(&m_Mutex);
        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::ThreadStart: pthread_mutex_lock failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
            exit(1);
        }

        /*
         * Here means, we got lock
         */

        // if (std::empty(m_WorkItems))
        while (std::empty(m_WorkItems)) 
        {
            /*
             * NOTE: Here I am using loop, because another woke-up threads
             * Q is empty. Wait until broadcasted or signaled which will
             * happen when another thread Enqueue item in Q.
             */

            /*
             * The following call will release m_Mutex atomically and wait for
             * for broadcast/signal by another thread via EnqueueWorkItem() when 
             * an item is pushed into work-Q.
             */
            rCode = pthread_cond_wait(&m_WorkCondVar, &m_Mutex);
            if (rCode)
            {
                //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
                //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
                cerr << "Workers::ThreadStart: pthread_cond_wait failed, RC="<<rCode
                     << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
                exit(1);
            }

            /*
             * We are boradcasted/signaled/waked-up by another thread via
             * EqueueWorkItem(), as data is in Q to process. As part of this
             * wake-up, it will re-get the lock (m_Mutex) within
             * pthread_cond_wait() and will be out of this loop if this 
             * data is not consumed by another thread.
             *
             * NOTE: We are using loop to check empty-Q, because when this
             * thread woke-up, another thread(s) may have woke-up earlier consumed the
             * Enqueued data and emptyed the Q.
             */
        }

        /*
         * There are items in Q and we got lock(m_Mutex) either via
         * pthread_mutex_lock or within pthread_cond_wait() after broadcasted/signaled
         */

        WorkItem * item = m_WorkItems.front();
        m_WorkItems.pop();

        rCode = pthread_mutex_unlock(&m_Mutex);
        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::ThreadStart: pthread_mutex_unlock failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
            exit(1);
        }

        if (item)
        {
            item->process();

            delete item;
        }

        /*
         * Now I'll get lock again to check if Q is empty.
         * If Q is empty, it will broadcast a signal to return to caller of
         * Wait().
         */
        rCode = pthread_mutex_lock(&m_Mutex);
        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::ThreadStart-1: pthread_mutex_lock failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
            exit(1);
        }
        bool broadcast = false;
        if (std::empty(m_WorkItems))
        {
            broadcast = true;
        }
        rCode = pthread_mutex_unlock(&m_Mutex);
        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::ThreadStart-1: pthread_mutex_unlock failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
            exit(1);
        }

        if (broadcast)
        {
            // Broadcast to retrun from Workers::Wait()
            rCode = pthread_cond_broadcast(&m_WaitCondVar);
            if (rCode)
            {
                //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
                //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
                cerr << "Workers::ThreadStart: pthread_cond_broadcast failed for m_WaitCondVar, RC="<<rCode
                     << ", emsg=" << szErrMsg<<endl;
                exit(1);
            }
        }

    } // while(true)

    return nullptr;
}

void Workers::Wait()
{
    /*
     * Wait until all work items in Q consumed
     */
    char szErrMsg[]=" ";
    //char *szErrMsg=" ";
    int rCode = pthread_mutex_lock(&m_Mutex);
    if (rCode)
    {
        //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
        //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
        cerr << "Workers::Wait: pthread_mutex_lock failed, RC="<<rCode
             << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
        exit(1);
    }

    /*
     * Here means, we got lock
     */

    while (!std::empty(m_WorkItems)) 
    {
        /*
         * The following call will release m_Mutex atomically
         * and wait for broadcast/signal from another thread which
         * will broadcast/signal when Q is empty
         */
        rCode = pthread_cond_wait(&m_WaitCondVar, &m_Mutex);
        if (rCode)
        {
            //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
            //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
            cerr << "Workers::Wait: pthread_cond_wait failed, RC="<<rCode
                 << ", emsg=" << szErrMsg<<", ThreadID="<< pthread_self() << endl;
            exit(1);
        }
        /*
         * Here means, broadasted/signaled by a thread from ThreadStart() 
         * because all items in Q are processed.
         * We got lock within pthread_cond_wait() upon bradcasted/signalled
         * and we have to retrun from this Wait() to caller
         * who was waiting on finishing all items in Q.
         */
    }

    rCode = pthread_mutex_unlock(&m_Mutex);
    if (rCode)
    {
        //strerror_r(rCode, szErrMsg, sizeof(szErrMsg)); 
        //szErrMsg = _strerror_r(_REENT, rCode, 1, NULL); 
        cerr << "Workers::Wait: pthread_mutex_unlock failed, RC=" << rCode
             << ", emsg=" << szErrMsg<<", ThreadID=" << pthread_self() << endl;
        exit(1);
    }
}

void *thread_start_func(void *p)
{
    return thread_start_func_cpp(p);
}
