#include "service.hpp"
#include "../value/value.hpp"
#include <iostream>
#include <ctime>
#include <list>
#include <thread>
#include <condition_variable>

using namespace std;
using namespace core;
using namespace fuwu;

static map<type_index, string> json_types {
    {typeid( int ), "number"},
    {typeid( float ), "number"},
    {typeid( double ), "number"},
    {typeid( string ), "string"},
    {typeid( bool ), "boolean"},
    {typeid( void ), "undefined"},
    {typeid( cvector ), "array"},
    {typeid( cmap ), "object"}
};

string fuwu::json_type( const type_info &t )
{
    return json_types[ t ];
}

class NullSessionStore : public SessionStore
{
public:
    void save( const string &sess_id, const cmap &data ) const {
    }

    bool has( string &sess_id ) const {
        return false;
    }

    cmap load( const string &sess_id ) const {
        return cmap{};
    }

    void touch( string &sess_id ) const {

    }

};

ClientSession::ClientSession( const string &sess_id ) : _id( sess_id ) {}

class ClientSessionImpl : public ClientSession {
    cmap _values;
    chrono::system_clock::time_point _last_touched;
    mutex _mutex;
public:
    ClientSessionImpl( const string &sess_id ) : ClientSession( sess_id ) {
        _last_touched = chrono::system_clock::now();
    }

    bool has( const string &name ) const {
        lock_guard<mutex> lock( const_cast<mutex &>( _mutex ));

        return _values.count( name ) > 0;
    }

    Value get( const string &name ) const {
        lock_guard<mutex> lock( const_cast<mutex &>( _mutex ));

        auto i = _values.find( name );

        if( i != _values.end())
            return i->second;

        throw invalid_argument( "can't find session value : " + name );
    }

    void set( const string &name, const Value &v ) {
        lock_guard<mutex> lock( _mutex );

        _values.insert( make_pair( name, v ));
    }

    bool allow( const string &access ) const {
        return true; // XXX
    }

    void touch() {
        _last_touched = chrono::system_clock::now();
    }

    chrono::seconds age() const {
        return chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - _last_touched);
    }
};

ClientSessionFactory::ClientSessionFactory() : _session_store( make_shared<NullSessionStore>())
{
}

ClientSessionFactory &ClientSessionFactory::inst()
{
    static ClientSessionFactory *inst = nullptr;

    if( inst == nullptr )
        inst = new ClientSessionFactory();

    return *inst;
}

bool ClientSessionFactory::has( const string &sess_id ) const
{
    lock_guard<mutex> lock( const_cast<mutex &>( _mutex ));

    return _sessions.count( sess_id ) > 0;
}

sess_hndl ClientSessionFactory::find( const string &sess_id ) const
{
    lock_guard<mutex> lock( const_cast<mutex &>( _mutex ));

    auto i = _sessions.find( sess_id );
    if( i != _sessions.end())
        return i->second;

    throw invalid_argument( "session id not found in factory " + sess_id );
}

sess_hndl ClientSessionFactory::create( const string &sess_id )
{
    lock_guard<mutex> lock( _mutex );

    if( !sess_id.empty()) {
        auto s = make_shared<ClientSessionImpl>( sess_id );

        _sessions.insert( make_pair( sess_id, s ));

        return s;
    }
    throw invalid_argument( "session id string must not be empty" );
}

// Create a session that has no id and will not be persisted
sess_hndl ClientSessionFactory::create_temp() const
{
    return make_shared<ClientSessionImpl>( "" );
}

void ClientSessionFactory::store_set( shared_ptr<SessionStore> sess_handler )
{
    _session_store = sess_handler;
}

/////////////
// Session

Session::~Session()
{
    // Commit data if not failed
}

bool Session::has( const string &name ) const
{
    return _sess->has( name );
}

Value Session::get( const string &name ) const
{
    return _sess->get( name );
}

void Session::set( const string &name, const Value &v )
{
    _sess->set( name, v );
}

Config &Session::config()
{
    throw invalid_argument( "session config not impl, yet" );
}

bool Session::allow( const string &access ) const {
    return _sess->allow( access );
}

 // Session will commit data is this is not called (error handling)
void Session::rollback()
{
    // Maybe not store values before success ?
}

//////////
// Event

Event::Event( const string &name ) : _name( name ) {}

Event::Event( const string &name, initializer_list<pair<const string, Value>> l )
    : _name( name ), _values( l ) {}

Event::Event( const Event &ev ) : _name( ev._name ), _values( ev._values ) {}

bool Event::has( const string &name ) const
{
    return _values.count( name );
}

Value Event::get( const string &name ) const
{
    auto i = _values.find( name );

    if( i != _values.end())
        return i->second;

    throw invalid_argument( "error getting event value " + name );
}

void Event::set( const string &name, const Value &v )
{
    _values.insert( make_pair( name, v ));
}


///////////////////
// EventDispatcher

class EventDispatcherImpl : public EventDispatcher {
    bool _stop;
    thread _worker;
    mutex _mutex, _reg_mutex;
    condition_variable _has_new;
    list<Event> _queue;
    list<function<void( const Event &ev )>> _handlers;
public:
    EventDispatcherImpl() : _stop( false ), _worker( &EventDispatcherImpl::event_loop, this ) {
    }

    ~EventDispatcherImpl() {
        _stop = true;
        _has_new.notify_one();
        _worker.join();
    }

    void post( const Event &ev ) {
        lock_guard<mutex> l( _mutex );

        _queue.push_back( ev );

        _has_new.notify_one();
    }

    void reg( function<void( const Event &ev )> fn ) {
        lock_guard<mutex> lock( _reg_mutex );

        _handlers.push_back( fn );
    }

protected:
    void event_loop() {
        while( _stop == false ) {
            unique_lock<mutex> l( _mutex );

            if( _queue.size() == 0 )
                _has_new.wait( l );

            if( _queue.size() > 0 ) {
                Event ev = _queue.front();
                _queue.pop_front();

                try {
                    lock_guard<mutex> lock( _reg_mutex );

                    for( auto handler: _handlers ) {
                        handler( ev );
                    }
                } catch( const exception &ex ) {
                    clog << "ERROR: handling event '" << ev.name_get() << "', saying " << ex.what() << endl;
                }
            }
        }
    }
};

EventDispatcher &EventDispatcher::init()
{
    static EventDispatcher *inst = nullptr;

    if( inst == nullptr )
        inst = new EventDispatcherImpl();

    return *inst;
}