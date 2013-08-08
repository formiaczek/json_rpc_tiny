json_rpc_tiny
=============

tiny json_rpc implementation for c/c++.

It is very simple and simple to use:
 - uses pre-allocated storage
 - compatible with JSON-RPC 2.0 (version is automatically recognised)
 - can be used in C / C++ code (also in embedded code, where allocators and stdlib is not available).
 - contains simple service / function handler registration mechanism.
 - gives full control over arguments extraction from the handler.
 - implements easy response creation using: json_rpc_result(): on success, 
   or json_rpc_error(): on failure.
 - allows for passing an argument to the handler, and it can be different for each call.
 - allows for passing pre-allocated response buffer (can be different for each call).
 - can be used in multi-threaded code (each thread needs it's own storage instance)
 
See example for more details.
