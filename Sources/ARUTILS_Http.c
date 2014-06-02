/**
 * @file ARUTILS_Http.c
 * @brief libARUtils Http c file.
 * @date 26/12/2013
 * @author david.flattin.ext@parrot.com
 **/

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <libARSAL/ARSAL_Sem.h>
#include <libARSAL/ARSAL_Print.h>
#include <curl/curl.h>

#include "libARUtils/ARUTILS_Error.h"
#include "libARUtils/ARUTILS_Http.h"
#include "libARUtils/ARUTILS_FileSystem.h"
#include "ARUTILS_Http.h"

#define ARUTILS_HTTP_TAG              "Http"

#ifdef DEBUG
#define ARUTILS_HTTP_CURL_VERBOSE         1
#endif

// Doc
//http://curl.haxx.se/libcurl/c/libcurl-easy.html

/*****************************************
 *
 *             Public implementation:
 *
 *****************************************/

ARUTILS_Http_Connection_t * ARUTILS_Http_Connection_New(ARSAL_Sem_t *cancelSem, const char *server, int port, eARUTILS_HTTPS_PROTOCOL security, const char *username, const char* password, eARUTILS_ERROR *error)
{
    ARUTILS_Http_Connection_t *newConnection = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_HTTP_TAG, "%s, %d, %s", server ? server : "null", port, username ? username : "null");

    if (server == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }

    newConnection = (ARUTILS_Http_Connection_t *)calloc(1, sizeof(ARUTILS_Http_Connection_t));

    if (newConnection == NULL)
    {
        result = ARUTILS_ERROR_ALLOC;
    }

    if (result == ARUTILS_OK)
    {
        newConnection->cancelSem = cancelSem;
    }

    if (result == ARUTILS_OK)
    {
        if (security == HTTPS_PROTOCOL_FALSE)
        {
            sprintf(newConnection->serverUrl, "http://%s:%d/", server, port);
        }
        else
        {
            sprintf(newConnection->serverUrl, "https://%s:%d/", server, port);
        }
    }

    if ((result == ARUTILS_OK) && (username != NULL))
    {
        strncpy(newConnection->username, username, ARUTILS_HTTP_MAX_USER_SIZE);
        newConnection->username[ARUTILS_HTTP_MAX_USER_SIZE - 1] = '\0';
    }

    if ((result == ARUTILS_OK) && (password != NULL))
    {
        strncpy(newConnection->password, password, ARUTILS_HTTP_MAX_USER_SIZE);
        newConnection->password[ARUTILS_HTTP_MAX_USER_SIZE - 1] = '\0';
    }

    if (result == ARUTILS_OK)
    {
        newConnection->curl = curl_easy_init();

        if (newConnection->curl == NULL)
        {
            result = ARUTILS_ERROR_CURL_ALLOC;
        }
    }

    if (result != ARUTILS_OK)
    {
        ARUTILS_Http_Connection_Delete(&newConnection);
    }

    *error = result;
    return newConnection;
}

void ARUTILS_Http_Connection_Delete(ARUTILS_Http_Connection_t **connectionPtrAddr)
{
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_HTTP_TAG, "");

    if (connectionPtrAddr != NULL)
    {
        ARUTILS_Http_Connection_t *connection = *connectionPtrAddr;

        if (connection != NULL)
        {
            if (connection->curl != NULL)
            {
                curl_easy_cleanup(connection->curl);
            }

            ARUTILS_Http_FreeCallbackData(&connection->cbdata);

            free(connection);
            *connectionPtrAddr = NULL;
        }
    }
}

eARUTILS_ERROR ARUTILS_Http_Connection_Cancel(ARUTILS_Http_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    int resutlSys = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_HTTP_TAG, "");

    if ((connection == NULL) || (connection->cancelSem == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (result == ARUTILS_OK)
    {
        resutlSys = ARSAL_Sem_Post(connection->cancelSem);

        if (resutlSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }

    return result;
}

eARUTILS_ERROR ARUTILS_Http_Get(ARUTILS_Http_Connection_t *connection, const char *namePath, const char *dstFile, ARUTILS_Http_ProgressCallback_t progressCallback, void* progressArg)
{
    return ARUTILS_Http_Get_Internal(connection, namePath, dstFile, NULL, NULL, progressCallback, progressArg);
}

eARUTILS_ERROR ARUTILS_Http_Get_WithBuffer(ARUTILS_Http_Connection_t *connection, const char *namePath, uint8_t **data, uint32_t *dataLen, ARUTILS_Http_ProgressCallback_t progressCallback, void* progressArg)
{
    return ARUTILS_Http_Get_Internal(connection, namePath, NULL, data, dataLen, progressCallback, progressArg);
}

eARUTILS_ERROR ARUTILS_Http_Get_Internal(ARUTILS_Http_Connection_t *connection, const char *namePath, const char *dstFile, uint8_t **data, uint32_t *dataLen, ARUTILS_Http_ProgressCallback_t progressCallback, void* progressArg)
{
    char fileUrl[ARUTILS_HTTP_MAX_URL_SIZE];
    eARUTILS_ERROR result = ARUTILS_OK;
    CURLcode code = CURLE_OK;
    long httpCode = 0L;
    double remoteSize = 0.f;
    uint32_t localSize = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_HTTP_TAG, "%s, %s", namePath ? namePath : "null", dstFile ? dstFile : "null");

    if ((connection == NULL) || (connection->curl == NULL) || (namePath == NULL))
    {
        result =  ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (dstFile != NULL)
    {
        if ((data != NULL) || (dataLen != NULL))
        {
            result =  ARUTILS_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        if ((data == NULL) || (dataLen == NULL))
        {
            result =  ARUTILS_ERROR_BAD_PARAMETER;
        }
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_Http_IsCanceled(connection);
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_Http_ResetOptions(connection);
    }

    if (result == ARUTILS_OK)
    {
        strncpy(fileUrl, connection->serverUrl, ARUTILS_HTTP_MAX_URL_SIZE);
        fileUrl[ARUTILS_HTTP_MAX_URL_SIZE - 1] = '\0';
        strncat(fileUrl, namePath, ARUTILS_HTTP_MAX_URL_SIZE - strlen(fileUrl) - 1);

        code = curl_easy_setopt(connection->curl, CURLOPT_URL, fileUrl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if ((result == ARUTILS_OK) && (dstFile != NULL))
    {
        connection->cbdata.file = fopen(dstFile, "wb");

        if (connection->cbdata.file == NULL)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_WRITEDATA, connection);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_WRITEFUNCTION, ARUTILS_Http_WriteDataCallback);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (progressCallback != NULL)
    {
        if (result == ARUTILS_OK)
        {
            connection->cbdata.progressCallback = progressCallback;
            connection->cbdata.progressArg = progressArg;

            code = curl_easy_setopt(connection->curl, CURLOPT_PROGRESSDATA, connection);

            if (code != CURLE_OK)
            {
                result = ARUTILS_ERROR_CURL_SETOPT;
            }
        }

        if (result == ARUTILS_OK)
        {
            code = curl_easy_setopt(connection->curl, CURLOPT_PROGRESSFUNCTION, ARUTILS_Http_ProgressCallback);

            if (code != CURLE_OK)
            {
                result = ARUTILS_ERROR_CURL_SETOPT;
            }
        }

        if (result == ARUTILS_OK)
        {
            code = curl_easy_setopt(connection->curl, CURLOPT_NOPROGRESS, 0L);

            if (code != CURLE_OK)
            {
                result = ARUTILS_ERROR_CURL_SETOPT;
            }
        }
    }

    //libcurl process
    if (result == ARUTILS_OK)
    {
        code = curl_easy_perform(connection->curl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_Http_GetErrorFromCode(connection, code);
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, &httpCode);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_GETINFO;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &remoteSize);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_GETINFO;
        }
    }

    //result checking
    if ((result == ARUTILS_OK) && (connection->cbdata.error != ARUTILS_OK))
    {
        result = connection->cbdata.error;
    }

    if (result == ARUTILS_OK)
    {
        // GET OK (200)
        if (httpCode == 200)
        {
            if (dstFile != NULL)
            {
                fflush(connection->cbdata.file);
            }
        }
        else if (httpCode == 401)
        {
            result = ARUTILS_ERROR_HTTP_AUTHORIZATION_REQUIRED;
        }
        else if (httpCode == 403)
        {
            result = ARUTILS_ERROR_HTTP_ACCESS_DENIED;
        }
        else
        {
            result = ARUTILS_ERROR_HTTP_CODE;
        }
    }

    if ((result == ARUTILS_OK) && (dstFile != NULL))
    {
        result = ARUTILS_FileSystem_GetFileSize(dstFile, &localSize);

        if (result == ARUTILS_OK)
        {
            if (localSize != (uint32_t)remoteSize)
            {
                result = ARUTILS_ERROR_HTTP_SIZE;
            }
        }
    }    
    
    if ((result == ARUTILS_OK) && (data != NULL) && (dataLen != NULL))
    {
        if (result == ARUTILS_OK)
        {
            *data = connection->cbdata.data;
            connection->cbdata.data = NULL;
            *dataLen = connection->cbdata.dataSize;
        }
    }

    //cleanup
    if (connection != NULL)
    {
        ARUTILS_Http_FreeCallbackData(&connection->cbdata);
    }

    return result;
}

eARUTILS_ERROR ARUTILS_Http_Put(ARUTILS_Http_Connection_t *connection, const char *namePath, const char *srcFile, ARUTILS_Http_ProgressCallback_t progressCallback, void* progressArg)
{
    return ARUTILS_Http_Put_Internal(connection, namePath, srcFile, NULL, 0, progressCallback, progressArg);
}

eARUTILS_ERROR ARUTILS_Http_Put_Internal(ARUTILS_Http_Connection_t *connection, const char *namePath, const char *srcFile, uint8_t *data, uint32_t dataLen, ARUTILS_Http_ProgressCallback_t progressCallback, void* progressArg)
{
    struct curl_httppost *formItems = NULL;
    struct curl_httppost *lastFormItem = NULL;
    char fileUrl[ARUTILS_HTTP_MAX_URL_SIZE];
    eARUTILS_ERROR result = ARUTILS_OK;
    CURLFORMcode formCode = CURL_FORMADD_OK;
    CURLcode code = CURLE_OK;
    long httpCode = 0L;
    uint32_t localSize = 0;
    
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_HTTP_TAG, "%s, %s", namePath ? namePath : "null", srcFile ? srcFile : "null");
    
    if ((connection == NULL) || (connection->curl == NULL) || (namePath == NULL))
    {
        result =  ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (srcFile != NULL)
    {
        if ((data != NULL) || (dataLen != 0))
        {
            result =  ARUTILS_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        if ((data == NULL) || (dataLen == 0))
        {
            result =  ARUTILS_ERROR_BAD_PARAMETER;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        result = ARUTILS_Http_IsCanceled(connection);
    }
    
    if (result == ARUTILS_OK)
    {
        result = ARUTILS_Http_ResetOptions(connection);
    }
    
    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_CUSTOMREQUEST, "PUT");
        
        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        strncpy(fileUrl, connection->serverUrl, ARUTILS_HTTP_MAX_URL_SIZE);
        fileUrl[ARUTILS_HTTP_MAX_URL_SIZE - 1] = '\0';
        strncat(fileUrl, namePath, ARUTILS_HTTP_MAX_URL_SIZE - strlen(fileUrl) - 1);
        
        code = curl_easy_setopt(connection->curl, CURLOPT_URL, fileUrl);
        
        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if ((result == ARUTILS_OK) && (srcFile != NULL))
    {
        connection->cbdata.file = fopen(srcFile, "rb");
        
        if (connection->cbdata.file == NULL)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }
    
    if ((result == ARUTILS_OK) && (srcFile != NULL))
    {
        result = ARUTILS_FileSystem_GetFileSize(srcFile, &localSize);
    }
    
    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_READDATA, connection);
        
        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_READFUNCTION, ARUTILS_Http_ReadDataCallback);
        
        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        if (connection->cbdata.file != NULL)
        {
            const char *fileName = srcFile + strlen(srcFile);
            while ((fileName > srcFile) && (*fileName != '/'))
            {
                fileName--;
            }
            if (*fileName == '/')
            {
                fileName++;
            }
            
            formCode = curl_formadd(&formItems, &lastFormItem,
                                    CURLFORM_COPYNAME, "userfile",
                                    CURLFORM_STREAM, connection,
                                    CURLFORM_FILENAME, fileName,
                                    CURLFORM_CONTENTSLENGTH, (long)localSize,
                                    //CURLFORM_CONTENTTYPE, "text/plain",
                                    CURLFORM_END);
        }
        else
        {
            formCode = curl_formadd(&formItems, &lastFormItem,
                                                 CURLFORM_COPYNAME, "userdata",
                                                 //CURLFORM_FILENAME, "data.bin",
                                                 CURLFORM_COPYCONTENTS, data,
                                                 CURLFORM_CONTENTSLENGTH, (long)dataLen,
                                                 CURLFORM_END);
        }
        
        if (formCode != CURL_FORMADD_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_HTTPPOST, formItems);
        
        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if (progressCallback != NULL)
    {
        if (result == ARUTILS_OK)
        {
            connection->cbdata.progressCallback = progressCallback;
            connection->cbdata.progressArg = progressArg;
            
            code = curl_easy_setopt(connection->curl, CURLOPT_PROGRESSDATA, connection);
            
            if (code != CURLE_OK)
            {
                result = ARUTILS_ERROR_CURL_SETOPT;
            }
        }
        
        if (result == ARUTILS_OK)
        {
            code = curl_easy_setopt(connection->curl, CURLOPT_PROGRESSFUNCTION, ARUTILS_Http_ProgressCallback);
            
            if (code != CURLE_OK)
            {
                result = ARUTILS_ERROR_CURL_SETOPT;
            }
        }
        
        if (result == ARUTILS_OK)
        {
            code = curl_easy_setopt(connection->curl, CURLOPT_NOPROGRESS, 0L);
            
            if (code != CURLE_OK)
            {
                result = ARUTILS_ERROR_CURL_SETOPT;
            }
        }
    }
    
    //libcurl process
    if (result == ARUTILS_OK)
    {
        code = curl_easy_perform(connection->curl);
        
        if (code != CURLE_OK)
        {
            result = ARUTILS_Http_GetErrorFromCode(connection, code);
        }
    }
    
    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, &httpCode);
        
        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_GETINFO;
        }
    }
    
    //result checking
    if ((result == ARUTILS_OK) && (connection->cbdata.error != ARUTILS_OK))
    {
        result = connection->cbdata.error;
    }
    
    if (result == ARUTILS_OK)
    {
        // GET OK (200)
        if (httpCode == 200)
        {
        }
        else if (httpCode == 401)
        {
            result = ARUTILS_ERROR_HTTP_AUTHORIZATION_REQUIRED;
        }
        else if (httpCode == 403)
        {
            result = ARUTILS_ERROR_HTTP_ACCESS_DENIED;
        }
        else
        {
            result = ARUTILS_ERROR_HTTP_CODE;
        }
    }
    
    //cleanup
    if (formItems != NULL)
    {
        curl_formfree(formItems);
    }
    
    if (connection != NULL)
    {
        ARUTILS_Http_FreeCallbackData(&connection->cbdata);
    }
    
    return result;
}

/*****************************************
 *
 *             Private implementation:
 *
 *****************************************/

eARUTILS_ERROR ARUTILS_Http_ResetOptions(ARUTILS_Http_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    CURLcode code = CURLE_OK;

    if ((connection == NULL) || (connection->curl == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (result == ARUTILS_OK)
    {
        ARUTILS_Http_FreeCallbackData(&connection->cbdata);
    }

    if (result == ARUTILS_OK)
    {
        curl_easy_reset(connection->curl);
    }

    if (result == ARUTILS_OK)
    {
#if (ARUTILS_HTTP_CURL_VERBOSE)
        code = curl_easy_setopt(connection->curl, CURLOPT_VERBOSE, 1L);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
#endif
    }

    if ((result == ARUTILS_OK) && (connection->serverUrl != NULL))
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_URL, connection->serverUrl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if ((result == ARUTILS_OK) && (connection->username != NULL) && (strlen(connection->username) != 0))
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_USERNAME, connection->username);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if ((result == ARUTILS_OK) && (connection->password != NULL) && (strlen(connection->password) != 0))
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_PASSWORD, connection->password);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    return result;
}

size_t ARUTILS_Http_ReadDataCallback(void *ptr, size_t size, size_t nmemb, void *userData)
{
    ARUTILS_Http_Connection_t *connection = (ARUTILS_Http_Connection_t *)userData;
    size_t readSize = 0;
    size_t retSize = 0;

    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_HTTP_TAG, "%d, %d", (int)size, (int)nmemb);

    if (connection != NULL)
    {
        connection->cbdata.error = ARUTILS_Http_IsCanceled(connection);

        if (connection->cbdata.error == ARUTILS_OK)
        {
            if (connection->cbdata.file != NULL)
            {
                do
                {
                    readSize = fread(ptr, size, nmemb, connection->cbdata.file);

                    if (readSize == 0)
                    {
                        int err = ferror(connection->cbdata.file);
                        if (err != 0)
                        {
                            connection->cbdata.error = ARUTILS_ERROR_SYSTEM;
                        }
                    }
                    else
                    {
                        retSize = readSize;
                    }
                }
                while ((connection->cbdata.error == ARUTILS_OK) && (readSize == 0) && !feof(connection->cbdata.file));
            }
        }

        if (connection->cbdata.error != ARUTILS_OK)
        {
            retSize = CURL_READFUNC_ABORT;
        }
    }
    else
    {
        retSize = 0;
    }

    return retSize;
}

size_t ARUTILS_Http_WriteDataCallback(void *ptr, size_t size, size_t nmemb, void *userData)
{
    ARUTILS_Http_Connection_t *connection = (ARUTILS_Http_Connection_t *)userData;
    u_char *olddata = NULL;
    size_t retSize = 0;

    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_HTTP_TAG, "%d, %d", (int)size, (int)nmemb);

    if (connection != NULL)
    {
        connection->cbdata.error = ARUTILS_Http_IsCanceled(connection);

        if (connection->cbdata.error == ARUTILS_OK)
        {
            if (connection->cbdata.file == NULL)
            {
                olddata = connection->cbdata.data;
                connection->cbdata.data = (u_char *)realloc(connection->cbdata.data, connection->cbdata.dataSize + (size * nmemb));
                if (connection->cbdata.data == NULL)
                {
                    connection->cbdata.data = olddata;
                    connection->cbdata.error = ARUTILS_ERROR_ALLOC;
                }
            }
            else
            {
                int len = fwrite(ptr, size, nmemb, connection->cbdata.file);
                if (len != size * nmemb)
                {
                    connection->cbdata.error = ARUTILS_ERROR_SYSTEM;
                }
                else
                {
                    connection->cbdata.dataSize += size * nmemb;
                    retSize = nmemb;
                }
            }
        }

        if ((connection->cbdata.error == ARUTILS_OK) && (connection->cbdata.file == NULL) && (connection->cbdata.data != NULL))
        {
            memcpy(&connection->cbdata.data[connection->cbdata.dataSize], ptr, size * nmemb);
            connection->cbdata.dataSize += size * nmemb;
            retSize = nmemb;
        }

        if (connection->cbdata.error != ARUTILS_OK)
        {
            retSize = 0;
        }
    }
    else
    {
        retSize = 0;
    }

    return retSize;
}

int ARUTILS_Http_ProgressCallback(void *userData, double dltotal, double dlnow, double ultotal, double ulnow)
{
    ARUTILS_Http_Connection_t *connection = (ARUTILS_Http_Connection_t *)userData;
    uint8_t percent;

    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_HTTP_TAG, "%.0f, %.0f, %.0f, %.0f", dltotal, dlnow, ultotal, ulnow);

    if (connection != NULL)
    {
        if (connection->cbdata.progressCallback != NULL)
        {
            if (connection->cbdata.isUploading == 0)
            {
                // when 0, uploading isn't started
                if (dltotal != 0.f)
                {
                    percent = (dlnow / dltotal) * 100.f;
                    connection->cbdata.progressCallback(connection->cbdata.progressArg, percent);
                }
            }
            else
            {
                // when 0, downloading isn't started
                if (ultotal != 0.f)
                {
                    percent = ulnow / ultotal;
                    connection->cbdata.progressCallback(connection->cbdata.progressArg, percent);
                }
            }
        }
    }

    return 0;
}

void ARUTILS_Http_FreeCallbackData(ARUTILS_Http_CallbackData_t *cbdata)
{
    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_HTTP_TAG, "%x", cbdata);

    if (cbdata != NULL)
    {
        if (cbdata->file != NULL)
        {
            if (cbdata->file != NULL)
            {
                fclose(cbdata->file);
                cbdata->file = NULL;
            }
        }

        if (cbdata->data != NULL)
        {
            free(cbdata->data);
            cbdata->data = NULL;
        }

        memset(cbdata, 0, sizeof(ARUTILS_Http_CallbackData_t));
    }
}

eARUTILS_ERROR ARUTILS_Http_IsCanceled(ARUTILS_Http_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;

    if (connection == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if ((connection != NULL) && (connection->cancelSem != NULL))
    {
        int resultSys = ARSAL_Sem_Trywait(connection->cancelSem);

        if (resultSys == 0)
        {
            result = ARUTILS_ERROR_HTTP_CANCELED;

            //give back the signal state lost from trywait
            ARSAL_Sem_Post(connection->cancelSem);
        }
        else if (errno != EAGAIN)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }

    return result;
}

eARUTILS_ERROR ARUTILS_Http_GetErrorFromCode(ARUTILS_Http_Connection_t *connection, CURLcode code)
{
    eARUTILS_ERROR result = ARUTILS_ERROR;
    //long httpCode = 0L;
    //CURLcode codeInfo;

    switch (code)
    {
        case CURLE_WRITE_ERROR://write callback error
            result = ARUTILS_ERROR_HTTP_CODE;
            if (connection->cbdata.error != 0)
            {
                result = connection->cbdata.error;
            }
            break;

        case CURLE_COULDNT_RESOLVE_HOST:
            result = ARUTILS_ERROR_HTTP_CONNECT;
            break;

        /*case CURLE_QUOTE_ERROR: //(file or directory doesn't exist)
            result = ARUTILS_ERROR_CURL_PERFORM;
            codeInfo = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, &httpCode);

            if (codeInfo == CURLE_OK)
            {
                if (httpCode != 200)
                {
                    result = ARUTILS_ERROR_HTTP_CODE;
                }
            }
            break;*/

        default:
            result = ARUTILS_ERROR_CURL_PERFORM;
            break;
    }

    return result;
}

