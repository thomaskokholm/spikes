#include "base.hpp"
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <set>

using namespace uWsgi;
using namespace std;

class HelloTestAppClass {
public:
    HelloTestAppClass() {
        register_app( "test", bind( &HelloTestAppClass::hello_world, this, placeholders::_1 ));
    }

    void hello_world(Request &req) const {
        if( req.method() == "POST" ) {
            istream &is = req.read_body();
            clog << "body = " << is.rdbuf() << endl;
        }

        req.prepare_headers(200);
        req.set_body("Hello, app world", "text/plain");
    }
};

class HelloTestClass {
public:
    HelloTestClass() {
        register_request( bind( &HelloTestClass::hello_world, this, placeholders::_1 ));
    }

    void hello_world(Request &req) const {
        req.prepare_headers(200);
        req.set_body("Hello, world", "text/plain");
    }
};

static HelloTestClass hello;
static HelloTestAppClass app_hello;