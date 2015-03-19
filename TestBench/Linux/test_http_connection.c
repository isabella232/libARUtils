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


void test_http_progress_callback(void* arg, float percent)
{
    char *message = (char *)arg;

    printf("%s %02f%%\n", message ? message : "null", percent);
}

void test_http_connection_public(const char *tmp)
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

void test_http_connection_private(const char *tmp)
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

void test_http_connection_length(const char *tmp)
{
    ARUTILS_Http_Connection_t *connection = NULL;
    eARUTILS_ERROR result;
    ARSAL_Sem_t cancelSem;
    
    ARSAL_Sem_Init(&cancelSem, 0, 0);
    
    // //download.parrot.com/Drones/0902/update.php?product=0902&serialNo=0000&version=0.0.0&platform=iOS&appVersion=3.4.7
    connection = ARUTILS_Http_Connection_New(&cancelSem, "download.parrot.com", 80, HTTPS_PROTOCOL_FALSE, NULL, NULL, &result);
    printf("result %d\n", result);
    
    char localFilePath[512];
    strcpy(localFilePath, tmp);
    strcat(localFilePath, "photo_20131001_235901.jpg");
    uint8_t *data = NULL;
    uint32_t dataLen = 0;
    
    result = ARUTILS_Http_Get_WithBuffer(connection, "Drones/0902/update.php?product=0902&serialNo=0000&version=0.0.0&platform=iOS&appVersion=3.4.7", &data, &dataLen, test_http_progress_callback, "progress: ");
    
    if (data != NULL)
    {
        printf("data %s\n", (char*)data);
    }
    printf("result %d\n", result);
    
    result = ARUTILS_Http_Connection_Cancel(connection);
    printf("result %d\n", result);
    
    ARUTILS_Http_Connection_Delete(&connection);
    ARSAL_Sem_Destroy(&cancelSem);
}



void test_http_connection(const char *tmp)
{
    test_http_connection_public(tmp);

    test_http_connection_private(tmp);
    
    test_http_connection_length(tmp);
}
