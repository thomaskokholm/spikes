#include "service.hpp"

namespace fuwu {
    /**
        Entity arguments defining what is possible as argueents for requesting
        a list of elements.
    */
    class EntityArgument {
    public:
        string name;
        string desc;    // Prosa description of argument
        const type_info &type; // Rtti type info
        bool optional;  // if true this value may be ommited
    };

    /**
        What columns to expect from this entity upon data return
    */
    class EntityColumn {
    public:
        string name;
        string desc;    // Prosa description of column
        const type_info &type; // Rtti type info
        bool readonly;  // does the entity accept this value as an update value
        bool order;     // May we order on this column
        string hint;    // Any kind of render hint
    };

    /**
        This entity form the basis for all RESTful interfaces, and serves like
        an abstraction for all REST resouces.

        It also holds meta data about fields and columns used for both validation
        and service definitions.
    */
    class Entity {
        typedef vector<EntityColumn> columns_t;
        typedef vector<EntityArgument> arguments_t;

    protected:
        string _desc;
        columns_t _columns;
        arguments_t _arguments;

    public:
        string desc_get() const {return _desc;}
        columns_t columns_get() const {return _columns;}
        arguments_t arguments_get() const {return _arguments;}

        virtual cmap do_get( Session &sess, const Value &key ) const = 0;
        virtual cvector do_list( Session &sess, const cmap &args, const strings &order, long limit, long offset ) const = 0;
        virtual cmap do_create( Session &sess, const cmap &args ) const = 0;
        virtual bool do_update( Session &sess, const Value &key, const cmap &data ) const = 0;
        virtual bool do_delete( Session &sess, const Value &key ) const = 0;
    };

    typedef shared_ptr<Entity> entity_ptr;
    class EntityDispatcher {
        map<string, entity_ptr> _entities;
        EntityDispatcher() {}
    public:
        static EntityDispatcher &inst();

        void reg( const string &name, entity_ptr ent );
        void unreg( const string &name );
        bool has_a( const string &name ) const;
        entity_ptr find( const string &name ) const;
    };
}