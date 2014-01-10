/**
 * @file test_http_connection.c
 * @brief libARUtils test http connection c file.
 * @date 19/12/2013
 * @author david.flattin.ext@parrot.com
 */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libARSAL/ARSAL_Sem.h>
#include <libARUtils/ARUTILS_Error.h>
#include <libARUtils/ARUTILS_Http.h>

#define DEVICE_IP    "172.20.5.117"
#define HTTP_PORT     80


void test_http_progress_callback(void* arg, uint8_t percent)
{
    char *message = (char *)arg;

    printf("%s %d%%\n", message ? message : "null", percent);
}

void test_http_connection_public(char *tmp)
{
    ARUTILS_Http_Connection_t *connection = NULL;
    eARUTILS_ERROR result;
    ARSAL_Sem_t cancelSem;

    ARSAL_Sem_Init(&cancelSem, 0, 0);

    connection = ARUTILS_Http_Connection_New(&cancelSem, DEVICE_IP, HTTP_PORT, HTTPS_PROTOCOL_FALSE, NULL, NULL, &result);
    printf("result %d\n", result);

    char localFilePath[512];
    strcpy(localFilePath, tmp);
    strcat(localFilePath, "photo_20131001_235901.jpg");

    result = ARUTILS_Http_Get(connection, "photo_20131001_235901.jpg", localFilePath, test_http_progress_callback, "progress: ");
    printf("result %d\n", result);

    result = ARUTILS_Http_Connection_Cancel(connection);
    printf("result %d\n", result);

    ARUTILS_Http_Connection_Delete(&connection);
    ARSAL_Sem_Destroy(&cancelSem);
}

void test_http_connection_private(char *tmp)
{
    ARUTILS_Http_Connection_t *connection = NULL;
    eARUTILS_ERROR result;
    ARSAL_Sem_t cancelSem;

    ARSAL_Sem_Init(&cancelSem, 0, 0);

    connection = ARUTILS_Http_Connection_New(&cancelSem, DEVICE_IP, HTTP_PORT, HTTPS_PROTOCOL_FALSE, "parrot", "__parrot", &result);
    printf("result %d\n", result);

    char localFilePath[512];
    strcpy(localFilePath, tmp);
    strcat(localFilePath, "photo_20131001_235901.jpg");

    result = ARUTILS_Http_Get(connection, "private/photo_20131001_235901.jpg", localFilePath, test_http_progress_callback, "progress: ");
    printf("result %d\n", result);

    result = ARUTILS_Http_Connection_Cancel(connection);
    printf("result %d\n", result);

    ARUTILS_Http_Connection_Delete(&connection);
    ARSAL_Sem_Destroy(&cancelSem);
}

void test_http_connection(char *tmp)
{
    test_http_connection_public(tmp);

    test_http_connection_private(tmp);
}
