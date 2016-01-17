#ifndef __RPC_HANDLER_HPP__
#define __RPC_HANDLER_HPP__ 1

#include "service.hpp"

namespace Service {
    using namespace std;
    using namespace core;

    class Rpc {
    protected:
        string _name;

    public:
        string name_get() const {return _name;}

        virtual cmap service_def() const = 0;

        virtual Value call( Session &sess, const cvector &params ) const = 0;
    };

    class RpcDispacher {
    private:
        map<string, Rpc *> _methods;

    public:
        Rpc &reg( Rpc * );
        bool unreg( const string &name );

        Value call( Session &sess, const string &name, const cvector &params ) const;
    };
}

#endif