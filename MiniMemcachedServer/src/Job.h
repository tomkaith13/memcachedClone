//
//  Job.h
//  MiniMemcachedServer
//
//

#ifndef Job_h
#define Job_h

/*
 *  This is the Job Class which is fed in the jobQueue of the ThreadPool
 *  Args: jobID,
 *          a function pointer that points a function that accepts void*
 *          and return void*,
 *          Argument to the function above in void* pointer.
 */
class Job {
    void* (*startRoutine) (void*);
    void* fArg;
    
public:
    Job() {};
    Job(void* (*startFP) (void*), void* arg) {
        startRoutine = startFP;
        fArg = arg;
    }
    virtual void start() {
        startRoutine(fArg);
    }
};

#endif /* Job_h */
