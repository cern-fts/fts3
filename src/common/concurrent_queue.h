#ifndef _TQUEUE_H_
#define _TQUEUE_H_

#include <pthread.h>
#include <queue>
#include <iostream>


class concurrent_queue
{
private:
        static bool instanceFlag;
        static concurrent_queue *single;            
        pthread_mutex_t lock; // The queue lock
        pthread_cond_t  cv;   // Lock conditional variable
        int             blck; // should pop() block by default
        
public:
    static concurrent_queue* getInstance(); 
    std::queue<std::string> the_queue;  // The queue       
    void nonblock();
    void block();
    bool empty();
    unsigned int size();
    void push( std::string value );
    std::string pop(const int wait = -1);
    concurrent_queue(){
                blck = 1;
                pthread_mutex_init(&lock, NULL);
                pthread_cond_init (&cv, NULL);
        }
};


#endif
