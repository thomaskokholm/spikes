#include "base.hpp"
#include <string>

using namespace uwsgi;
using namespace std;

class HelloClass {
public:
    HelloClass() {
        uwsgi::register_request( bind( &HelloClass::hello_world, this, placeholders::_1 ));
    }
    void hello_world(Request &req);
};

void HelloClass::hello_world(Request &req) {
    string path_info = req.get("PATH_INFO");

    req.prepare_headers("200 OK");
    req.add_content_type("text/html");
    req.write_body(path_info);
}