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

#include <stdint.h>
#include <string.h>
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

typedef struct rpc_token_info
{
    int16_t obj_name_start;
    int16_t obj_name_end;
    int16_t values_start;
    int16_t values_end;
} rpc_token_info_t;

static json_rpc_handler_t obj_names[] =
{
    {0, "jsonrpc"},
    {0, "method"},
    {0, "params"},
    {0, "id"}
};

enum obj_name_ids
{
    jsonrpc = 0,
    method,
    params,
    request_id
};

static const int num_of_obj_names = sizeof(obj_names)/sizeof(obj_names[0]);


/* Private function declarations ------------------------------------------------------- */
static char* append_str(char* to, const char* from);
static int str_are_equal(const char* first, const char* second);
static int find_next_object(int start_from, const char* input, struct rpc_token_info* info);
static void reset_token_info(rpc_token_info_t* info);
static int get_obj_id(const char* input, rpc_token_info_t* info);
static int get_fcn_id(json_rpc_instance_t* self, const char* input, rpc_token_info_t* info);
static int name_to_id(const char* name, json_rpc_instance* table);


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

char* json_rpc_handle_request(json_rpc_instance_t* self, json_rpc_request_data_t* request_data)
{
    char* res = 0;
    rpc_token_info_t token_info;
    rpc_request_info_t request_info;

    int curr_pos = 0;
    int obj_id = -1;
    int fcn_id = -2;

    request_info.info_flags = rpc_request_is_notification; // assume it is notification
    request_info.id_start = -1;
    request_info.data = request_data;

    while(curr_pos < request_data->request_len && request_data->request[curr_pos] != '{')
    {
        curr_pos++; // move past any whitespace/quotes and other..
    }
    while(curr_pos < request_data->request_len)
    {
        curr_pos = find_next_object(curr_pos, request_data->request, &token_info);
        if (token_info.obj_name_start > 0)
        {
            obj_id = get_obj_id(request_data->request, &token_info);
            switch (obj_id)
            {
                case jsonrpc:
                    if(str_are_equal(request_data->request + token_info.values_start, "\"2.0\""))
                    {
                        request_info.info_flags |= rpc_request_is_rpc_20;
                    }
                    break;

                case method:
                    fcn_id = get_fcn_id(self, request_data->request, &token_info);
                    break;

                case params:
                    request_info.params_start = token_info.values_start;
                    request_info.params_len = token_info.values_end - token_info.values_start;
                    break;

                case request_id:
                    if(!str_are_equal(request_data->request + token_info.values_start, "none") &&
                       !str_are_equal(request_data->request + token_info.values_start, "null"))
                    {
                        request_info.info_flags &= ~rpc_request_is_notification;
                        request_info.id_start = token_info.obj_name_start; // mark start of the whole "id": ...
                    }
                    break;
            }
        }
    }

    if(fcn_id < 0)
    {
        if(fcn_id == -1)
        {
            res = json_rpc_error(json_rpc_err_method_not_found, &request_info);
        }
        else
        {
            res = json_rpc_error(json_rpc_err_invalid_request, &request_info);
        }
    }
    else if(request_info.params_start < 0)
    {
        res = json_rpc_error(json_rpc_err_invalid_request, &request_info);
    }
    else
    {
        res = self->handlers[fcn_id].handler(&request_info); // everything OK, can call a handler
    }

    return res;
}

char* json_rpc_result(const char* result_str, rpc_request_info_t* info)
{
    char* buf = info->data->response;
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
            buf = append_str(buf, ", ");
            buf = append_str(buf, info->data->request + info->id_start); // the rest of request, which is ID
        }
        else
        {
            buf = append_str(buf, "}");
        }
    }
    *buf = 0; // end of response (note, that it will be 0-length for notification)
    return info->data->response;
}

char* json_rpc_error(int err, rpc_request_info_t* info)
{
    char* buf = info->data->response;
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
        if(info->id_start > 0)
        {
            buf = append_str(buf, ", ");
            buf = append_str(buf, info->data->request + info->id_start); // the rest of request, which is ID
        }
        else
        {
            buf = append_str(buf, "}");
        }
    }
    *buf = 0; // end of response (note, that it will be 0-length for notification)
    return info->data->response;
}

char* json_rpc_error(const char* err_msg, rpc_request_info_t* info)
{
    char* buf = info->data->response;
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
            buf = append_str(buf, ", ");
            buf = append_str(buf, info->data->request + info->id_start); // the rest of request, which is ID
        }
        else
        {
            buf = append_str(buf, "}");
        }
    }
    *buf = 0; // end of response (note, that it will be 0-length for notification)
    return info->data->response;
}


/* Private functions ------------------------------------------------------- */

static char* append_str(char* to, const char* from)
{
    while(*from)
    {
        *to++ = *from++;
    }
    return to;
}

static int str_are_equal(const char* first, const char* second)
{
    int are_equal = 0;
    while (*second != 0   && // not the end of string?
           *first != 0  && // nor end of str
           *first == *second) // until are equal
    {
        first++;
        second++;
    }

    // here, if we're at the end of second = they match
    if (*second == 0)
    {
        are_equal = 1;
    }
    return are_equal;
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

        while (*curr_name != 0   && // not the end of string?
               *curr_name != '\"' && // end of word marked by quote
               *curr_item != 0  && // nor end of handler function name
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

static int get_fcn_id(json_rpc_instance_t* self, const char* input, rpc_token_info_t* info)
{
    const char* name = &input[info->values_start];
    return name_to_id(name, self);
}

static int get_obj_id(const char* input, rpc_token_info_t* info)
{
    json_rpc_instance table;
    table.handlers = (json_rpc_handler_t*)obj_names;
    table.num_of_handlers = num_of_obj_names;
    const char* name = &input[info->obj_name_start];
    return name_to_id(name, &table);
}

static void reset_token_info(rpc_token_info_t* info)
{
    info->obj_name_start = -1;
    info->obj_name_end = -1;
    info->values_start = -1;
    info->values_end = -1;
}

enum parsing_flags
{
    flag_none = 0,
    flag_in_array = 1,
    flag_in_object = 2
};

static int find_next_object(int start_from, const char* input, struct rpc_token_info* info)
{
    int curr_pos = start_from;
    int max_len = strlen(input);
    unsigned int flags = flag_none;
    char curr;
    reset_token_info(info);

    // find the object name (starts with ", ends with " and :
    while(curr_pos < max_len)
    {
        curr = input[curr_pos];
        if(curr == '\"')
        {
            if(info->obj_name_start < 0)
            {
                info->obj_name_start = curr_pos;
                curr_pos++;
                continue;
            }

            if(info->obj_name_end < 0)
            {
                curr_pos++;
                info->obj_name_end = curr_pos;
                break;
            }
        }
        curr_pos++;
    }

    // find value(s) for this object
    while(curr_pos < max_len)
    {
        curr = input[curr_pos];
        switch(curr)
        {
        case ':':
            while(input[++curr_pos] == ' '); // skip spaces
            if(!(flags & flag_in_object))
            {
                info->values_start = curr_pos;
            }
            continue;

        case '[':
            flags |= flag_in_array;
            curr_pos++;
            continue;

        case '{':
            flags |= flag_in_object;
            curr_pos++;
            continue;

        case ']':
            flags &= ~flag_in_array;
            curr_pos++;
            continue;

        case '}':
            flags &= ~flag_in_object;
            curr_pos++;
            continue;
        }

        if( flags == flag_none &&
            (curr == ',' ||
             curr == '}'))
        {
            info->values_end = curr_pos;
            curr_pos++;
            break;
        }
        curr_pos++;
    }
    return curr_pos;
}


