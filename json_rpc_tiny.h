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
 * @brief Structure defining rpc data info. Object of such a structure
 *        is to be used to pass information about request/response buffers.
 */
typedef struct json_rpc_data
{
    const char* request;
    char*       response;
    int         request_len;
    int         response_len;
    void*       arg;
} json_rpc_data_t;


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
    int id_len;
    unsigned int info_flags;
    json_rpc_data_t* data;
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
 * @brief Struct containing information about json token (json object).
 *        It is used to aid extraction / parsing of json objects.
 */
typedef struct json_token_info
{
    int16_t name_start;
    int16_t name_len;
    int16_t values_start;
    int16_t values_len;
} json_token_info_t;

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

/* RPC specific functions ---------------------------------------------- */

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
 *        to be passed to the handler (see json_rpc_data_t for more info).
 * @return Pointer to buffer containing the response (the same buffer as passed in request_data).
 *         If the request was a notification only, this buffer will be empty.
 */
char* json_rpc_handle_request(json_rpc_instance_t* self, json_rpc_data_t* request_data);


/**
 * @brief Function to create an RPC response. It is designed to be used
 *        in the handler to create RCP response (in the response buffer).
 *        This function will either fill out the buffer with a valid response, or null-terminate
 *        it at it's beginning for notification, so there is no need to check that in the handler.
 *        This allows handlers to be implemented the same way for requests and notifications.
 * @param result_str string (null terminated!) to be copied to the response.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 */
char* json_rpc_create_result(const char* result_str, rpc_request_info_t* info);


/**
 * @brief Function to create an RPC error response. It is designed to be used
 *        in the handler to create RCP response (in the response buffer).
 *        This function will either fill out the buffer with a valid error response, or null-terminate
 *        it at it's beginning for notification, so there is no need to check that in the handler.
 *        This allows handlers to be implemented the same way for requests and notifications.
 * @param error_code - one of json_20_errors. Error will be constructed as per JSON_RPC 2.0 definitions.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 */
char* json_rpc_create_error(int err_code, rpc_request_info_t* info);


/**
 * @brief Function to create an RPC error response.  It is designed to be used
 *        in the handler to create RCP response (in the response buffer).
 *        This function will either fill out the buffer with a valid error response, or null-terminate
 *        it at it's beginning for notification, so there is no need to check that in the handler.
 *        This allows handlers to be implemented the same way for requests and notifications.
 * @param err_msg manually constructed error message string. It will be copied to the RPC response 'as is'.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 */
char* json_rpc_create_error(const char* err_msg, rpc_request_info_t* info);


/* Functions to aid extraction of RPC call parameters (by name or order) */

/**
 * @brief Function to extract value of a named member (searching in the 'params' list) as string.
 *        It also can be used to extract parameters of any type (*integer parameters
 *        can be extracted using rpc_extract_param_int) and result can be converted appropriately.
 * @param param_name null-terminated name of the named parameter to extract.
 * @param str_length (out) Pointer to a variable where length of the string value will
 *        be stored if param is found and successfully extracted.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns pointer to a string (within the original request buffer) where the value of
 *          parameter starts, or NULL if parameter is not found.
 */
const char* rpc_extract_param_str(const char* param_name, int* str_length, rpc_request_info_t* info);


/**
 * @brief Function to extract value of a named member (searching in the 'params' list) as integer.
 *        It works similarly to the 'atoi', but works on the original request buffer
 *        and automatically detects and extracts negative, octal or decimal values.
 * @param param_name null-terminated name of the named parameter to extract.
 * @param result (out) Pointer to a variable where extracted value will be
 *        be stored if param is found.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns non-zero if value was successfully extracted, zero-otherwise.
 */
int rpc_extract_param_int(const char* param_name, int* result, rpc_request_info_t* info);


/**
 * @brief Function to extract value of a named parameter as string.
 *        It also can be used to extract parameters of any type (*integer parameters
 *        can be extracted using rpc_extract_param_int) and result can be converted appropriately.
 * @param member_no_zero_based zero-based member number.
 * @param str_length (out) Pointer to a variable where length of the string value will
 *        be stored if param is found and successfully extracted.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns pointer to a string (within the original request buffer) where the value of
 *          parameter starts, or NULL if parameter is not found.
 */
const char* rpc_extract_param_str(int member_no_zero_based, int* str_length, rpc_request_info_t* info);


/**
 * @brief Function to extract value of a parameter as integer.
 *        It works similarly to the 'atoi', but works on the original request buffer
 *        and automatically detects and extracts negative, octal or decimal values.
 * @param member_no_zero_based Number of the member to be converted (zero-based).
 * @param result (out) Pointer to a variable where extracted value will be
 *        be stored if param is found and successfully extracted.
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns non-zero if value was successfully extracted, zero-otherwise.
 */
int rpc_extract_param_int(int member_no_zero_based, int* result, rpc_request_info_t* info);


/* generic JSON extraction functions ---------------------------------------------- */

/**
 * @brief Function to find the first position within the next object in the parsed input.
 *        (i.e. to forward/move to the beginning of the next object).
 * @param start_from Offset at which parsing should start within the input.
 * @param input Input string.
 * @param input_len length of the input.
 * @returns new offset (position) at which next object has been found.
 *          If input does not contain any object,
 *          start_from is returned (as assumption that we're in the object already).
 */
int json_begining_of_next_object(int start_from, const char* input, int input_len);


/**
 * @brief Function to extract name and value of a next member within a json string.
 *        It is assumed that the input is null-terminated, and it's length will be calculated.
 * @param start_from offset at which parsing should start within the input.
 * @param input Input string.
 * @param info pointer to the json_token_info structure where results will be stored.
 *             As a result, token info will contain offset and length of a name (key) and value.
 *             If start_from was pointing at the beginning of an object (i.e. '{') or a list ('[')
 *             value start and length will select the whole object (or list), and name will be empty.
 *             Similarly - if a value itself was another object or list, values (start,len) will select
 *             this whole sub-object / list.
 * @returns new offset (position) at which parsing has been stopped during the search.
 */
int json_find_next_member(int start_from, const char* input, struct json_token_info* info);

/**
 * @brief Function to extract name and value of a next member within a json string.
 * @param start_from offset at which parsing should start within the input.
 * @param input Input string.
 * @param input_len length of the input.
 * @param info pointer to the json_token_info structure where results will be stored.
 * @returns new offset (position) at which parsing has been stopped during the search.
 */
int json_find_next_member(int start_from, const char* input, int input_len, struct json_token_info* info);


/**
 * @brief Function to extract the member of a given name (value) from the input containing JSON string.
 *        Object/list boundaries are crossed during the search (i.e. the whole JSON is inspected).
 * @param member_name zero-terminated name of the member to find.
 * @param str_length pointer to a variable where the members length is stored (if found).
 * @param input Input string.
 * @param input_len length of the input.
 * @returns on success: pointer set to the beginning of values of a member
 *               and str_length is updated with its size.
 *          on failure: NULL (and str_length is also set to 0).
 */
const char* json_extract_member_str(const char* member_name, int* str_length, const char* input, int input_len);


/**
 * @brief Function to extract the member of a given number (i.e. as ordered by number of members) (value).
 *        from the input containing JSON string.
 * @param member_no_zero_based zero-terminated number of the member to find in current object / list.
 * @param str_length pointer to a variable where length of value for this member is stored (if found).
 * @param input Input string.
 * @param input_len length of the input.
 * @returns on success: pointer set to the beginning of values of a member
 *               and str_length is updated with its size.
 *          on failure: NULL (and str_length is also set to 0).
 */
const char* json_extract_member_str(int member_no_zero_based, int* str_length, const char* input, int input_len);


/**
 * @brief Function to extract value of a named member as an integer.
 *        It works similarly to the 'atoi', but works on the original request buffer
 *        and automatically detects and extracts negative, octal or decimal values.
 *        It searches throughout the whole input JSON (passing any object/list boundaries).
 * @param member_name null-terminated name of the named member to extract.
 * @param result (out) Pointer to a variable where extracted value will be be stored (if found).
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns non-zero (bool) if value was successfully extracted, zero-otherwise.
 */
int json_extract_member_int(const char* member_name, int* result, const char* input, int input_len);


/**
 * @brief Function to extract value of an ordered member as an integer.
 *        It works similarly to the 'atoi', but works on the original request buffer
 *        and automatically detects and extracts negative, octal or decimal values.
 *        It searches throughout the highest-level JSON object in the given input
 * @param member_name null-terminated name of the named member to extract.
 * @param result (out) Pointer to a variable where extracted value will be be stored (if found).
 * @param info pointer to the rpc_request_info_t structure that was passed to the handler.
 * @returns non-zero (bool) if value was successfully extracted, zero-otherwise.
 */
int json_extract_member_int(int member_no_zero_based, int* result, const char* input, int input_len);

/**
 * @brief Function to check if member pointed by info token is an object.
 */
int json_next_member_is_object(const char* input, struct json_token_info* info);

/**
 * @brief Function to check if member pointed by info token is a list.
 */
int json_next_member_is_list(const char* input, struct json_token_info* info);

/**
 * @brief Function to check if member pointed by info token is an object.
 */
int json_next_member_is_object(const char* input, struct json_token_info* info);

/**
 * @brief Function to check if member pointed by info token is either object or list.
 */
int json_next_member_is_object_or_list(const char* input, struct json_token_info* info);




#endif /* JSON_RPC_TINY */
