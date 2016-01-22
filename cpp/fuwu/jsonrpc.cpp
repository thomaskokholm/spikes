#include "handler.hpp"
#include "../value/json.hpp"
#include <iostream>

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
    JsonrpcHandler() : Handler( "jsonrpc" ) {}

    void handle( uWsgi::Request &req, Session &sess ) const {
        if( req.method() != "POST" ) {
            req.prepare_headers(200);
            req.set_body("JsonRPC need POST method", "text/plain");
            return;
        }

        if( req.content_type() != "application/json" ) {
            req.prepare_headers(200);
            req.set_body("JsonRPC need application/json mime type", "text/plain");
            return;
        }

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

            if( cenv.count( "jsonrpc" ) > 0 && cenv[ "jsonrpc" ].get<string>() == "2.0") {
                if( cenv.count( "method" ) > 0 ) {
                    string method = cenv[ "method" ].get<string>();

                    if( _rpc.has_a( method )) {
                        Value res;

                        if( cenv.count( "params" ) > 0 ) {
                            if( cenv[ "params" ].is_type<cvector>()) {
                                cvector &params = cenv[ "params" ].get<cvector>();

                                res = _rpc.call( sess, method, params );

                                if( notice )
                                    return Value();
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

            if( cres.count("error") == 0)
                error_set( cres, INVALID_REQUEST );

            return Value( cres );
        };

        Value env = json_parse( req.read_body());

        if( env.is_type<cmap>()) { // Handle single
            Value res = hndl( sess, env.get<cmap>() );

            req.prepare_headers(200);

            stringstream os;

            json_serialize(os, res);
            req.set_body(os.str(), "application/json");
        } else if( env.is_type<cvector>()) { // Handle batch
            cvector &batch = env.get<cvector>();

            cvector batch_list;

            for( auto &single: batch ) {
                Value res = hndl( sess, single.get<cmap>());

                if(res.get<cmap>().count( "id" ) > 0)
                    batch_list.push_back( res );
            }

            req.prepare_headers(200);

            if( batch_list.size() > 0 ) {
                stringstream os;

                json_serialize(os, Value( batch_list ));

                req.set_body(os.str(), "application/json");
            }
        } else {
            cmap cerror {
                {"error", Value( INVALID_REQUEST )},
                {"message", Value( "bad JsonRPC envelope" )}
            };

            stringstream os;

            json_serialize(os, Value( cerror ));

            req.prepare_headers(200);
            req.set_body(os.str(), "application/json");
        }
    }
};

static JsonrpcHandler rpc_handler;