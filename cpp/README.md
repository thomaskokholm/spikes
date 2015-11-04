# C++ spikes

All the C++ code in this repos are held in a pure std C++. There is only binding to function
libs if it is needed to perform a task.

If there is a way to do things in std C++ 11 or 14, that is the preferred way.

## command queue

This can be used as a input queue for a pool of C++11 threads that need a common command
dispatched out to more than one thread.

The too are templated and uses the lockings found in std C++

I much modern code the use of channels are more common, but this pattern is plain vanilla
command execute.

## value

C++ is static types, but the world is not. Over the time many libraries have been made to
try to make a dynamic value for C++, but this is an attempt to use as many C++ elements as
possible to prevent reinvention.

We use C++ rtti for type id for our dynamic type, and we use shared_ptr for all value references,
and all types used inside the value must be stream able.

There are some natural expansions to a value system, like the enclosed json parser and
serializer but these will have there own spikes.

## JsonRPC

Handling JsonRPC 2.0 services can be done as soon as there is a way to transfer JSON to
an internal format like the Value. But this server support layer tries to use C++ even more
by using the type info found on the C++ function as the driver for :

* SMD service list [like this](http://json-schema.org/latest/json-schema-hypermedia.html)
  and [this](http://www.simple-is-better.org/json-rpc/jsonrpc20-schema-service-descriptor.html)
* PRC envelope argument mapping

This way we can with ease add a function to our RPC handler, and this handler will be able to
tell with functions has been added and what kind arguments it requires. This way we can have
runtime checked mapping of static arguments.

## [uWSGI](http://uwsgi-docs.readthedocs.org/en/latest/index.html)

Wrapper for C++ to make it more easy to make uwsgi plugins in pure C++. uwsgi are used many
places and it takes care of many trivial network elements and works perfectly with nginx and
other web services.

## DB

Many wrappers for DB and SQL have been made, but they either limited in there functionality like
[libdbi](http://libdbi.sourceforge.net/docs/programmers-guide/index.html) or they are tight to
larger frameworks like boost, Qt or Gtk+.

My goal is again to make it a C++11 compliment as possible, and only depending on the value system
defined here.