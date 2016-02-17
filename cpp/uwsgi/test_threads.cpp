#include "base.hpp"
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <set>

using namespace uWsgi;
using namespace std;

class ThreadsTestClass {
    mutex _threads_lock;
    set<std::thread::id> _threads;
    volatile long _reqs;
public:
    ThreadsTestClass() {
        register_request( bind( &ThreadsTestClass::hello_world, this, placeholders::_1 ));
        clog << "Startup tid " << std::this_thread::get_id() << endl;
        _reqs = 0;
    }

    void thread_count() {
        lock_guard<mutex> l( _threads_lock );

        if( _threads.count( this_thread::get_id() ) == 0 ) {
            clog << "tid added " << this_thread::get_id() << endl;
            _threads.insert(this_thread::get_id());
        }

        clog << _threads.size() << " threads in use" << endl;
    }

    void hello_world(Request &req);
};

void ThreadsTestClass::hello_world(Request &req) {
    thread_count();
    ++_reqs;

    req.prepare_headers(200);
    req.set_body("Hello, world", "text/html");

    clog << "requests " << _reqs << endl;
}

static ThreadsTestClass thread;