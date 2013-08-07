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


char* handleMessage(const char* rpc_request, rpc_request_info_t* info)
{
    char params[64];
    char* resp = 0;
    strncpy(params, rpc_request+info->params_start, info->params_len);
    params[info->params_len] = 0;


    printf(" ===> called handleMessage(%s, id: %d)\n\n", params, info->id);

    if(info->id >= 0)
    {
        resp = json_rpc_result("\"OK\"", rpc_request, info);
    }

    return resp;
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

    return json_rpc_result(res.str().c_str(), rpc_request, info);
}



const char* example1 = "{\"method\": \"handleMessage\", \"params\": [\"user3\", \"sorry, gotta go now, ttyl\"], \"id\": null}";
const char* example2 = "{\"jsonrpc\": \"2.0\", \"method\": \"getTimeDate\", \"params\": none, \"id\": 123}";
const char* example3 = "{\"jsonrpc\": \"2.0\", \"method\": \"helloWorld\", \"params\": [\"Hello World\"], \"id\": 32}";


int main(int argc, char **argv)
{
    json_rpc_init();

    // register our handlers
    json_rpc_register_handler("handleMessage", handleMessage);
    json_rpc_register_handler("getTimeDate", getTimeDate);


    json_rpc_exec(example1, strlen(example1));



    std::cout << "--> " << example2 << "\n<-- " << json_rpc_exec(example2, strlen(example2)) << "\n\n";

    std::cout << "--> " << example3 << "\n<-- " << json_rpc_exec(example3, strlen(example3)) << "\n";


    return 0;

}

