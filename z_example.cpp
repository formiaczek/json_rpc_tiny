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
    const char* last_name;

    // for named params- there are too 'helper' functions to find and extract parameters by their name
    // (can be useful as this allows finding them regardless of their order in the original request.
    // (in C++ could alternatively extract them one by one, putting them in a map,
    // and then access them by name, for example)

    int age;
    int last_name_len = 0;
    last_name = rpc_extract_param_str("last_name", &last_name_len, info);

    if(last_name && rpc_extract_param_int("age", &age, info))
    {
        if(strncmp(last_name, "\"Python\"", last_name_len) == 0 &&
           age == 26)
        {
            res = json_rpc_result("\"Monty\"", info);
        }
        else
        {
            res = json_rpc_result("none", info);
        }
    }
    else
    {
        // return json_rpc_error (using error value) on failure
        res = json_rpc_error(json_rpc_err_invalid_params, info);
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


char* calculate(rpc_request_info_t* info)
{
    char* res = 0;
    int first;
    int second;

    int op_len = 0;
    const char* operation = rpc_extract_param_str("op", &op_len, info);

    if(operation &&
      rpc_extract_param_int("first", &first, info) &&
      rpc_extract_param_int("second", &second, info))
    {
        std::stringstream result;
        switch(operation[1]) // operation is defined in quotes
        {
        case '*':
            result << first * second;
            break;

        case '+':
            result << first + second;
            break;

        case '-':
            result << first - second;
            break;

        case '/':
            result << first / second;
            break;
        }
        std::cout << "\nresult : " << result.str();
        res = json_rpc_result(result.str().c_str(), info);
    }
    else
    {
        // return json_rpc_error (using error value) on failure
        res = json_rpc_error(json_rpc_err_invalid_params, info);
    }
    return res;
}

char* ordered_params(rpc_request_info_t* info)
{
    char* res = 0;

    // can use similar functions to extract parameters by their position
    // (i.e. if they are comma-separated, not-named parameters)
    int first;
    int third;

    int second_len = 0;
    const char* second = rpc_extract_param_str(1, &second_len, info); // zero-based second parameter
    if(second &&
       rpc_extract_param_int(0, &first, info) &&
       rpc_extract_param_int(2, &third, info))
    {
        std::stringstream s;
        s << "{\"first\": " << first << ", ";
        s << "\"second\": " << std::string(second, second_len) << ", ";
        s << "\"third\": " << third << "}";

        res = json_rpc_result(s.str().c_str(), info);
    }
    else
    {
        // return json_rpc_error (using error value) on failure
        res = json_rpc_error(json_rpc_err_invalid_params, info);
    }
    return res;
}

char* handleMessage(rpc_request_info_t* info)
{
    // of course info contains all information about the request.
    // input (request) string is in {info->data->request, info->data->request_len}
    // information about that data is already parsed, and params_start is offset
    // within the request to the beginning of string containing parameters.
    // info->params_len contains length of this string.
    // This can, if really needed, be used directly if extraction-aiding methods are not
    // applicable or one whishes to do it all manually.
    char params[64];
    strncpy(params, info->data->request + info->params_start, info->params_len);
    params[info->params_len] = 0;

    printf(" ===> called handleMessage(%s, notif: %d)\n\n", params, info->info_flags);

    // note : don't change response directly: using json_rpc_result() or json_rpc_error()
    // you can update response data with required data (and JSON tags will be appended
    // automatically
    return json_rpc_result("\"OK\"", info);
}


const char* example_requests[] =
{
 "{\"jsonrpc\": \"2.0\", \"method\": \"getTimeDate\", \"params\": none, \"id\": 123}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"helloWorld\", \"params\": [\"Hello World\"], \"id\": 32}",
 "{\"method\": \"search\", \"params\": [{\"last_name\": \"Python\"}, {\"age\": 26}], \"id\": 32}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"search\", \"params\": [{\"last_n\": \"Python\"}], \"id\": 33}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"search\", \"params\": [{\"last_name\": \"Doe\"}], \"id\": 34}",
 "{\"jsonrpc\": \"2.0\", \"thod\": \"search\", ", // not valid
 "{\"method\": \"err_example\",  \"params\": [], \"id\": 35}", // not valid
 "{\"jsonrpc\": \"2.0\", \"method\": \"use_param\", \"params\": [], \"id\": 36s}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"first\": 128, \"second\": 32, \"op\": \"+\"}], \"id\": 37}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"second\": 0x10, \"first\": 0x2, \"op\": \"*\"}], \"id\": 38}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"first\": 128, \"second\": 32, \"op\": \"+\"}], \"id\": 39}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"ordered_params\", \"params\": [128, \"the string\", 0x100], \"id\": 40}",
 "{\"method\": \"handleMessage\", \"params\": [\"user3\", \"sorry, gotta go now, ttyl\"], \"id\": null}",
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
    json_rpc_register_handler(&rpc, "calculate", calculate);
    json_rpc_register_handler(&rpc, "ordered_params", ordered_params);

    // prepare and initialise request data
    json_rpc_request_data_t req;
    req.response = response_buffer;
    req.response_len = RESPONSE_BUF_MAX_LEN;
    req.arg = argv[0];

    // now execute try it out with example requests defined above
    // printing request and response to std::out
    for(int i = 0; i < num_of_examples; i++)
    {
        req.request = example_requests[i];
        req.request_len = strlen(example_requests[i]);
        std::cout << "\n--> " << example_requests[i] << "\n<-- " << json_rpc_handle_request(&rpc, &req);
        std::cout << "\n";
    }

    return 0;

}

