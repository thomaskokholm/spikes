#ifndef __RESTFUL_HANDLER_HPP__
#define __RESTFUL_HANDLER_HPP__ 1

#include "service.hpp"

namespace Service {
    struct ListResult {
        size_t total;
        cvector list;
    };

    class Restful {
    protected:
        string _name;
    public:
        string name_get() const {return _name;}

        virtual cmap do_get( Session &sess, const Value &key ) const = 0;
        virtual ListResult do_list( Session &sess, const cmap &args, const strings order, long limit = -1, long offset = 0 ) const = 0;
        virtual cmap do_create( Session &sess, const cmap &data ) const = 0;
        virtual bool do_update( Session &sess, const Value &key, const cmap &data ) = 0;
        virtual bool do_delete( Session &sess, const Value &key ) const = 0;
    };

    class RestfulDispacher {
    private:
        map<string, Restful *> _entities;

    public:
        Restful &reg( Restful * );
        bool unreg( const string &name );

        void dispatch(); // A request object of a kind
    };
}

#endif