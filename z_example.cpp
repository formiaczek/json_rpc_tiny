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

const char* example3 = "{\"jsonrpc\": \"2.0\", \"method\": \"helloWorld\", \"params\": [\"Hello World\"], \"id\": 32}";
char* handleMessage(const char* rpc_request, rpc_request_info_t* info)
{
    char params[64];

    strncpy(params, rpc_request+info->params_start, info->params_len);
    params[info->params_len] = 0;


    printf(" ===> called handleMessage(%s, notif: %d)\n\n", params, info->is_notification);

    return json_rpc_result("\"OK\"", rpc_request, info);
}

char* getTimeDate(const char* rpc_request, rpc_request_info_t* info)
{
    std::stringstream res;
    time_t curr_time;
    time(&curr_time);
    struct tm * now = localtime(&curr_time);
    res << "\""
         << (now->tm_year + 1900) << '-'
         << (now->tm_mon + 1) << '-'
         <<  now->tm_mday
         << "\"";

    return json_rpc_result(res.str().c_str(), rpc_request, info); // return json_rpc_result on success
}

char* search(const char* rpc_request, rpc_request_info_t* info)
{
    // could do it in 'C' (a.k.a.: hacking)
    char params[64];
    strncpy(params, rpc_request+info->params_start, info->params_len);
    params[info->params_len] = 0;

    char param_name[16], param_value[16];

    printf("\n ===> called\n  search(%s, notif: %d)\n  i.e.:\n",
            params, info->is_notification);

    sscanf(params, "%*[^\"]\"%[^\"]\"%*[^\"]\"%[^\"]\"", param_name, param_value);
    printf("  search(%s=%s)\n", param_name, param_value);

    return json_rpc_error(json_rpc_invalid_params, rpc_request, info); // return json_rpc_error on failure
}


const char* example1 = "{\"method\": \"handleMessage\", \"params\": [\"user3\", \"sorry, gotta go now, ttyl\"], \"id\": null}";
const char* example2 = "{\"jsonrpc\": \"2.0\", \"method\": \"getTimeDate\", \"params\": none, \"id\": 123}";
const char* example4 = "{\"jsonrpc\": \"2.0\", \"method\": \"search\", \"params\": {\"last_name\": \"Python\"}, \"id\": 32}";
const char* example5 = "{\"jsonrpc\": \"2.0\", \"thod\": \"search\", "; // not valid


int main(int argc, char **argv)
{
    json_rpc_init();

    // register our handlers
    json_rpc_register_handler("handleMessage", handleMessage);
    json_rpc_register_handler("getTimeDate", getTimeDate);
    json_rpc_register_handler("search", search);



    // now execute execs with our example jsonrpc
    std::cout << "\n--> " << example1 << "\n<-- " << json_rpc_exec(example1, strlen(example1));



    std::cout << "\n--> " << example2 << "\n<-- " << json_rpc_exec(example2, strlen(example2)) << "\n\n";
    std::cout << "\n--> " << example3 << "\n<-- " << json_rpc_exec(example3, strlen(example3)) << "\n";

    std::cout << "\n--> " << example4 << "\n<-- " << json_rpc_exec(example4, strlen(example4)) << "\n";
    std::cout << "\n--> " << example5 << "\n<-- " << json_rpc_exec(example5, strlen(example5)) << "\n";

    return 0;

}

