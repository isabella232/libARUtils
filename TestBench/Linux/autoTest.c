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

#define ARACADEMY_AUTOTEST_TAG          "autoTest"

extern void test_ftp_connection(char *tmp);
//extern void test_http_connection(char *tmp);

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
    //ARSAL_PRINT(ARSAL_PRINT_WARNING, ARACADEMY_AUTOTEST_TAG, "options <-f, -h> -f: ftp tests, -h: http tests");
    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARACADEMY_AUTOTEST_TAG, "autoTest Starting");
    
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
        //test_http_connection(tmpPath);
    }

    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARACADEMY_AUTOTEST_TAG, "autoTest Completed");
    return 0;
}

