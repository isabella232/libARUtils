/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
/**
 * @file autoTest.c
 * @brief libARUtils autoTest c file.
 * @date 19/12/2013
 * @author david.flattin.ext@parrot.com
 **/


#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <libARSAL/ARSAL_Print.h>

#define ARUTILS_AUTOTEST_TAG          "autoTest"

extern void test_ftp_connection(char *tmp);
extern void test_http_connection(char *tmp);

/*void sigIntHandler(int sig)
{
    printf("SIGINT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    test_manager_checking_running_signal();
}*/

void sigAlarmHandler(int sig)
{
    printf("SIGALRM !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
}

int main(int argc, char *argv[])
{
    int opt = 0;
    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARUTILS_AUTOTEST_TAG, "options <-f, -h> -f: ftp tests, -h: http tests");
    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARUTILS_AUTOTEST_TAG, "autoTest Starting");

    if (argc > 1)
    {
        if (strcmp(argv[1], "-f") == 0)
        {
            opt = 1;
        }
        else if (strcmp(argv[1], "-h") == 0)
        {
            opt = 2;
        }
    }

    //signal(SIGINT, sigIntHandler);
    signal(SIGALRM, sigAlarmHandler);

    char *tmp = getenv("HOME");
    char tmpPath[512];
    strcpy(tmpPath, tmp);
    strcat(tmpPath, "/");

    if (opt == 1)
    {
        test_ftp_connection(tmpPath);
    }
    else if (opt == 2)
    {
        test_http_connection(tmpPath);
    }

    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARUTILS_AUTOTEST_TAG, "autoTest Completed");
    return 0;
}

