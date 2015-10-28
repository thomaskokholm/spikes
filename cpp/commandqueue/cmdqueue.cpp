/**
   This files try to make a command / execute model using
   C++11 new thread model.
*/
#include <functional>
#include <thread>
#include <list>
#include <tuple>
#include <iostream>
#include <mutex>
#include <chrono>
#include <condition_variable>

using namespace std;

// unpack templates (http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer)
template<int ...> struct seq {};

template<int N, int ...S> struct gens : gens<N-1, N-1, S...> {};

template<int ...S> struct gens<0, S...>{ typedef seq<S...> type; };

template<typename ... Args> class CommandQueue {
    function<void (Args ...)> _fn;
    list<tuple<Args ...>> _queue;
    condition_variable _has_new, _ready;
    list<thread> _pool;     // The thread pool
    mutex _mutex;
    size_t _working;           // Number of working threads
    size_t _max;
    bool _stop;

    template<int ...S> void callFunc( tuple<Args ...> &args, seq<S...> ) {
        _fn( std::get<S>( args ) ...);
    }

    void check_pool() {
        if( _pool.size() < _max) {
            _pool.push_back( thread( [&] {
                while( ! _stop ) {
                    tuple<Args ...> args;
                    {
                        _ready.notify_one();
                        unique_lock<mutex> l( _mutex );

                        if( _queue.size() == 0 && ! _stop )
                            _has_new.wait( l );

                        if( _queue.size() > 0 ) {
                            args = _queue.front();

                            _queue.pop_front();
                        } else
                            continue;
                    }
                    this->_working++;
                    this->callFunc( args, typename gens<sizeof...(Args)>::type());
                    this->_working--;
                }
            }));

            if( _pool.size() > 0 ) {
                unique_lock<mutex> l( _mutex );

                _ready.wait( l ); // Wait for the new thread to start up
            }
        }
    }

public:
    CommandQueue( function<void (Args ... )> fn ) : _fn( fn ), _working( 0 ), _max( 10 ), _stop( false ) {
        check_pool();
    }

    ~CommandQueue() {
        {
            unique_lock<mutex> l( _mutex );

            _stop = true;

            _has_new.notify_all();
        }

        for( auto &th: _pool )
            th.join();
    }

    /*
        Wait for all workers to empty the queue
    */
    void wait() {
        while( _working ) {
            unique_lock<mutex> l( _mutex );

            _ready.wait( l );
        }
    }

    void operator()( Args... args ) {
        check_pool();

        unique_lock<mutex> l( _mutex );

        _queue.push_back( tuple<Args ...>( args... ));

        _has_new.notify_one();
    }
};

#ifdef TEST

int main( int argc, const char *argv[] )
{
    CommandQueue<int> queue( []( int cnt ) -> void {
        cout << "result: " << cnt << endl;

        this_thread::sleep_for(chrono::milliseconds( 100 ));
    });

    for( int n = 0; n != 100; n++ ) {
        //this_thread::sleep_for(chrono::milliseconds( 100 ));
        queue( n );
    }

    queue.wait();
}

#endif