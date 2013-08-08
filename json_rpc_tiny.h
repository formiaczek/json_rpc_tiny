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
#define TINY_RPC_MAX_NUM_HANDLERS      16
#define TINY_RPC_MAX_FCN_NAME_LEN      16
#define TINY_RPC_MAX_RESPONSE_BUF_LEN  64

#ifndef RPC_VER
#define RPC_VER 2
#endif


#include <stdint.h>
/* Exported types ------------------------------------------------------------*/
typedef struct rpc_request_info
{
    int params_start;
    int params_len;
    int is_notification;
    int id_start;
} rpc_request_info_t;

typedef struct json_rpc_error_code
{
    const char* error_code;
    const char* error_msg;
} json_rpc_error_code_t;


extern json_rpc_error_code_t json_rpc_err_codes[];

enum json_rpc_errors
{
    json_rpc_parse_error = 0,       /* An error occurred on the server while parsing the JSON text */
    json_rpc_invalid_request,       /* The JSON sent is not a valid Request object */
    json_rpc_method_not_found,      /* The method does not exist / is not available */
    json_rpc_invalid_params,        /* Invalid method parameter(s) */
    json_rpc_internal_error,        /* Internal JSON-RPC error */
};

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
typedef char* (*json_rpc_handler_fcn)(const char* rpc_request, rpc_request_info_t* info);


/* Exported functions ------------------------------------------------------- */
void json_rpc_init();
void json_rpc_register_handler(const char* fcn_name, json_rpc_handler_fcn handler);
char* json_rpc_exec(const char* input, int input_len);
char* json_rpc_result(const char* result_str, const char* rpc_request, rpc_request_info_t* info);
char* json_rpc_error(int err_code, const char* rpc_request, rpc_request_info_t* info);


#endif /* JSON_RPC_TINY */
