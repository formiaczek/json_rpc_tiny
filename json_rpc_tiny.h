/**
 @file    json_rpc_tiny.h
 @author  lukasz.forynski
 @date    06/08/2013
 @brief   Very lightweight implementation of JSON-RPC framework for C.
 ___________________________

 The MIT License (MIT)

 Copyright (c) 2013 Lukasz Forynski

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef JSON_RPC_TINY
#define JSON_RPC_TINY

/* Exported defines ------------------------------------------------------------*/

#include <stdint.h>
/* Exported types ------------------------------------------------------------*/

typedef struct json_rpc_request_data
{
    const char* request;
    char*       response;
    int         request_len;
    int         response_len;
    void*       arg;
} json_rpc_request_data_t;

enum request_info_flags
{
    rpc_request_is_notification = 1,
    rpc_request_is_rpc_20 = 2
};

typedef struct rpc_request_info
{
    int params_start;
    int params_len;
    int id_start;
    unsigned int info_flags; // TODO: add param to be passed to handlers ?
    json_rpc_request_data_t* data;
} rpc_request_info_t;

typedef struct rpc_param_info
{
    int value_start;
    int value_len;
}rpc_param_info_t;


/**
 * @brief  Definition of a handler type for RPC handlers.
 * @param rpc_request pointer to original JSON-RPC request string that was received.
 * @param info rpc_request_info with parsing results. This structure will be
 *        initialised to point at 'params', so:
 *        - params_start/params_len within rpc_request points at string containing parameters
 *          for this RPC call. Handler implementation should use params_start, params_len to extract
 *          those parameters as appropriate.
 * @return pointer to the JSON-RPC complaint response (if 'id' is > 0): request, or NULL (if id < 0): notification
 */
typedef char* (*json_rpc_handler_fcn)(rpc_request_info_t* info);

typedef struct json_rpc_handler
{
    json_rpc_handler_fcn handler;
    const char* fcn_name;
} json_rpc_handler_t;

typedef struct json_rpc_instance
{
    json_rpc_handler_t* handlers;
    int num_of_handlers;
    int max_num_of_handlers;
} json_rpc_instance_t;


enum json_rpc_20_errors
{
    json_rpc_err_parse_error = 0,       /* An error occurred on the server while parsing the JSON text */
    json_rpc_err_invalid_request,       /* The JSON sent is not a valid Request object */
    json_rpc_err_method_not_found,      /* The method does not exist / is not available */
    json_rpc_err_invalid_params,        /* Invalid method parameter(s) */
    json_rpc_err_internal_error,        /* Internal JSON-RPC error */
};


/* Exported functions ------------------------------------------------------- */
void json_rpc_init(json_rpc_instance_t* self, json_rpc_handler_t* table_for_handlers, int max_num_of_handlers);
void json_rpc_register_handler(json_rpc_instance_t* self, const char* fcn_name, json_rpc_handler_fcn handler);
char* json_rpc_handle_request(json_rpc_instance_t* self, json_rpc_request_data_t* request_data);
char* json_rpc_result(const char* result_str, rpc_request_info_t* info);
char* json_rpc_error(int err_code, rpc_request_info_t* info);
char* json_rpc_error(const char* err_msg, rpc_request_info_t* info);
void rpc_find_param_values(const char* param_name, rpc_param_info* param_info, rpc_request_info_t* info);

#endif /* JSON_RPC_TINY */
