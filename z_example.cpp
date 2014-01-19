/*
 * z_example.cpp
 *
 *  Created on: 11 Feb 2013
 *      Author: lukasz.forynski
 *
 * This file contains eample uses of json_rpc_tiny and is a mix of C and C++
 * (mostly for convenience).
 */

#include "json_rpc_tiny.h"


#include <string.h>

#include <iostream>
#include <sstream>
#include <ctime>

#include <stdio.h>

int run_tests(json_rpc_instance&);

// does not use any params
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

// uses named params
char* search(rpc_request_info_t* info)
{
    // could do it in 'C' (a.k.a.: hacking)
    char* res = 0;
    const char* last_name;

    // for named params- there are 'helper' functions to find and extract parameters by their name
    // (can be useful as this allows finding them regardless of their order in the original request.
    int age = 0;
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

// uses the argument set in the json_rpc_instance::data::arg
char* use_argument(rpc_request_info_t* info)
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

// uses named parameters: 'first', 'second' and 'operation',
// calculates result and forms the response.
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
        result << "{" << "\"operation\": " << std::string(operation, op_len);
        result << ", \"res\": ";

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
        result << "}";
        res = json_rpc_result(result.str().c_str(), info);
    }
    else
    {
        // return json_rpc_error (using error value) on failure
        res = json_rpc_error(json_rpc_err_invalid_params, info);
    }
    return res;
}

// uses position based params (i.e. extracts params based on their order)
// and just replies their values as "first": <val0>, "second": <val1> ..etc..
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

// demonstrates manual access to params within the request string
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
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"first\": -0x17, \"second\": -17, \"op\": \"+\"}], \"id\": 41}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"first\": -0x32, \"second\": -055, \"op\": \"-\"}], \"id\": 42}",

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
    json_rpc_register_handler(&rpc, "handleMessage",  handleMessage);
    json_rpc_register_handler(&rpc, "getTimeDate",    getTimeDate);
    json_rpc_register_handler(&rpc, "search",         search);
    json_rpc_register_handler(&rpc, "err_example",    non_20_error_example);
    json_rpc_register_handler(&rpc, "use_argument",   use_argument);
    json_rpc_register_handler(&rpc, "calculate",      calculate);
    json_rpc_register_handler(&rpc, "ordered_params", ordered_params);

    // prepare and initialise request data
    json_rpc_data_t req_data;
    req_data.response = response_buffer;
    req_data.response_len = RESPONSE_BUF_MAX_LEN;
    req_data.arg = argv[0];

    // now execute try it out with example requests defined above
    // printing request and response to std::out
    for(int i = 0; i < num_of_examples; i++)
    {
        // assign buffer from next example
        req_data.request = example_requests[i];
        req_data.request_len = strlen(example_requests[i]);

        // and handle rpc request
        char* res_str = json_rpc_handle_request(&rpc, &req_data);
        std::cout << "\n" << i << ": " << "\n--> " << example_requests[i];
        std::cout << "\n<-- " << res_str << "\n"; //json_rpc_handle_request(&rpc, &req);

        // try to extract response params and print them
        if(res_str)
        {
            rpc_request_info_t info;
            json_rpc_data_t r_data; // (could re-use req_data )
            if(json_rpc_parse_result(res_str, &r_data, &info))
            {
                int int_res = -1;

                // extract str params from the response (position based)
                const char* str_res = NULL;
                for (int p = 0; p < 3; p++)
                {
                    str_res = rpc_extract_param_str(p, &int_res, &info);
                    if(str_res)
                    {
                        std::cout << "pos " << p << " str in res: [" << std::string(str_res, int_res) << "]\n";
                    }
                }

                str_res = rpc_extract_param_str("operation", &int_res, &info);
                if(rpc_extract_param_str("operation", &int_res, &info))
                {
                    std::cout << "named \"operation\" in res: [" << std::string(str_res, int_res) << "]\n";
                }

                if(rpc_extract_param_int("res", &int_res, &info))
                {
                    std::cout << "named \"res\" in res: [" << std::dec << int_res << "]\n";
                }
            }
        }
        std::cout << "\n";
    }

    return run_tests(rpc);  // run sanity tests. Note that they depend on some examples defined above..
}

// --------------- TEST CODE -----------------------------------

#include <stdexcept>
#define TEST_COND_(_cond_, ...) ({ if(!(_cond_)) { \
                                  std::stringstream s; s << "test error: assertion at line: " << __LINE__ << "\n"; \
                                  throw std::runtime_error(s.str()); } })

const char*  handle_request_for_example(int example_number, json_rpc_data_t& req_data, json_rpc_instance& rpc)
{
    req_data.request = example_requests[example_number];
    req_data.request_len = strlen(example_requests[example_number]);
    return json_rpc_handle_request(&rpc, &req_data);
}


template <typename ParamType>
int extract_int_param(ParamType param_pos, const char* res_str, json_rpc_instance& rpc)
{
    int int_res = -1;
    rpc_request_info_t info;
    json_rpc_data_t r_data;
    if(json_rpc_parse_result(res_str, &r_data, &info))
    {
        if(!rpc_extract_param_int(param_pos, &int_res, &info))
        {
            std::stringstream err;
            err << __FUNCTION__ << " error extracting param: " << param_pos;
            err << " from: " << res_str << "\n";
        }
    }
    return int_res;
}

template <typename ParamType>
std::string extract_str_param(ParamType param_pos, const char* res_str, json_rpc_instance& rpc)
{
    std::string result;
    rpc_request_info_t info;
    json_rpc_data_t r_data;
    if(json_rpc_parse_result(res_str, &r_data, &info))
    {
        int str_size = 0;
        const char* str_res = rpc_extract_param_str(param_pos, &str_size, &info);
        if(str_res)
        {
            result = std::string(str_res, str_size);
        }
        else
        {
            std::stringstream err;
            err << __FUNCTION__ << " error extracting param: " << param_pos;
            err << " from: " << res_str << "\n";
        }
    }
    return result;
}

int run_tests(json_rpc_instance& rpc)
{
    try
    {
        json_rpc_data_t req_data;
        req_data.response = response_buffer;
        req_data.response_len = RESPONSE_BUF_MAX_LEN;
        const char* res_str;

        res_str = handle_request_for_example(2, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_str_param(0, res_str, rpc) == "\"Monty\"");
        TEST_COND_(extract_str_param(1, res_str, rpc) == ""); // not existing.

        res_str = handle_request_for_example(9, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_int_param("res", res_str, rpc) == 32);
        TEST_COND_(extract_str_param("operation", res_str, rpc) == "\"*\"");

        res_str = handle_request_for_example(10, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_str_param("operation", res_str, rpc) == "\"+\"");
        TEST_COND_(extract_int_param("res", res_str, rpc) == 160);

        res_str = handle_request_for_example(11, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_int_param("first", res_str, rpc) == 128);
        TEST_COND_(extract_str_param("second", res_str, rpc) == "\"the string\"");
        TEST_COND_(extract_int_param("third", res_str, rpc) == 256);
        TEST_COND_(extract_str_param(0, res_str, rpc) == "\"first\": 128"); // the whole '0-param' part

        // tests negative value extraction (hex/dec/oct) etc.
        res_str = handle_request_for_example(13, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_int_param("res", res_str, rpc) == -40);

        res_str = handle_request_for_example(14, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_int_param("res", res_str, rpc) == -5);

        std::cout << "\n===== ALL TESTS PASSED =====\n\n";
    }
    catch(const std::exception& e)
    {
        std::cout << "\n\n===== TESTING ERROR =====\n\n";
        std::cerr << e.what() << "\n";
        return 1;
    }
    return 0;
}

