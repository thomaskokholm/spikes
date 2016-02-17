#ifndef __SERVICE_HPP__
#define __SERVICE_HPP__ 1

#include <map>
#include <string>
#include <memory>
#include <typeindex>
#include <chrono>
#include <mutex>
#include "../value/value.hpp"

namespace fuwu {
    using namespace std;
    using namespace core;

    typedef vector<string> strings;

    string json_type( const type_info &t );

    class Config {
    public:
        virtual bool has( const string &path ) const = 0;
        virtual Value get( const string &path ) const = 0;
        virtual void set( const string &path, const Value &nval ) = 0;

        Value operator[]( const string &path ) const {return get( path );}
    };

    // System wide config
    Config &config();

    class Event {
        string _name;
        cmap _values;

    public:
        Event( const string &name );
        Event( const string &name, initializer_list<pair<const string, Value>> l );
        Event( const Event &ev );

        string name_get() const {return _name;}

        bool has( const string &name ) const;
        Value get( const string &name ) const;
        void set( const string &name, const Value &v );
        Value operator[]( const string &name ) const {return get( name );}
    };

    class EventDispatcher {
    public:
        static EventDispatcher &init();

        /**
            Post an event for later processing by the registred handlers
        */
        virtual void post( const Event &ev ) = 0;

        /**
            Register a handler function that gets all events postet but
            in the worker thread, at a later time.
        */
        virtual void reg( function<void( const Event &ev )> fn ) = 0;
    };

    class ClientSession {
    protected:
        string _id;
        ClientSession( const string &sess_id );

    public:
        string id_get() const {return _id;}

        virtual bool has( const string &name ) const = 0;
        virtual Value get( const string &name ) const = 0;
        virtual void set( const string &name, const Value &v ) = 0;

        virtual bool allow( const string &access ) const = 0;

        virtual void touch() = 0 ; // called when request uses this instance
        virtual chrono::seconds age() const = 0;
    };

    class SessionStore {
    public:
        virtual void save( const string &sess_id, const cmap &data ) const = 0;
        virtual bool has( string &sess_id ) const = 0;
        virtual void touch( string &sess_id ) const = 0;
        virtual cmap load( const string &sess_id ) const = 0;
    };

    typedef shared_ptr<ClientSession> sess_hndl;
    class ClientSessionFactory {
        shared_ptr<SessionStore> _session_store;
        map<string, shared_ptr<ClientSession>> _sessions;
        mutex _mutex;

        ClientSessionFactory();
    public:
        static ClientSessionFactory &inst();

        bool has( const string &sess_id ) const;
        sess_hndl find( const string &sess_id ) const;
        sess_hndl create( const string &sess_id );
        // Create a session that has no id and will not be persisted
        sess_hndl create_temp() const;

        void store_set( shared_ptr<SessionStore> sess_handler );
    };

    // this is a light weight wrapper over a ClientSession that lives as long as
    // the client request
    class Session {
        sess_hndl _sess;
    public:
        Session( sess_hndl sess ) : _sess( sess ) {}
        ~Session();

        bool has( const string &name ) const;
        Value get( const string &name ) const;
        void set( const string &name, const Value &v );
        Value operator[]( const string &name ) const {return get( name );}

        Config &config(); // session specific config

        bool allow( const string &access ) const;

        void rollback(); // Session will commit data is this is not called (error handling)
    };
}

#endif