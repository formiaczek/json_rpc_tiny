json_rpc_tiny
=============

Tiny json_rpc implementation for C and /C++ designed to be used also in on embedded systems.
An incredibly simple (and simple to use), yet powerful library that implements rpc server functionality as well as all json-parsing futures potentially needed by programs using JSON.


Some of the futures:
 - implemented to make no allocations and to be (time & space) efficient
 - allows for use of pre-allocated storage for handlers, response and request buffers etc
 - compatible with JSON-RPC 2.0 (version is automatically recognised and response created accordingly)
 - contains simple service / function handler registration mechanism (to implement RPC service)
 - provides interface to aid params extraction from handlers (named and position-based params, to-int conversions (that also support hex/octal base)).
 - implements easy response creation using: json_rpc_create_result(): on success, or json_rpc_create_error() on failure (using custom error response or standard error codes).
 - rpc service supports other futures, including: passing an argument to the handler (and it can be different for each call), passing pre-allocated response buffer (can be different for each call).
 - provides copy & allocation-less JSON parsing mechanism that allows extracting named/position based members, extraction of integers (also including hex/octal/negative values - so it can be used outside of RPC etc)
 - can be used in multi-threaded code (proviced that each thread uses it's own storage instance)
 
See example code for more details.
