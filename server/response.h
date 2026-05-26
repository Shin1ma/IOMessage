/*
*    Made by Ruben Amadei on xx/xx/2026
*
*    Helper functions that enable to format responses in a structure and as its raw form
*
*   RESPONSE FORMAT
*       responses follow this format: [{<status code>}{<payload>}]
*       '[]' characters delimit the response start and end
*       '{}' characters delimit the beginning and end of a section
*       <status code>: numeric number representing a success (2xx), an error (1xx)
*       where the second digit represents the section that gave the error and the third the error code:
*           SECTION CODES:
*           x0x - communicator error:
*               100 - ungraceful exit: your client acted strangely/absent and was kicked out
*               200 - graceful exit: your client or the communicator decided to terminate the connection
*           xxx - TODO
*       <payload>: string representing a response eg. "message sent", "error in sending message" character escape
*       is achieved by using the '\' character before the symbol we want to escape, any escaped symbols will be ignored by
*       response parser (eg. \[ will be skipped entirely) and they will be treated as part of the payload and not the response eg. you can use {} in the payload by escaping them \{ \} or even \\
*/

#ifndef RESPONSE_H
#define RESPONSE_H

#include <stdlib.h>
#include <assert.h>
#include "utils.h"

typedef struct response{
    int status;
    char* payload;
} response_t;

response_t* make_resp(int status, const char* payload); //allocates the response structure for easy sending
void destroy_resp(response_t* resp);

/*
formats the response into a raw string to be sendable, the returned string
is allocated on the heap and ownership is of the caller
*/
char* make_raw_resp(const response_t* resp);

#endif