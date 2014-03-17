/**
 @file    json_rpc_tiny.cpp
 @author  lukasz.forynski
 @date    06/08/2013
 @brief   Very lightweight implementation of JSON-RPC framework for C or C++.
          It allows registering handlers, supports request handling and does not perform
          any memory allocations. It can be used in embedded systems.
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

#include "json_rpc_tiny.h"


/* Private types and definitions ------------------------------------------------------- */

static const char* response_1x_prefix = "{";
static const char* response_20_prefix = "{\"jsonrpc\": \"2.0\", ";

typedef struct json_rpc_error_code
{
    const char* error_code;
    const char* error_msg;
} json_rpc_error_code_t;


extern json_rpc_error_code_t json_rpc_err_codes[];

json_rpc_error_code_t json_rpc_err_codes[] =
{
    {"-32700", "Parse error"     },   /* An error occurred on the server while parsing the JSON text */
    {"-32600", "Invalid Request" },   /* The JSON sent is not a valid Request object */
    {"-32601", "Method not found"},   /* The method does not exist / is not available */
    {"-32602", "Invalid params"  },   /* Invalid method parameter(s) */
    {"-32603", "Internal error"  }    /* Internal JSON-RPC error */
};

static json_rpc_handler_t obj_names[] =
{
    {0, "jsonrpc"},
    {0, "method"},
    {0, "params"},
    {0, "id"},
    {0, "result"},
    {0, "error"},

};

enum obj_name_ids
{
    jsonrpc = 0,
    method,
    params,
    request_id,
    the_result,
    the_error
};

enum request_info_flags
{
    rpc_request_is_notification = 1,
    rpc_request_is_rpc_20 = 2
};

static const int num_of_obj_names = sizeof(obj_names)/sizeof(obj_names[0]);


/* Private function declarations ------------------------------------------------------- */
static int str_len(const char* str);
static char* append_str(char* to, const char* from, int len = -1);
static int str_are_equal(const char* first, const char* second_zero_ended);
static int str_are_equal(const char* first, int first_len, const char* second_zero_ended);
static int int_val(char symbol, int* result);
static int convert_to_int(const char* start, int length, int* result);
static int json_find_member_value(int start_from, const char* input, struct json_token_info* info);
static void reset_token_info(json_token_info_t* info);
static int get_obj_id(const char* input, json_token_info_t* info);
static int get_fcn_id(json_rpc_instance_t* self, const char* input, json_token_info_t* info);
static int name_to_id(const char* name, json_rpc_instance* table);
static int skip_all_of(const char* input, int start_at, const char* values, char reversed);

/* Exported functions ------------------------------------------------------- */
void json_rpc_init(json_rpc_instance_t* self, json_rpc_handler_t* table_for_handlers, int max_num_of_handlers)
{
    int i;
    self->handlers = table_for_handlers;
    self->num_of_handlers = 0;
    self->max_num_of_handlers = max_num_of_handlers;

    for (i = 0; i < self->max_num_of_handlers; i++)
    {
        self->handlers[i].fcn_name = 0;
        self->handlers[i].handler = 0;
    }
}

void json_rpc_register_handler(json_rpc_instance_t* self, const char* fcn_name, json_rpc_handler_fcn handler)
{
    if (self->num_of_handlers < self->max_num_of_handlers)
    {
        if (fcn_name && handler)
        {
            self->handlers[self->num_of_handlers].fcn_name = fcn_name;
            self->handlers[self->num_of_handlers].handler = handler;
            self->num_of_handlers++;
        }
    }
}

char* json_rpc_handle_request(json_rpc_instance_t* self, json_rpc_data_t* request_data)
{
    char* res = 0;
    rpc_request_info_t request_info;
    json_token_info_t next_req_token;
    json_token_info_t next_mem_token;

    int curr_pos = 0;
    int next_r_pos = 0;

    int next_req_max_pos = 0;

    int obj_id = -1;
    int fcn_id = -2;

    request_info.data = request_data;
    if(request_data->response && request_data->response_len)
    {
        *request_data->response = 0; // null
    }

    next_r_pos = skip_all_of(request_data->request, 0, " \n\r\t", 0);

    reset_token_info(&next_req_token);
    next_req_token.values_start = next_r_pos;
    next_req_token.values_len = request_data->request_len-next_r_pos;

    // skip [] bracket for a batch..
    if(json_next_member_is_list(request_data->request, &next_req_token))
    {
        next_r_pos = skip_all_of(request_data->request, next_r_pos+1, " \n\r\t", 0);
        if(request_data->response && request_data->response_len)
        {
            append_str(request_data->response, "[");
        }
    }

    while(next_r_pos < request_data->request_len)
    {
        // reset some of the request info data
        request_info.id_start = -1;
        request_info.info_flags = 0;
        fcn_id = -2;
        obj_id = -1;


        // extract next request (there can be a batch of them)
        next_r_pos = json_find_next_member(next_r_pos, request_data->request, request_data->request_len, &next_req_token);
        if(json_next_member_is_object(request_data->request, &next_req_token))
        {
            curr_pos = next_req_token.values_start+1; // move past this next object
        }
        else
        {
            curr_pos = next_req_token.values_start;
        }

        // skip the whitespace
        curr_pos = skip_all_of(request_data->request, curr_pos, " \n\r\t", 0);

        next_req_max_pos = next_req_token.values_start+next_req_token.values_len;
        while(curr_pos < next_req_max_pos)
        {
            curr_pos = json_find_next_member(curr_pos,
                                             request_data->request,
                                             next_req_max_pos, &next_mem_token);

            if (next_mem_token.name_start > 0)
            {
                obj_id = get_obj_id(request_data->request, &next_mem_token);
                switch (obj_id)
                {
                    case jsonrpc:
                        if(str_are_equal(request_data->request + next_mem_token.values_start, "2.0"))
                        {
                            request_info.info_flags |= rpc_request_is_rpc_20;
                        }
                        break;

                    case method:
                        fcn_id = get_fcn_id(self, request_data->request, &next_mem_token);
                        if(fcn_id >= 0)
                        {
                            request_info.info_flags |= rpc_request_is_notification; // assume it is notification
                        }
                        break;

                    case params:
                        request_info.params_start = next_mem_token.values_start;
                        request_info.params_len = next_mem_token.values_len;
                        break;

                    case request_id:
                        if(!str_are_equal(request_data->request + next_mem_token.values_start, "none") &&
                           !str_are_equal(request_data->request + next_mem_token.values_start, "null"))
                        {
                            request_info.info_flags &= ~rpc_request_is_notification;
                            request_info.id_start = next_mem_token.values_start;
                            request_info.id_len = next_mem_token.values_len;
                        }
                        break;
                }
            }
            else
            {
                break;
            }
        }

        if(fcn_id < 0)
        {
            if(fcn_id == -1)
            {
                res = json_rpc_create_error(json_rpc_err_method_not_found, &request_info);
            }
            else
            {
                res = json_rpc_create_error(json_rpc_err_invalid_request, &request_info);
            }
        }
        else if(request_info.params_start < 0)
        {
            res = json_rpc_create_error(json_rpc_err_invalid_request, &request_info);
        }
        else
        {
            res = self->handlers[fcn_id].handler(&request_info); // everything OK, can call a handler
        }
    }

    if(request_data->response && request_data->response_len &&
       request_data->response[0] == '[')
    {
        append_str(request_data->response+str_len(request_data->response), "]");
    }

    return res;
}


const char* rpc_extract_param_str(const char* param_name, int* str_length, rpc_request_info_t* info)
{
    return json_extract_member_str(param_name, // just find a member, but narrow-down the search to params
                                   str_length,
                                   info->data->request+info->params_start,
                                   info->params_len);
}


int rpc_extract_param_int(const char* param_name, int* result, rpc_request_info_t* info)
{
    return json_extract_member_int(param_name,
                                   result,
                                   info->data->request+info->params_start,
                                   info->params_len);
}


const char* rpc_extract_param_str(int member_no_zero_based, int* str_length, rpc_request_info_t* info)
{
    return json_extract_member_str(member_no_zero_based, str_length,
                                   info->data->request + info->params_start,
                                   info->params_len);
}

int rpc_extract_param_int(int member_no_zero_based, int* result, rpc_request_info_t* info)
{
    return json_extract_member_int(member_no_zero_based,
                                   result,
                                   info->data->request+info->params_start,
                                   info->params_len);
}

char* json_rpc_create_result(const char* result_str, rpc_request_info_t* info)
{
    char* buf;
    if(!info->data->response_len || !info->data->response) // if no space nor response, return..
    {
        return info->data->response;
    }

    buf = info->data->response + str_len(info->data->response);
    if(buf - info->data->response > 2) // not the beginning of a batch response
    {
        buf = append_str(buf, ", ", 2);
    }

    if(!(info->info_flags & rpc_request_is_notification))
    {
        if(info->info_flags & rpc_request_is_rpc_20)
        {
            buf = append_str(buf, response_20_prefix);
        }
        else
        {
            buf = append_str(buf, response_1x_prefix);
        }

        buf = append_str(buf, "\"result\": ");
        buf = append_str(buf, result_str);
        if(!(info->info_flags & rpc_request_is_rpc_20))
        {
            buf = append_str(buf, ", \"error\": none");
        }
        if(info->id_start > 0)
        {
            buf = append_str(buf, ", \"id\": ");
            buf = append_str(buf, info->data->request + info->id_start, info->id_len);
        }
        buf = append_str(buf, "}");
    }
    return info->data->response;
}

char* json_rpc_create_error(int err, rpc_request_info_t* info)
{
    char* buf;
    if(!info->data->response_len || !info->data->response) // if no space nor response, return..
    {
        return info->data->response;
    }

    buf = info->data->response + str_len(info->data->response);
    if(buf - info->data->response > 2) // not the beginning of a batch response
    {
        buf = append_str(buf, ", ", 2);
    }

    if(!(info->info_flags & rpc_request_is_notification))
    {
        if(info->info_flags & rpc_request_is_rpc_20)
        {
            buf = append_str(buf, response_20_prefix);
        }
        else
        {
            buf = append_str(buf, response_1x_prefix);
        }
        buf = append_str(buf, "\"error\": {\"code\": ");
        buf = append_str(buf, json_rpc_err_codes[err].error_code);
        buf = append_str(buf, ", \"message\": \"");
        buf = append_str(buf, json_rpc_err_codes[err].error_msg);
        buf = append_str(buf, "\"}");
        if(info->id_start > 0 || err == json_rpc_err_invalid_request)
        {
            buf = append_str(buf, ", \"id\": ");
            if(info->id_start > 0)
            {
                buf = append_str(buf, info->data->request + info->id_start, info->id_len);
            }
            else
            {
                buf = append_str(buf, "none");
            }
        }
        buf = append_str(buf, "}");
    }
    return info->data->response;
}

char* json_rpc_create_error(const char* err_msg, rpc_request_info_t* info)
{
    char* buf;
    if(!info->data->response_len || !info->data->response) // if no space nor response, return..
    {
        return info->data->response;
    }

    buf = info->data->response + str_len(info->data->response);
    if(buf - info->data->response > 2) // not the beginning of a batch response
    {
        buf = append_str(buf, ", ", 2);
    }

    if(!(info->info_flags & rpc_request_is_notification))
    {
        if(info->info_flags & rpc_request_is_rpc_20)
        {
            buf = append_str(buf, response_20_prefix);
        }
        else
        {
            buf = append_str(buf, response_1x_prefix);
        }
        buf = append_str(buf, "\"error\": ");
        buf = append_str(buf, err_msg);
        if(info->id_start > 0)
        {
            buf = append_str(buf, ", \"id\": ");
            buf = append_str(buf, info->data->request + info->id_start, info->id_len);
        }
        buf = append_str(buf, "}");
    }
    return info->data->response;
}

int json_begining_of_next_object(int start_from, const char* input, int input_len)
{
    int next_obj_start = start_from;
    while(start_from < input_len)
    {
        if(input[start_from] == '{' || input[start_from] == '[')
        {
            next_obj_start = start_from;
            break;
        }
        start_from++;
    }
    return next_obj_start;
}

int json_find_next_member(int start_from, const char* input, struct json_token_info* info)
{
    return json_find_next_member(start_from, input, str_len(input), info);
}

int json_find_next_member(int start_from, const char* input, int input_len, struct json_token_info* info)
{
    int curr_pos = start_from;
    reset_token_info(info);
    if(curr_pos >= input_len)
    {
        return input_len;
    }

    curr_pos = skip_all_of(input, curr_pos, " \n\r\t", 0);
    if(curr_pos >= input_len)
    {
        return input_len;
    }

    if(input[curr_pos] != '{' && input[curr_pos] != '[') // if it's an object, get it as a value
    {
        start_from = curr_pos; // re-use start_from variable
        while(start_from < input_len)
        {
            start_from++;
            if(input[start_from] == ':')
            {
                // ok, found member name (a.k.a key)
                info->name_start = curr_pos; // assume start was beginning found above
                info->name_start = skip_all_of(input, info->name_start, "\"", 0); // strip begin
                curr_pos = start_from + 1;   // move curr past what we've parsed already
                start_from = skip_all_of(input, start_from, " :\"", 1); // strip end
                info->name_len = start_from - info->name_start + 1;
                break;
            }
            if(input[start_from] == ',') // also skip if it appears to be ordered param
            {
                break;
            }
        }
    }
    return json_find_member_value(curr_pos, input, info);
}

const char* json_extract_member_str(const char* member_name, int* str_length, const char* input, int input_len)
{
    const char* result = 0;
    json_token_info_t token_info;
    int curr_pos = 0;
    reset_token_info(&token_info);
    *str_length = 0; // on error - return 0 as length (default)

    while(true)
    {
        curr_pos = json_find_next_member(curr_pos,
                                         input,
                                         input_len,
                                         &token_info);
        if(!token_info.values_len)
        {
            break;
        }
        else
        {
            if(str_are_equal(input+token_info.name_start, token_info.name_len, member_name))
            {
                *str_length = token_info.values_len;
                result = input + token_info.values_start;
                break;
            }
        }

        if(json_next_member_is_object_or_list(input, &token_info))
        {
            curr_pos = token_info.values_start+1; // move past objects/list boundaries
        }
    };
    return result;
}

const char* json_extract_member_str(int member_no_zero_based, int* str_length, const char* input, int input_len)
{
    const char* result = 0;
    json_token_info_t token_info;
    int curr_param_no = 0;
    int curr_pos = 0;

    *str_length = 0; // on error - return 0 as length (default)

    reset_token_info(&token_info);
    token_info.values_len = input_len;
    if(json_next_member_is_object_or_list(input, &token_info)) // if it's object or list, enter..
    {
        curr_pos = token_info.values_start+1; // move past the list begin
    }

    while(true)
    {
        curr_pos = json_find_next_member(curr_pos,
                                         input,
                                         input_len,
                                         &token_info);
        if(!token_info.values_len)
        {
            break;
        }
        else
        {
            if(curr_param_no == member_no_zero_based)
            {
                *str_length = token_info.values_len;
                result = input + token_info.values_start;
                break;
            }
            curr_param_no++;
        }
    };
    return result;
}

int json_extract_member_int(const char* member_name, int* result, const char* input, int input_len)
{
    int extracted_ok = 0;
    int result_str_len = 0;
    const char* p  = json_extract_member_str(member_name, &result_str_len, input, input_len);
    if(p && result_str_len)
    {
        if(convert_to_int(p, result_str_len, result))
        {
            extracted_ok = 1;
        }
    }
    return extracted_ok;
}

int json_extract_member_int(int member_no_zero_based, int* result, const char* input, int input_len)
{
    int extracted_ok = 0;
    int result_str_len = 0;
    const char* p  = json_extract_member_str(member_no_zero_based, &result_str_len, input, input_len);
    if(p && result_str_len)
    {
        if(convert_to_int(p, result_str_len, result))
        {
            extracted_ok = 1;
        }
    }
    return extracted_ok;
}

int json_next_member_is_object(const char* input, struct json_token_info* info)
{
    int is_object = 0;
    if(info->values_len > 0)
    {
        if(input[info->values_start] == '{' &&
           input[info->values_start+info->values_len-1] == '}')
        {
            is_object = 1;
        }
    }
    return is_object;
}

int json_next_member_is_list(const char* input, struct json_token_info* info)
{
    int is_list = 0;
    if(info->values_len > 0)
    {
        if(input[info->values_start] == '[' &&
           input[info->values_start+info->values_len-1] == ']')
        {
            is_list = 1;
        }
    }
    return is_list;
}

int json_next_member_is_object_or_list(const char* input, struct json_token_info* info)
{
    int is_object = 0;
    if(info->values_len > 0)
    {
        if( (input[info->values_start] == '{' &&
             input[info->values_start+info->values_len-1] == '}')  ||
             (input[info->values_start] == '[' &&
             input[info->values_start+info->values_len-1] == ']') )
        {
            is_object = 1;
        }
    }
    return is_object;
}

/* Private functions ------------------------------------------------------- */
static int json_find_member_value(int start_from, const char* input, struct json_token_info* info)
{
    int curr_pos = start_from;
    int max_len = str_len(input);

    int in_quotes = 0;
    int in_object = 0;
    int in_array = 0;
    char curr;
    int values_end = 0;

    curr_pos = skip_all_of(input, curr_pos, "\n\r\t :", 0); // whitespace & colon: we'll be searching for a value
    info->values_start = curr_pos;

    // find value(s) for this object
    while(curr_pos < max_len)
    {
        curr = input[curr_pos];
        switch(curr)
        {
        case '\"':
            in_quotes = ~in_quotes;
            curr_pos++;
            continue;

        case '[':
            if(!in_quotes)
            {
                in_array++;
            }
            curr_pos++;
            continue;

        case '{':
            if(!in_quotes)
            {
                in_object++;
            }
            curr_pos++;
            continue;

        case ']':
            curr_pos++;
            if(!in_quotes)
            {
                in_array--;
                if(in_array <= 0)
                {
                    if(!in_object)
                    {
                        values_end = curr_pos;
                        break;
                    }
                }
            }
            continue;

        case '}':
            curr_pos++;
            if(!in_quotes)
            {
                in_object--;
                if(in_object <= 0)
                {
                    if(!in_array)
                    {
                        values_end = curr_pos;
                        break;
                    }
                }
            }
            continue;
        }

        if(values_end > 0 ||
          ( curr == ',' && !in_object && !in_array && !in_quotes ))
        {
             // in_object or in_array will have negative values if a whole object and / or array
             // was extracted, so adjust for this
             values_end = curr_pos + in_object + in_array;
             curr_pos++;
             break;
        }
        curr_pos++;
    }

    if(input[info->values_start] == '\"' && input[values_end-1] == '\"')
    {
        info->values_start++;
        values_end--;
    }
    else
    {
        info->values_start = skip_all_of(input, info->values_start, " \t\n\r", 0);
        values_end = skip_all_of(input, values_end, " \t\n\r", 1);
    }

    info->values_len = (values_end >= info->values_start) ? values_end - info->values_start : 0;
    return curr_pos;
}

static int str_len(const char* str)
{
    // yes yes, the unsafe str_len.. hopefully we'll use it wisely ;)
    int i = 0;
    while(*str++)
    {
        i++;
    }
    return i;
}

static char* append_str(char* to, const char* from, int len/* = -1*/)
{
    if(len > 0)
    {
        while(len--)
        {
            *to++ = *from++;
        }
    }
    else
    {
        while(*from)
        {
            *to++ = *from++;
        }
    }
    *to = 0;
    return to;
}

static int str_are_equal(const char* first, const char* second_zero_ended)
{
    int are_equal = 0;
    while (*second_zero_ended != 0   &&  // not the end of string?
           *first != 0  &&               // nor end of str
           *first == *second_zero_ended) // until are equal
    {
        first++;
        second_zero_ended++;
    }

    // here, if we're at the end of second = they do match
    if (*second_zero_ended == 0)
    {
        are_equal = 1;
    }
    return are_equal;
}

static int str_are_equal(const char* first, int first_len, const char* second_zero_ended)
{
    int are_equal = 0;
    while (*second_zero_ended != 0   &&  // not the end of string?
           *first != 0  &&    // nor end of str
           *first == *second_zero_ended) // until are equal
    {
        first++;
        second_zero_ended++;
        first_len--;
    }

    // here, if we're at the end of first and second they do match at least at first_len characters.
    if (*second_zero_ended == 0 && first_len == 0)
    {
        are_equal = 1;
    }
    return are_equal;
}

static int int_val(char symbol, int* result)
{
    int value_ok = 1;
    int correct_by = 0;

    if ((symbol >= '0') && (symbol <= '9'))
    {
        correct_by = '0';
    }
    else if ((symbol >= 'A') && (symbol <= 'F'))
    {
        correct_by = ('A' - 10);
    }
    else if ((symbol >= 'a') && (symbol <= 'f'))
    {
        correct_by = ('a' - 10);
    }
    else
    {
        value_ok = 0;
    }

    *result = symbol - correct_by;
    return value_ok;
}

static int convert_to_int(const char* start, int length, int* result)
{
    int extracted_ok = 1;
    int base = 10;
    int value = 0;
    int sign = 1;
    int val_start = 0;
    int i = 0;

    if(length > 0 && start[0] == '-')
    {
        sign = -1;
        val_start++;
    }

    // find base: 0x: hex(16), 0:octal(8)
    if(length > val_start+2 &&
       start[val_start] == '0')
    {
        if(start[val_start+1] == 'x')
        {
            base = 16;
            val_start += 2;
        }
        else
        {
            base = 8;
            val_start += 1;
        }
    }

    *result = 0;
    for(i = val_start; i < length; i++)
    {
        if(i > val_start)
        {
            *result *= base;
        }

        if(!int_val(start[i], &value) ||
           value > base)
        {
            extracted_ok = 0;
            break;
        }
        *result += value;
    }
    *result *= sign;
    return extracted_ok;
}

static int name_to_id(const char* name, json_rpc_instance* table)
{
    const char* curr_name = name;
    const char* curr_item;
    int i;
    for (i = 0; i < table->num_of_handlers; i++)
    {
        curr_name = name;
        while(*curr_name == '\"')
        {
            curr_name++; // skip opening quotes
        }

        curr_item = table->handlers[i].fcn_name;

        while (*curr_name != 0   &&  // not the end of string?
               *curr_name != '\"' && // end of word marked by quote
               *curr_item != 0  &&   // nor end of handler function name
               *curr_name == *curr_item) // until are equal
        {
            curr_name++;
            curr_item++;
        }

        // here, if we're at the end of either curr_: we've found it!
        if (*curr_item == 0 && (*curr_name == 0 || *curr_name == '\"'))
        {
            break;
        }
    }
    return i == table->num_of_handlers ? -1 : i; // if not found, return -1
}

static int get_fcn_id(json_rpc_instance_t* self, const char* input, json_token_info_t* info)
{
    const char* name = &input[info->values_start];
    return name_to_id(name, self);
}

static int get_obj_id(const char* input, json_token_info_t* info)
{
    json_rpc_instance table;
    table.handlers = (json_rpc_handler_t*)obj_names;
    table.num_of_handlers = num_of_obj_names;
    const char* name = &input[info->name_start];
    return name_to_id(name, &table);
}

static void reset_token_info(json_token_info_t* info)
{
    info->name_start = 0;
    info->name_len = 0;
    info->values_start = 0;
    info->values_len = 0;
}

static int skip_all_of(const char* input, int start_at, const char* values, char reversed)
{
    int size = str_len(values);
    int i = 0;
    char found = 0;
    do
    {
        for(i = 0; i < size; i++)
        {
            found = 0;
            if(input[start_at] == values[i])
            {
                if(reversed)
                {
                    --start_at;
                    if(start_at < 0)
                    {
                        return start_at;
                    }
                }
                else
                {
                    ++start_at;
                }
                found = 1;
                break;
            }
        }
    }
    while(found);
    return start_at;
}
