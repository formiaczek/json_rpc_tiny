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

static const char* response_prefix = "{\"jsonrpc\": \"2.0\", ";

json_rpc_error_code_t json_rpc_err_codes[] =
{
    {"-32700", "Parse error"     },   /* An error occurred on the server while parsing the JSON text */
    {"-32600", "Invalid Request" },   /* The JSON sent is not a valid Request object */
    {"-32601", "Method not found"},   /* The method does not exist / is not available */
    {"-32602", "Invalid params"  },   /* Invalid method parameter(s) */
    {"-32603", "Internal error"  }    /* Internal JSON-RPC error */
};

typedef struct
{
    json_rpc_handler_fcn handler;
    const char* fcn_name;
} handler_t;

static handler_t handlers[TINY_RPC_MAX_NUM_HANDLERS];
static int number_of_handlers = 0;

typedef struct rpc_token_info
{
    int16_t obj_name_start;
    int16_t obj_name_end;
    int16_t values_start;
    int16_t values_end;
} rpc_token_info_t;

static handler_t obj_names[] =
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

typedef struct
{
    handler_t* table;
    int number_of_items;
} search_table_t;

static char response_buf[TINY_RPC_MAX_RESPONSE_BUF_LEN];


/* Private function declarations ------------------------------------------------------- */
static char* append_str(char* to, const char* from);
static int str_are_equal(const char* first, const char* second);
static int find_next_object(int start_from, const char* input, struct rpc_token_info* info);
static void reset_token_info(struct rpc_token_info* info);
static int get_obj_id(const char* input, rpc_token_info_t* info);
static int get_fcn_id(const char* input, rpc_token_info_t* info);
static int name_to_id(const char* name, search_table_t* table);


/* Exported functions ------------------------------------------------------- */
void json_rpc_init()
{
  int i;
  for (i = 0; i < TINY_RPC_MAX_NUM_HANDLERS; i++)
  {
    handlers[i].fcn_name = 0;
    handlers[i].handler = 0;
  }
}

void json_rpc_register_handler(const char* fcn_name, json_rpc_handler_fcn handler)
{
  if(number_of_handlers < TINY_RPC_MAX_NUM_HANDLERS)
  {
    if(fcn_name && handler)
    {
      handlers[number_of_handlers].fcn_name = fcn_name;
      handlers[number_of_handlers].handler = handler;
      number_of_handlers++;
    }
  }
}

char* json_rpc_exec(const char* input, int input_len)
{
    char* res = 0;
    rpc_token_info_t info;
    rpc_request_info_t request_info;
    int curr_pos = 0;
    int obj_id;
    int fcn_id;

    request_info.is_notification = 1; // assume ID is not valid (by default)
    request_info.id_start = -1;

    while(curr_pos < input_len)
    {
        curr_pos = find_next_object(curr_pos, input, &info);
        if (info.obj_name_start > 0)
        {
            obj_id = get_obj_id(input, &info);
            switch (obj_id)
            {
                case method:
                    fcn_id = get_fcn_id(input, &info);
                    break;
                case params:
                    request_info.params_start = info.values_start;
                    request_info.params_len = info.values_end - info.values_start;
                    break;
                case request_id:
                    if(!str_are_equal(input + info.values_start, "none") ||
                       !str_are_equal(input + info.values_start, "null"))
                    {
                        request_info.is_notification = 0;
                        request_info.id_start = info.obj_name_start; // mark start of the whole "id": ...
                    }
                    break;
            }
        }
    }

    if(fcn_id < 0)
    {
        res = json_rpc_error(json_rpc_method_not_found, input, &request_info);
    }
    else if(request_info.params_start < 0)
    {
        res = json_rpc_error(json_rpc_invalid_request, input, &request_info);
    }
    else
    {
        res = handlers[fcn_id].handler(input, &request_info); // everything OK, can call a handler
    }

    return res;
}

char* json_rpc_result(const char* result_str, const char* rpc_request, rpc_request_info_t* info)
{
    char* buf = response_buf;
    if(!info->is_notification)
    {
        buf = append_str(buf, response_prefix);
        buf = append_str(buf, "\"result\": ");
        buf = append_str(buf, result_str);
        if(info->id_start > 0)
        {
            buf = append_str(buf, ", ");
            buf = append_str(buf, rpc_request + info->id_start); // the rest of request, which is ID
        }
        else
        {
            buf = append_str(buf, "}");
        }
    }
    *buf = 0; // end of response (note, that it will be 0-length for notification)
    return response_buf;
}

char* json_rpc_error(int err, const char* rpc_request, rpc_request_info_t* info)
{
    char* buf = response_buf;
    if(!info->is_notification)
    {
        buf = append_str(buf, response_prefix);
        buf = append_str(buf, "\"error\": {\"code\": ");
        buf = append_str(buf, json_rpc_err_codes[err].error_code);
        buf = append_str(buf, ", \"message\": \"");
        buf = append_str(buf, json_rpc_err_codes[err].error_msg);
        buf = append_str(buf, "\"}");
        if(info->id_start > 0)
        {
            buf = append_str(buf, ", ");
            buf = append_str(buf, rpc_request + info->id_start); // the rest of request, which is ID
        }
        else
        {
            buf = append_str(buf, "}");
        }
    }
    *buf = 0; // end of response (note, tht it will be 0-length for notification)
    return response_buf;
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


static int name_to_id(const char* name, search_table_t* table)
{
    const char* curr_name = name;
    const char* curr_item;
    int i;
    for (i = 0; i < table->number_of_items; i++)
    {
        curr_name = name;
        while(*curr_name == '\"')
            {
            curr_name++; // skip opening quotes
            }

        curr_item = table->table[i].fcn_name;

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
    return i == table->number_of_items ? -1 : i; // if not found, return -1
}

static int get_fcn_id(const char* input, rpc_token_info_t* info)
{
    search_table_t table;
    table.table = (handler_t*)handlers;
    table.number_of_items = number_of_handlers;
    const char* name = &input[info->values_start];
    return name_to_id(name, &table);
}

static int get_obj_id(const char* input, rpc_token_info_t* info)
{
    search_table_t table;
    table.table = (handler_t*)obj_names;
    table.number_of_items = num_of_obj_names;
    const char* name = &input[info->obj_name_start];
    return name_to_id(name, &table);
}

static void reset_token_info(struct rpc_token_info* info)
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


