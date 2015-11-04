#include "exception.hpp"
#include <sstream>
#ifdef __GNUC__
#include <execinfo.h>
#include <cxxabi.h>
#endif

// Inspired of this discussion
// http://stackoverflow.com/questions/77005/how-to-generate-a-stacktrace-when-my-gcc-c-app-crashes

namespace core {
    using namespace std;

    Exception::Exception( const string &err ) : _err( err ), _st_size( 0 ) {
#ifdef __GNUC__
        _st_size = ::backtrace( _st_buffer, sizeof( _st_buffer ));
#endif
    }

    string Exception::stacktrace() const {
        stringstream os;

#ifdef __GNUC__
        char **msg = backtrace_symbols( _st_buffer, _st_size );

        // skip first stack frame (points here)
        for (int i = 1; i < _st_size && msg != NULL; ++i) {
            char *mangled_name = 0, *offset_begin = 0, *offset_end = 0;

            // find parentheses and +address offset surrounding mangled name
            for (char *p = msg[i]; *p; ++p) {
                if (*p == '(') {
                    mangled_name = p;
                } else if (*p == '+') {
                    offset_begin = p;
                } else if (*p == ')') {
                    offset_end = p;
                    break;
                }
            }

            // if the line could be processed, attempt to demanding the symbol
            if (mangled_name && offset_begin && offset_end && mangled_name < offset_begin) {
                *mangled_name++ = '\0';
                *offset_begin++ = '\0';
                *offset_end++ = '\0';

                int status;
                char * real_name = abi::__cxa_demangle(mangled_name, 0, 0, &status);

                // if demandingly is successful, output the demangled function name
                if (status == 0) {
                    os << "[bt]: (" << i << ") " << msg[i] << " : "
                       << real_name << "+" << offset_begin << offset_end
                       << endl;

                } else { // otherwise, output the mangled function name
                    os << "[bt]: (" << i << ") " << msg[i] << " : "
                       << mangled_name << "+" << offset_begin << offset_end
                       << endl;
                }
                free(real_name);
            } else { // otherwise, print the whole line
                os << "[bt]: (" << i << ") " << msg[i] << endl;
            }
        }
        os << endl;

        free( msg );

        return os.str();
#else
        return "no stacktrace";
#endif
    }

    const char *Exception::what() const throw() {
        return _err.c_str();
    }
}