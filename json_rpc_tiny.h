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


/**
 * @brief Structure defining request info. Object of such a structure
 *        is to be used to pass information about request/response buffers to
 *        to function handling the request.
 */
typedef struct json_rpc_request_data
{
    const char* request;
    char*       response;
    int         request_len;
    int         response_len;
    void*       arg;
} json_rpc_request_data_t;


/**
 * @brief Structure containing all information about the request.
 *        Pointer to such a structure will be passed to each handler, so that
 *        handlers could access all required request information (and pass
 *        this information to other extraction-aiding functions).
 */
typedef struct rpc_request_info
{
    int params_start;
    int params_len;
    int id_start;
    unsigned int info_flags; // TODO: add param to be passed to handlers ?
    json_rpc_request_data_t* data;
} rpc_request_info_t;


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


/**
 * @brief Structure used to define a storage for the service/function handler.
 *        It should be used to define storage for the JSON-RPC instance
 *        (table for handlers).
 */
typedef struct json_rpc_handler
{
    json_rpc_handler_fcn handler;
    const char* fcn_name;
} json_rpc_handler_t;


/**
 * @brief Struct defining and instance of the JSON-RPC handling entity.
 *        Number of different entities can be used (also from different threads),
 *        and this structure is used to define memory used by such an instance.
 */
typedef struct json_rpc_instance
{
    json_rpc_handler_t* handlers;
    int num_of_handlers;
    int max_num_of_handlers;
} json_rpc_instance_t;


/**
 * @brief Enumeration to allow selecting JSON-RPC2.0 error codes.
 */
enum json_rpc_20_errors
{
    json_rpc_err_parse_error = 0,       /* An error occurred on the server while parsing the JSON text */
    json_rpc_err_invalid_request,       /* The JSON sent is not a valid Request object */
    json_rpc_err_method_not_found,      /* The method does not exist / is not available */
    json_rpc_err_invalid_params,        /* Invalid method parameter(s) */
    json_rpc_err_internal_error,        /* Internal JSON-RPC error */
};


/* Exported functions ------------------------------------------------------- */

/**
 * @brief initialise rpc instance with a storage for handlers.
 * @param self pointer to the json_rpc_instance_t object.
 * @param table_for_handlers pointer to an allocated table that will hold handlers for this instance
 * @param max_num_of_handlers number of items above table can hold.
 */
void json_rpc_init(json_rpc_instance_t* self, json_rpc_handler_t* table_for_handlers, int max_num_of_handlers);


/**
 * @brief Registers a new handler.
 * @param self pointer to the json_rpc_instance_t object.
 * @param fcn_name name of the function (as it appears in RCP request).
 * @param handler pointer to the function handler (function of json_rpc_handler_fcn type).
 */
void json_rpc_register_handler(json_rpc_instance_t* self, const char* fcn_name, json_rpc_handler_fcn handler);


/**
 * @brief Method to handle RPC request. As a result, one of the registered handlers might be executed
 *        (if the function name from RFC request matches name for which a handler was registered).
 * @param self pointer to the json_rpc_instance_t object.
 * @param request_data pointer to a structure holding information about the request string,
 *        information where the resulting response is to be stored (if any), and additional information
 *        to be passed to the handler (see json_rpc_request_data_t for more info).
 * @return Pointer to buffer containing the response (the same buffer as passed in request_data).
 *         If the request was a notification only, this buffer will be empty.
 */
char* json_rpc_handle_request(json_rpc_instance_t* self, json_rpc_request_data_t* request_data);


/**
 * @brief Helper function to create an RPC response. It is designed to be used
 *        in the handler to create RCP response (in the response buffer).
 *        This function will either fill out the buffer with a valid response, or null-terminate
 *        it at it's beginning for notification, so there is no need to check that in the handler.
 *        This allows handlers to be implemented the same way for requests and notifications.
 * @param result_str string (null terminated!) to be copied to the response.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 */
char* json_rpc_result(const char* result_str, rpc_request_info_t* info);


/**
 * @brief Helper function to create an RPC error response. It is designed to be used
 *        in the handler to create RCP response (in the response buffer).
 *        This function will either fill out the buffer with a valid error response, or null-terminate
 *        it at it's beginning for notification, so there is no need to check that in the handler.
 *        This allows handlers to be implemented the same way for requests and notifications.
 * @param error_code - one of json_20_errors. Error will be constructed as per JSON_RPC 2.0 definitions.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 */
char* json_rpc_error(int err_code, rpc_request_info_t* info);


/**
 * @brief Helper function to create an RPC error response.  It is designed to be used
 *        in the handler to create RCP response (in the response buffer).
 *        This function will either fill out the buffer with a valid error response, or null-terminate
 *        it at it's beginning for notification, so there is no need to check that in the handler.
 *        This allows handlers to be implemented the same way for requests and notifications.
 * @param err_msg manually constructed error message string. It will be copied to the RPC response 'as is'.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 */
char* json_rpc_error(const char* err_msg, rpc_request_info_t* info);


/* Functions to aid extraction of RPC call parameters (by name and by order) */

/**
 * @brief Helper function to extract value of a named parameter as string.
 * @param param_name null-terminated name of the named parameter to extract.
 * @param str_length (out) Pointer to a variable where length of the string value will
 *        be stored if param is found and successfully extracted.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns pointer to a string (within the original request buffer) where the value of
 *          parameter starts, or NULL if parameter is not found.
 */
const char* rpc_extract_param_str(const char* param_name,  int* str_length, rpc_request_info_t* info);


/**
 * @brief Helper function to extract value of a named parameter as integer.
 *        It works similarly to the 'atoi', but works on the original request buffer
 *        and automatically detects and extracts octal or decimal values.
 * @param param_name null-terminated name of the named parameter to extract.
 * @param result (out) Pointer to a variable where extracted value will be
 *        be stored if param is found.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns non-zero if value was successfully extracted, zero-otherwise.
 */
int         rpc_extract_param_int(const char* param_name,  int* result,     rpc_request_info_t* info);


/**
 * @brief Helper function to extract value of a named parameter as string.
 * @param param_name null-terminated name of the named parameter to extract.
 * @param str_length (out) Pointer to a variable where length of the string value will
 *        be stored if param is found and successfully extracted.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns pointer to a string (within the original request buffer) where the value of
 *          parameter starts, or NULL if parameter is not found.
 */
const char* rpc_extract_param_str(int param_no_zero_based, int* str_length, rpc_request_info_t* info);

/**
 * @brief Helper function to extract value of a parameter as integer.
 *        It works similarly to the 'atoi', but works on the original request buffer
 *        and automatically detects and extracts octal or decimal values.
 * @param param_no_zero_based Number of the parameter to be converted (zero-based).
 * @param result (out) Pointer to a variable where extracted value will be
 *        be stored if param is found and successfully extracted.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns non-zero if value was successfully extracted, zero-otherwise.
 */
int         rpc_extract_param_int(int param_no_zero_based, int* result,     rpc_request_info_t* info);


#endif /* JSON_RPC_TINY */
