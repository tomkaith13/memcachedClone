//
//  ThreadPool.cpp
//  MiniMemcachedServer
//
//  Copyright © 2017 . All rights reserved.
//

#include "ThreadPool.hpp"


//the wrapper function for calling a start of a job
void ThreadPool::threadFunc() {
    while(1) {
        Job* currJob;
        { //scope of the conditional variable
            unique_lock<mutex> lck(mt);
            
            //wait until jobQueue is filled or if stopAccepting is set to true
            cv.wait(lck, [this]() -> bool { return this->jobQueue.size() || this->stopAccepting; });
            
            //once stopAccepting is true, and the queue is empty, we exit the thread
            if (this->stopAccepting && this->jobQueue.empty())
                return;
            
            currJob = jobQueue.front();
            jobQueue.pop_front();
        }
        
        currJob->start();
        delete currJob;
    } //end while
}

ThreadPool::ThreadPool(int threadCount) {
    tCount = threadCount;
    stopAccepting = false;
    
    for (int i = 0 ; i < tCount; i++) {
        threadARR[i] = thread(&ThreadPool::threadFunc, this);
    }
    
}

ThreadPool::~ThreadPool() {
    //stop receiving new jobs and empty the job Queue
    stopReceivingJobs();
    
    //join all threads spun
    for (int i =0; i <tCount; i++)
        threadARR[i].join();
}

bool
ThreadPool::AddJob(Job* j) {
    { //scope for locking the queue mutex before pushing in a job
        unique_lock<mutex> guard(mt);
        if (stopAccepting)
            return false;
        jobQueue.push_back(j);
    }
    //signal that a job has been entered to one of the threads
    cv.notify_one();
    return true;
}

void ThreadPool::stopReceivingJobs() {
    
    // locking and setting stopAccepting to true to signal a wrapup
    unique_lock<mutex> lck(mt);
    stopAccepting = true;
}

void ThreadPool::restartReceivingJobs() {
    
    // locking and setting stopAccepting to false to signal a restart of job queueing
    unique_lock<mutex> lck(mt);
    stopAccepting = false;
}
