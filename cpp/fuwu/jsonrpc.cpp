#include "handler.hpp"
#include "../value/json.hpp"

using namespace fuwu;
using namespace core;

enum JsonError {
    SERVER_ERROR     = -32000, // to -32099, Reserved for implementation-defined server-errors
    PARSE_ERROR      = -32700, // Invalid JSON was received by the server,
                               // An error occurred on the server while parsing the JSON text.
    INVALID_REQUEST  = -32600, // The JSON sent is not a valid Request object.
    METHOD_NOT_FOUND = -32601, // The method does not exist / is not available.
    INVALID_PARAMS   = -32602, // Invalid method parameter(s).
    INTERNAL_ERROR   = -32603  // Internal JSON-RPC error
};

static map<int, string> jsonrpc_error {
    {PARSE_ERROR,      "Parse error: Invalid JSON was received by the server, "
                       "An error occurred on the server while parsing the JSON text."},
    {INVALID_REQUEST,  "Invalid Request: The JSON sent is not a valid Request object."},
    {METHOD_NOT_FOUND, "Method not found: The method does not exist / is not available."},
    {INVALID_PARAMS,   "Invalid params:  Invalid method parameter(s)."},
    {INTERNAL_ERROR,   "Internal error:  Internal JSON-RPC error."}
};

void error_set( cmap &cres, int errnr )
{
    cres[ "error" ] = Value( errnr );
    cres[ "message" ] = Value( jsonrpc_error[ errnr ]);
}

class JsonrpcHandler : public Handler {
    RpcDispatcher _rpc;
public:
    JsonrpcHandler() {
        // Make this a uwsgi app to make uwsgi do the routing
        uWsgi::register_app( "jsonrpc",
            bind( &JsonrpcHandler::request, this, placeholders::_1 ));
    }

    void handle( uWsgi::Request &req, Session &sess ) const {
        // Handle the RPC !!!
        auto hndl = [this](Session &sess, cmap &cenv) -> Value {
            bool notice = true;
            cmap cres {
                {"jsonrpc", Value( "2.0" )}
            };

            if( cenv.count( "id" ) > 0 ) {
                cres[ "id" ] = cenv[ "id" ];
                notice = false;
            }

            if( cenv[ "jsonrpc" ].get<string>() == "2.0") {
                if( cenv.count( "method" ) > 0 ) {
                    string method = cenv[ "method" ].get<string>();

                    if( _rpc.has_a( method )) {
                        Value res;

                        if( cenv.count( "params" ) > 0 ) {
                            if( cenv[ "params" ].is_type<cvector>()) {
                                cvector &params = cenv[ "params" ].get<cvector>();

                                res = _rpc.call( sess, method, params );
                            } else
                                error_set( cres, INVALID_PARAMS );
                        } else
                            res = _rpc.call( sess, method, cvector() );

                        cres[ "result" ] = res;

                        return Value( cres );
                    } else
                        error_set( cres, METHOD_NOT_FOUND );
                }
            }

            error_set( cres, INVALID_REQUEST );

            return Value( cres );
        };

        Value env = json_parse( req.read_body());

        if( env.is_type<cmap>()) { // Handle single
            Value res = hndl( sess, env.get<cmap>() );

            if( res.get<cmap>().count( "id" ) > 0 ) {
                stringstream os;

                json_serialize(os, res);

                req.prepare_headers("200 OK");
                req.add_content_type("application/json");
                req.write_body(os.str());
            }
        } else if( env.is_type<cvector>()) { // Handle batch
            cvector &batch = env.get<cvector>();

            cvector batch_list;

            for( auto &single: batch ) {
                Value res = hndl( sess, single.get<cmap>());

                if(res.get<cmap>().count( "id" ) > 0)
                    batch_list.push_back( res );
            }

            if( batch_list.size() > 0 ) {
                stringstream os;

                json_serialize(os, Value( batch_list ));

                req.prepare_headers("200 OK");
                req.add_content_type("application/json");
                req.write_body(os.str());
            }
        }
    }
};

static JsonrpcHandler rpc_handler();