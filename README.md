json_rpc_tiny
=============

tiny json_rpc implementation for c/c++.

It is very simple and simple to use:
 - can be used in C / C++ code (also in embedded code, where allocators and stdlib is not available).
 - simply register your handlers before using the service.
 - execute json_rpc_exec() function with data (that was, e.g. received from the network).
 - in your handler - extract and use parameter(s), define response data and create response string (with the use of pre-allocated buffer) using json_rpc_response()
 - compatible with JSON-RPC 2.0
 
See example for more details.
