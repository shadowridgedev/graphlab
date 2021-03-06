#ifndef GRAPHLAB_SHARED_TERMINATION_HPP
#define GRAPHLAB_SHARED_TERMINATION_HPP

#include <graphlab/parallel/pthread_tools.hpp>
#include <graphlab/parallel/atomic.hpp>

namespace graphlab {
  /**
     simple condition variable based shared termination checker.
     When a processor decides to go to sleep, it should call 
     - begin_sleep_critical_section(cpuid), 
     - check the state of the queue.
     - If the queue has jobs, call cancel_sleep_critical_section().
     - If the queue has no jobs, then call end_sleep_critical_section(cpuid)
     - If (end_sleep_critical_section() returns true, the scheduler can terminate.
     Otherwise it must loop again.
  */
  class shared_termination {
  public:
    shared_termination(size_t ncpus) {
      numactive = ncpus;
      numcpus = ncpus;
      done = false;
      trying_to_sleep.value = 0;
      sleeping.resize(ncpus);
      for (size_t i = 0; i < ncpus; ++i) sleeping[i] = 0;
    }
  
    void reset() {
      numactive = sleeping.size();
      numcpus = sleeping.size();
      done = false;
      trying_to_sleep.value = 0;
      for (size_t i = 0; i < sleeping.size(); ++i) sleeping[i] = 0;
    }
  
    ~shared_termination(){ }
  
    void begin_sleep_critical_section(size_t cpuid) {
      trying_to_sleep.inc();
      sleeping[cpuid] = true;
      m.lock();
    }

    void cancel_sleep_critical_section(size_t cpuid) {
      m.unlock();
      sleeping[cpuid] = false;
      trying_to_sleep.dec();
    }

    bool end_sleep_critical_section(size_t cpuid) {
      // if done flag is set, quit immediately
      if (done) {
        m.unlock();
        trying_to_sleep.dec();
        sleeping[cpuid] = false;
        return true;
      }
      /*
        Assertion: Since numactive is decremented only within 
        a critical section, and is incremented only within the same critical 
        section. Therefore numactive is a valid counter of the number of threads 
        outside of this critical section. 
      */
      numactive--;
    
      /*
        Assertion: If numactive is ever 0 at this point, the algorithm is done.
        WLOG, let the current thread which just decremented numactive be thread 0
      
        Since there is only 1 active thread (0), there must be no threads 
        performing insertions. Since only 1 thread can be in the critical section 
        at any time, and the critical section checks the status of the task queue, 
        the task queue must be empty.
      */
      if (numactive == 0) {
        done = true;
        cond.broadcast();
      }
      else {
        cond.wait(m);
        // here we are protected by the mutex again.
        if (!done) numactive++;
      }
      m.unlock();
      trying_to_sleep.dec();
      sleeping[cpuid] = false;
      return done;
    }
  
    void new_job() {
      /*
        Assertion: numactive > 0 if there is work to do.
        This is relatively trivial. Even if no threads wake up in time to 
        pick up any jobs, the thread which created the job must see it in the 
        critical section.
      */
      if (trying_to_sleep.value > 0) {
        m.lock();
        if (numactive < numcpus) cond.broadcast();
        m.unlock();
      }
    }

    void new_job(size_t cpuhint) {
      if (sleeping[cpuhint]) {
        m.lock();
        if (numactive < numcpus) cond.broadcast();
        m.unlock();
      }
    }

    size_t num_active() {
      return numactive;
    }
  private:
    conditional cond;
    mutex m;
    size_t numactive;
    size_t numcpus;
    atomic<size_t> trying_to_sleep;
    std::vector<char> sleeping;
    bool done;
  };

}
#endif
