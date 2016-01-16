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

    void hello_world(Request &req) {
        if( req.method() == "POST" ) {
            istream &is = req.read_body();
            clog << "body = " << is.rdbuf() << endl;
        }

        req.prepare_headers("200 OK");
        req.add_content_type("text/plain");
        req.write_body("Hello, app world");
    }
};

class HelloTestClass {
public:
    HelloTestClass() {
        register_request( bind( &HelloTestClass::hello_world, this, placeholders::_1 ));
    }

    void hello_world(Request &req) {
        req.prepare_headers("200 OK");
        req.add_content_type("text/plain");
        req.write_body("Hello, world");
    }
};

static HelloTestClass hello;
static HelloTestAppClass app_hello;