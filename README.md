json_rpc_tiny
=============

tiny json_rpc implementation for C (inc. embedded with no stdlib) and /c++.
An incredibly simple (and simple to use), yet powerful library that 
implements server, but could also be used for client code.

Some of the futures:
 - uses pre-allocated storage
 - compatible with JSON-RPC 2.0 (version is automatically recognised)
 - can be used in C / C++ code (also in embedded code, where allocators and stdlib is not available).
 - contains simple service / function handler registration mechanism.
 - provides interface to aid params extraction from handlers (including named params and to-int conversions).
 - implements easy response creation using: json_rpc_result(): on success, 
   or json_rpc_error(): on failure.
 - allows for passing an argument to the handler, and it can be different for each call.
 - allows for passing pre-allocated response buffer (can be different for each call).
 - can be used in multi-threaded code (each thread needs it's own storage instance)
 
See example for more details.
