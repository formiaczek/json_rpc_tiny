/*
 * main.cpp
 *
 *  Created on: 11 Feb 2013
 *      Author: lukasz.forynski
 */

#include "json_rpc_tiny.h"


#include <string.h>

#include <iostream>
#include <sstream>
#include <ctime>

#include <stdio.h>

char* handleMessage(rpc_request_info_t* info)
{
    // input (request) string is in {info->data->request, info->data->request_len}
    // information about that data is already parsed, and
    // params_start is offset within the request to the beginning of string conraining
    // parameters. info->params_len contains length of this string.
    char params[64];
    strncpy(params, info->data->request + info->params_start, info->params_len);
    params[info->params_len] = 0;

    printf(" ===> called handleMessage(%s, notif: %d)\n\n", params, info->info_flags);

    // note : don't change response directly: using json_rpc_result() or json_rpc_error()
    // you can update response data with required data (and JSON tags will be appended
    // automatically
    return json_rpc_result("\"OK\"", info);
}

char* getTimeDate(rpc_request_info_t* info)
{
    // (no need to parse arguments here)
    std::stringstream res;
    time_t curr_time;
    time(&curr_time);
    struct tm * now = localtime(&curr_time);
    res << "\""
         << (now->tm_year + 1900) << '-'
         << (now->tm_mon + 1) << '-'
         <<  now->tm_mday
         << "\"";

    return json_rpc_result(res.str().c_str(), info);
}

char* search(rpc_request_info_t* info)
{
    // could do it in 'C' (a.k.a.: hacking)
    char* res = 0;
    char params[64];
    strncpy(params, info->data->request + info->params_start,
            info->params_len);
    params[info->params_len] = 0;

    char param_name[16], param_value[16];

    printf("\n ===> called\n  search(%s, notif: %d)\n  i.e.:\n",
            params, info->info_flags & rpc_request_is_notification);

    sscanf(params, "%*[^\"]\"%[^\"]\"%*[^\"]\"%[^\"]\"", param_name, param_value);
    printf("  search(%s=%s)\n", param_name, param_value);

    if(strcmp(param_name, "last_name"))
    {
        // return json_rpc_error (using error value) on failure
        res = json_rpc_error(json_rpc_err_invalid_params, info);
    }
    else
    {
        if(!strcmp(param_value, "Python"))
        {
            res = json_rpc_result("\"Monty\"", info);
        }
        else
        {
            res = json_rpc_result("none", info);
        }
    }
    return res;
}

char* non_20_error_example(rpc_request_info_t* info)
{
    int error_occured = 1;
    std::stringstream msg;
    char* res = 0;
        // ...

    if(error_occured)
    {
        // manually construct the error code..
        msg << "{\"name\": \"some_error\", ";
        msg << "\"message\": \"SOmething went wrong..\"}";
        res = json_rpc_error(msg.str().c_str(), info); // and past it as a string
    }
    else
    {
        res = json_rpc_result("\"OK\"", info);
    }
    return res;
}

char* use_param(rpc_request_info_t* info)
{
    std::stringstream msg;
    const char* param = (const char*)info->data->arg;
    char* res = 0;
    if(param)
    {
        msg << "\""<< param << "\"";
        res = json_rpc_result(msg.str().c_str(), info);
    }
    else
    {
        res = json_rpc_error(json_rpc_err_internal_error, info);
    }
    return res;
}

const char* example_requests[] =
{
 "{\"method\": \"handleMessage\", \"params\": [\"user3\", \"sorry, gotta go now, ttyl\"], \"id\": null}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"getTimeDate\", \"params\": none, \"id\": 123}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"helloWorld\", \"params\": [\"Hello World\"], \"id\": 32}",
 "{\"method\": \"search\", \"params\": [{\"last_name\": \"Python\"}], \"id\": 32}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"search\", \"params\": [{\"last_n\": \"Python\"}], \"id\": 33}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"search\", \"params\": [{\"last_name\": \"Doe\"}], \"id\": 34}",
 "{\"jsonrpc\": \"2.0\", \"thod\": \"search\", ", // not valid
 "{\"method\": \"err_example\",  \"params\": [], \"id\": 35}", // not valid
 "{\"jsonrpc\": \"2.0\", \"method\": \"use_param\", \"params\": [], \"id\": 36s}",
};
const int num_of_examples = sizeof(example_requests)/sizeof(char*);

// define our storage:
#define MAX_NUM_OF_HANDLERS 32
json_rpc_handler_t storage_for_handlers[MAX_NUM_OF_HANDLERS];

#define RESPONSE_BUF_MAX_LEN  128
char response_buffer[RESPONSE_BUF_MAX_LEN];


int main(int argc, char **argv)
{
    // create and initialise instance
    json_rpc_instance rpc;
    json_rpc_init(&rpc, storage_for_handlers, MAX_NUM_OF_HANDLERS);

    // register our handlers
    json_rpc_register_handler(&rpc, "handleMessage", handleMessage);
    json_rpc_register_handler(&rpc, "getTimeDate", getTimeDate);
    json_rpc_register_handler(&rpc, "search", search);
    json_rpc_register_handler(&rpc, "err_example", non_20_error_example);
    json_rpc_register_handler(&rpc, "use_param", use_param);

    // prepare and initialise request data
    json_rpc_request_data_t req;
    req.response = response_buffer;
    req.response_len = RESPONSE_BUF_MAX_LEN;
    req.arg = argv[0];

    // now execute try it out with example requests defined above

    for(int i = 0; i < num_of_examples; i++)
    {
        req.request = example_requests[i];
        req.request_len = strlen(example_requests[i]);
        std::cout << "\n--> " << example_requests[i] << "\n<-- " << json_rpc_handle_request(&rpc, &req);
        std::cout << "\n";
    }

    return 0;

}

