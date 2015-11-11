/**
    Base exception the is able to make a stacktrace when the compiler supports it (gcc).

    If using the gcc compiler (or clang) this exception may print the user given text and
    additionally it is able to print out a human readable stacktrace.
*/

#include <stdexcept>
#include <string>

namespace core {
    using namespace std;

    class Exception : public std::exception {
        string _err;
        void *_st_buffer[100]; // stack trace buffer
        int _st_size; // number of entries in stack trace
    public:
        Exception( const string &err );

        // Given exception text
        const char *what() const throw();

        // Return a description of the stack where this was thrown from
        string stacktrace() const;
    };
}
