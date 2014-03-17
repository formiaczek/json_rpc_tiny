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

void rpc_handling_examples(char** argv);
void extracting_json_examples();
void print_all_members_of_object(const std::string& input, int curr_pos, int object_len);
int run_tests();


// ========  example JSON RPC handlers ==========

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

    return json_rpc_create_result(res.str().c_str(), info);
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
        if(strncmp(last_name, "Python", last_name_len) == 0 &&
           age == 26)
        {
            res = json_rpc_create_result("\"Monty\"", info);
        }
        else
        {
            res = json_rpc_create_result("none", info);
        }
    }
    else
    {
        // return json_rpc_error (using error value) on failure
        res = json_rpc_create_error(json_rpc_err_invalid_params, info);
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
        res = json_rpc_create_error(msg.str().c_str(), info); // and past it as a string
    }
    else
    {
        res = json_rpc_create_result("\"OK\"", info);
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
        res = json_rpc_create_result(msg.str().c_str(), info);
    }
    else
    {
        res = json_rpc_create_error(json_rpc_err_internal_error, info);
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
        result << "{" << "\"operation\": \"" << std::string(operation, op_len);
        result << "\", \"res\": ";

        switch(operation[0])
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
        res = json_rpc_create_result(result.str().c_str(), info);
    }
    else
    {
        // return json_rpc_error (using error value) on failure
        res = json_rpc_create_error(json_rpc_err_invalid_params, info);
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
        s << "\"second\": \"" << std::string(second, second_len) << "\", ";
        s << "\"third\": " << third << "}";

        res = json_rpc_create_result(s.str().c_str(), info);
    }
    else
    {
        // return json_rpc_error (using error value) on failure
        res = json_rpc_create_error(json_rpc_err_invalid_params, info);
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
    return json_rpc_create_result("OK", info);
}

char* send_back(rpc_request_info_t* info)
{
    char* res = NULL;
    int len = 0;
    const char* msg = rpc_extract_param_str("what", &len, info);
    if(msg)
    {
        std::string message(msg, len);
        message = "{\"res\": \"" + message + "\"}";
        res = json_rpc_create_result(message.c_str(), info);
    }
    else
    {
        res = json_rpc_create_error(json_rpc_err_invalid_params, info);
    }
    return res;
}

// ====  example JSON RPC requests ==
const char* example_requests[] =
{
 "{\"jsonrpc\": \"2.0\", \"method\": \"getTimeDate\", \"params\": none, \"id\": 10}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"helloWorld\", \"params\": [\"Hello World\"], \"id\": 11}",
 "{\"method\": \"search\", \"params\": [{\"last_name\": \"Python\", \"age\": 26}], \"id\": 22}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"search\", \"params\": [{\"last_n\": \"Python\"}], \"id\": 43}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"search\", \"params\": [{\"last_name\": \"Doe\"}], \"id\": 54}",
 "{\"jsonrpc\": \"2.0\", \"thod\": \"search\", ", // not valid, but not whole object, will not be parsed..
 "{\"method\": \"err_example\",  \"params\": [], \"id\": 36}", // not valid
 "{\"jsonrpc\": \"2.0\", \"method\": \"use_param\", \"params\": [], \"id\": 37s}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"first\": 128, \"second\": 32, \"op\": \"+\"}], \"id\": 38}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"second\": 0x10, \"first\": 0x2, \"op\": \"*\"}], \"id\": 39}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"first\": 128, \"second\": 32, \"op\": \"+\"}], \"id\": 40}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"ordered_params\", \"params\": [128, \"the string\", 0x100], \"id\": 41}",
 "{\"method\": \"handleMessage\", \"params\": [\"user3\", \"sorry, gotta go now, ttyl\"], \"id\": null}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"first\": -0x17, \"second\": -17, \"op\": \"+\"}], \"id\": 43}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"calculate\", \"params\": [{\"first\": -0x32, \"second\": -055, \"op\": \"-\"}], \"id\": 44}",
 "{\"jsonrpc\": \"2.0\", \"method\": \"send_back\", \"params\": [{\"what\": \"{[{abcde}]}\"}], \"id\": 45}",
 "{\"jsonrpc\": \"2.0\", \"thod\": \"search\".. }", // not valid, but a whole object, jsonrpc will be parsed..
};
const int num_of_examples = sizeof(example_requests)/sizeof(char*);



int main(int argc, char** argv)
{
    rpc_handling_examples(argv); // examples on how tho register handlers, parse requests and responses.

    extracting_json_examples(); // examples on how to traverse (and extract all members of) a JSON object

    return run_tests();  // sanity tests. Note that they depend on some examples defined above..
}


// define our storage  (C-style) (*can also be allocated dynamically)
#define MAX_NUM_OF_HANDLERS 32
json_rpc_handler_t storage_for_handlers[MAX_NUM_OF_HANDLERS];

#define RESPONSE_BUF_MAX_LEN  256
char response_buffer[RESPONSE_BUF_MAX_LEN];

void rpc_handling_examples(char** argv)
{
    // create and initialise the instance
    json_rpc_instance_t rpc;
    json_rpc_init(&rpc, storage_for_handlers, MAX_NUM_OF_HANDLERS);

    // register our handlers
    json_rpc_register_handler(&rpc, "handleMessage",  handleMessage);
    json_rpc_register_handler(&rpc, "getTimeDate",    getTimeDate);
    json_rpc_register_handler(&rpc, "search",         search);
    json_rpc_register_handler(&rpc, "err_example",    non_20_error_example);
    json_rpc_register_handler(&rpc, "use_argument",   use_argument);
    json_rpc_register_handler(&rpc, "calculate",      calculate);
    json_rpc_register_handler(&rpc, "ordered_params", ordered_params);
    json_rpc_register_handler(&rpc, "send_back",      send_back);

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
        std::cout << "\n<-- " << res_str << "\n";

        // try to extract response and print it (see extracting_json_examples() for more on extracting)
        if(res_str)
        {
            int result_len = 0;
            const char* result_str = json_extract_member_str("result", &result_len, res_str, strlen(res_str));
            if(result_str)
            {
                std::cout << "result was: " << std::string(result_str, result_len) << "\n";
                for (int i = 0; ; i++)
                {
                    int member_val_len = 0;
                    const char* member_start = json_extract_member_str(i, &member_val_len, result_str, result_len);
                    if(!member_val_len)
                    {
                        break;
                    }
                    std::cout << " result [" << i << "]: " << std::string(member_start, member_val_len) << "\n";
                }
            }
        }
        std::cout << "\n";
    }
}

void extracting_json_examples()
{
    std::cout << "\n\n ==== extracting_json_examples ====\n\n";
    std::string input = "{\"jsonrpc\": \"2.0\", \"method\": \"getTimeDate\", \"params\": none, \"id\": 123}";
    std::cout << "printing all members of: \n " << input << "\n\n";
    print_all_members_of_object(input, 0, input.size());

    input = "{[{\"first\": 128, \"second\": 32, \"op\": \"+\"}, {\"jsonrpc\": \"2.0\", \"method\": \"getTimeDate\"}]}";
    std::cout << "printing all members of: \n " << input << "\n\n";
    print_all_members_of_object(input, 0, input.size());

    input = "{\"jsonrpc\": \"2.0\", \"method\": \"ordered_params\", \"params\": [128, \"the string\", 0x100], \"id\": 40}";
    std::cout << "\n---\nprinting all members of: \n " << input << "\n\n";
    print_all_members_of_object(input, 0, input.size());


    std::cout << "\n\n ==== finding members by name ====\n";
    std::string member_name = "jsonrpc";
    int member_val_len = 0;
    const char* member_start = json_extract_member_str(member_name.c_str(), &member_val_len, input.c_str(), input.size());
    std::cout << member_name << " (found by name): " << std::string(member_start, member_val_len) << "\n";

    member_name = "params";
    member_start = json_extract_member_str(member_name.c_str(), &member_val_len, input.c_str(), input.size());
    std::cout << member_name << " (found by name): " << std::string(member_start, member_val_len) << "\n";


    member_name = "method";
    member_start = json_extract_member_str(member_name.c_str(), &member_val_len, input.c_str(), input.size());
    std::cout << member_name << " (found by name): " << std::string(member_start, member_val_len) << "\n";

    member_name = "id";
    member_start = json_extract_member_str(member_name.c_str(), &member_val_len, input.c_str(), input.size());
    std::cout << member_name << " (found by name): " << std::string(member_start, member_val_len) << "\n";


    input = "{\"result\": {\"operation\": \"*\", \"res\": 32}, \"error\": none, \"id\": 38}";

    member_name = "result";
    member_start = json_extract_member_str(member_name.c_str(), &member_val_len, input.c_str(), input.size());
    std::cout << member_name << " (found by name): " << std::string(member_start, member_val_len) << "\n";

    member_name = "operation";
    member_start = json_extract_member_str(member_name.c_str(), &member_val_len, input.c_str(), input.size());
    std::cout << member_name << " (found by name): " << std::string(member_start, member_val_len) << "\n";

    member_name = "res";
    member_start = json_extract_member_str(member_name.c_str(), &member_val_len, input.c_str(), input.size());
    std::cout << member_name << " (found by name): " << std::string(member_start, member_val_len) << "\n";

    member_name = "operation";
    member_start = json_extract_member_str(member_name.c_str(), &member_val_len, input.c_str(), input.size());
    std::cout << member_name << " (found by name): " << std::string(member_start, member_val_len) << "\n";

    std::cout << "\n\n ==== finding members by number (order) ==== \n";

    input = "{{\"first\": 128, \"second\": 32, \"op\": \"+\"}, {\"jsonrpc\": \"2.0\", \"method\": \"getTimeDate\"}}";
    std::cout << "JSON object: \n" << input << "\n\n";
    for (int i = 0; ; i++)
    {
        member_start = json_extract_member_str(i, &member_val_len, input.c_str(), input.size());
        if(!member_val_len)
        {
            break;
        }
        std::cout << " member #" << i << ": " << std::string(member_start, member_val_len) << "\n";
    }

    input = example_requests[3];
    std::cout << "\n\nJSON object: \n" << input << "\n\n";
    for (int i = 0; ; i++)
    {
        member_start = json_extract_member_str(i, &member_val_len, input.c_str(), input.size());
        if(!member_val_len)
        {
            break;
        }
        std::cout << " member #" << i << ": " << std::string(member_start, member_val_len) << "\n";
    }

    std::cout << "\n\n ==== extracting_json_examples (end) ====\n\n";
}

// example function to show how to print recursively all sub-objects / lists within a JSON object
void print_all_members_of_object(const std::string& input, int curr_pos, int object_len)
{
    json_token_info info;
    std::cout << "=> inside next sub-obj\n";
    int object_pos_max_within_input = curr_pos + object_len;

    while(true)
    {
        curr_pos = json_find_next_member(curr_pos, input.c_str(), object_pos_max_within_input, &info);
        if(info.values_len == 0)
        {
            break;
        }
        std::cout << "next member is: ->";
        std::cout << input.substr(info.name_start, info.name_len);
        std::cout << "<- value (at: " << info.values_start << ", len: " << info.values_len << "): ";
        std::cout << ">" << input.substr(info.values_start, info.values_len) << "<\n";

        if(json_next_member_is_object_or_list(input.c_str(), &info))
        {
            print_all_members_of_object(input, info.values_start+1, info.values_len);
        }
    }
    std::cout << "<= end of next sub-obj\n";
}



// --------------- TEST CODE -----------------------------------

#include <stdexcept>
#define TEST_COND_(_cond_, ...) ({ if(!(_cond_)) { \
                                  std::stringstream s; s << "test error: assertion at line: " << __LINE__ << "\n"; \
                                  s << " " << #_cond_ << "\n"; \
                                  throw std::runtime_error(s.str()); } })

// helper method: executes JSON rpc given example number and returns the response
const char*  handle_request_for_example(int example_number, json_rpc_data_t& req_data, json_rpc_instance& rpc)
{
    req_data.request = example_requests[example_number];
    req_data.request_len = strlen(example_requests[example_number]);
    return json_rpc_handle_request(&rpc, &req_data);
}


template <typename ParamType>
int extract_int_param(ParamType param_name_or_pos, const std::string& res_str)
{
    int int_res = -1;
    if(!json_extract_member_int(param_name_or_pos, &int_res, res_str.c_str(), res_str.length()))
    {
        std::stringstream err;
        err << __FUNCTION__ << " error extracting param: " << param_name_or_pos;
        err << " from: " << res_str << "\n";
        throw std::runtime_error(err.str());
    }
    return int_res;
}

template <typename ParamType>
std::string extract_str_param(ParamType param_name_or_pos, const std::string& res_str)
{
    int str_size = 0;
    const char* str_res = json_extract_member_str(param_name_or_pos, &str_size, res_str.c_str(), res_str.length());
    return std::string(str_res, str_size);
}

int run_tests()
{
    json_rpc_instance_t rpc;
    json_rpc_init(&rpc, storage_for_handlers, MAX_NUM_OF_HANDLERS);

    // register our handlers
    json_rpc_register_handler(&rpc, "handleMessage",  handleMessage);
    json_rpc_register_handler(&rpc, "getTimeDate",    getTimeDate);
    json_rpc_register_handler(&rpc, "search",         search);
    json_rpc_register_handler(&rpc, "err_example",    non_20_error_example);
    json_rpc_register_handler(&rpc, "use_argument",   use_argument);
    json_rpc_register_handler(&rpc, "calculate",      calculate);
    json_rpc_register_handler(&rpc, "ordered_params", ordered_params);
    json_rpc_register_handler(&rpc, "send_back",      send_back);


    try
    {
        json_rpc_data_t req_data;
        req_data.response = response_buffer;
        req_data.response_len = RESPONSE_BUF_MAX_LEN;
        const char* res_str = NULL;

        res_str = handle_request_for_example(2, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_str_param(0, res_str) == "Monty"); // "result": "Monty"
        TEST_COND_(extract_str_param(1, res_str) == "none"); // "error": none
        TEST_COND_(extract_str_param(2, res_str) == "22"); // "id": 22
        TEST_COND_(extract_int_param(2, res_str) == 22); // "id": 22
        TEST_COND_(extract_str_param(3, res_str) == ""); // not existing.

        res_str = handle_request_for_example(5, req_data, rpc);
        TEST_COND_(res_str);
        std::cout << "=====> " << extract_str_param("id", res_str);
        TEST_COND_(extract_str_param("id", res_str) == "none"); // "id": none

        std::string error = extract_str_param("error", res_str);
        TEST_COND_(extract_int_param("code", error) == -32600);
        TEST_COND_(extract_str_param("message", res_str) == "Invalid Request");

        res_str = handle_request_for_example(16, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_str_param("jsonrpc", res_str) == "2.0"); // "jsonrpc": "2.0"
        error = extract_str_param("error", res_str);
        TEST_COND_(extract_int_param("code", error) == -32600);

        res_str = handle_request_for_example(9, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_int_param("res", res_str) == 32);
        TEST_COND_(extract_str_param("operation", res_str) == "*");

        res_str = handle_request_for_example(10, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_str_param("operation", res_str) == "+");
        TEST_COND_(extract_int_param("res", res_str) == 160);

        res_str = handle_request_for_example(11, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_str_param("jsonrpc", res_str) == "2.0");
        TEST_COND_(extract_int_param("first", res_str) == 128);
        TEST_COND_(extract_str_param("second", res_str) == "the string");
        TEST_COND_(extract_int_param("third", res_str) == 256);

        std::string expected = "{\"first\": 128, \"second\": \"the string\", \"third\": 256}";
        TEST_COND_(extract_str_param(0, res_str) == "2.0");    // the whole '0-param' part
        TEST_COND_(extract_str_param(1, res_str) == expected); // the whole '0-param' part
        TEST_COND_(extract_int_param(2, res_str) == 41);   // "id": 41
        // and result..
        TEST_COND_(extract_int_param(0, expected) == 128);
        TEST_COND_(extract_str_param(1, expected) == "the string");
        TEST_COND_(extract_int_param(2, expected) == 256);

        // tests negative value extraction (hex/dec/oct) etc.
        res_str = handle_request_for_example(13, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_int_param("res", res_str) == -40);

        res_str = handle_request_for_example(14, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_int_param("res", res_str) == -5);

        res_str = handle_request_for_example(15, req_data, rpc);
        TEST_COND_(res_str);
        TEST_COND_(extract_str_param("res", res_str) == "{[{abcde}]}"); // if value in quotes, do not treat it as JSON


        // test batch requests..
        std::string batch_request("[");
        batch_request += example_requests[8];
        batch_request += ",";
        batch_request += example_requests[9];
        batch_request += "]";

        // prepare request buffer
        req_data.request = batch_request.c_str();
        req_data.request_len = batch_request.size();

        // and handle rpc request
        res_str = json_rpc_handle_request(&rpc, &req_data);
        std::cout << "\nbatch request:\n--> " << batch_request << "\n";
        std::cout << "\n<-- " << res_str << "\n";

        TEST_COND_(res_str);
        std::string batch_res = extract_str_param(0, res_str); // first batch item
        TEST_COND_(extract_int_param("res", batch_res) == 160);
        TEST_COND_(extract_str_param("operation", batch_res) == "+");
        TEST_COND_(extract_int_param("id", batch_res) == 38);

        batch_res = extract_str_param(1, res_str); // second batch item
        TEST_COND_(extract_int_param("res", batch_res) == 32);
        TEST_COND_(extract_str_param("operation", batch_res) == "*");
        TEST_COND_(extract_int_param("id", batch_res) == 39);

        batch_request = "[,233]"; // invalid requests in the batch..
        // prepare request buffer
        req_data.request = batch_request.c_str();
        req_data.request_len = batch_request.size();

        // and handle rpc request
        res_str = json_rpc_handle_request(&rpc, &req_data);
        std::cout << "\nbatch request:\n--> " << batch_request << "\n";
        std::cout << "\n<-- " << res_str << "\n";

        TEST_COND_(res_str);
        batch_res = extract_str_param(0, res_str); // first batch item
        TEST_COND_(extract_str_param("error", batch_res) == "{\"code\": -32600, \"message\": \"Invalid Request\"}");
        TEST_COND_(extract_str_param("id", batch_res) == "none");

        batch_res = extract_str_param(1, res_str); // second batch item
        TEST_COND_(extract_str_param("error", batch_res) == "{\"code\": -32600, \"message\": \"Invalid Request\"}");
        TEST_COND_(extract_str_param("id", batch_res) == "none");


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

