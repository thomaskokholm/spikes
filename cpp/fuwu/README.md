## fuwu (服务) web service core

Define a web service for basic handling of JsonRPC 2.0 and RESTful (json) services
that handle all basic needs for services like

 * Session Handling
 * JsonRPC handling
 * RESTful handling
 * database handling

While making this core, I have the following goals :

 * Use C++11, and prefer standards over anything else.
 * Don't reinvent more than needed, so
   * I use libpqxx as database handler (I only use Psql)
   * use uwsgi as connection and workload handler
 * offload a many things as possible, to make performance nice and to prevent
   reinventing wheels again.

The idea of using C++ is of cause speed, but another thing is the language's strict
typing, that really is a big help, together with a deterministic memory model.

# internal models

* Request

  Each time a client contact out service we have a request, and uwsgi gives us this as a request structure holding data related to this client request.

* Session

  This defines a single client, so that it is possible for our service to references
  to the same client on each request from this. This will be used to identify a user
  on the system, and control authentication and other things.

  All the logic about getting and setting cookies on the client are handled by the
  core logic and the rest of the system dont need to worry about it.

  * Session values

    A session can hold a set of values that will exist as long as the session
    exists, when the session no longer exist these values are lost. these are
    typiacally used for client states like login.

  * Config values

    There are config set for the server to startup (in uwsgi), and then there are config value only relevant for the client system, or the logic of the app.

    These can be stored in config, and can either be system wise or user specific.
    There the user specific takes precedence over system wise.

* Entity

  All RESTful operation are all mapped to a entity handler, using this simple
  mapping :

  * GET ../id -> do_get
  * GET ..?   -> do_list
  * POST      -> do_create
  * PUT       -> do_update
  * DELETE    -> do_delete  

  All REST calles gets an Session ref as first argument.

* Method

  Is a JsonRPC method call, this gets a Session ref as first argument and what
  ever agument it needs a the rest.

  It is event possible to make C++ understand a function footprint directly
  while compiling, in order to be able to give more precise error handling
  and introspection.

[spec](http://www.jsonrpc.org/specification)  

SMD

   To be able as the service from a client what the server is able to do, this
   is service provides it by asking all emtity and methods about there meta data
   it may compile this kind of list, and the client can then validate and test
   its interface.

[spec](http://dojotoolkit.org/reference-guide/1.10/dojox/rpc/smd.html)
