/**
 * @file ARUTILS_Ftp.c
 * @brief libARUtils Ftp c file.
 * @date 19/12/2013
 * @author david.flattin.ext@parrot.com
 **/

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>

#include <libARSAL/ARSAL_Sem.h>
#include <libARSAL/ARSAL_Print.h>
#include <curl/curl.h>

#include "libARUtils/ARUTILS_Error.h"
#include "libARUtils/ARUTILS_Manager.h"
#include "libARUtils/ARUTILS_Ftp.h"
#include "libARUtils/ARUTILS_FileSystem.h"
#include "ARUTILS_Manager.h"
#include "ARUTILS_WifiFtp.h"

#define ARUTILS_WIFIFTP_TAG              "WifiFtp"

#define ARUTILS_WIFIFTP_LOW_SPEED_TIME   5
#define ARUTILS_WIFIFTP_LOW_SPEED_LIMIT  1

#ifdef DEBUG
#define ARUTILS_FTP_CURL_VERBOSE         1
#endif

/*****************************************
 *
 *             Documentation:
 *
 *****************************************/

// libcurl doc
//http://curl.haxx.se/libcurl/c/curl_easy_setopt.html
// rfc ftp
//http://www.ietf.org/rfc/rfc959.txt
// ftp doc
//http://www.nsftools.com/tips/RawFTP.htm

/*****************************************
 *
 *             Public implementation:
 *
 *****************************************/

ARUTILS_WifiFtp_Connection_t * ARUTILS_WifiFtp_Connection_New(ARSAL_Sem_t *cancelSem, const char *server, int port, const char *username, const char* password, eARUTILS_ERROR *error)
{
    ARUTILS_WifiFtp_Connection_t *newConnection = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%s, %d, %s", server ? server : "null", port, username ? username : "null");

    if (server == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (result == ARUTILS_OK)
    {
        newConnection = (ARUTILS_WifiFtp_Connection_t *)calloc(1, sizeof(ARUTILS_WifiFtp_Connection_t));

        if (newConnection == NULL)
        {
            result = ARUTILS_ERROR_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        newConnection->curlSocket = -1;
        newConnection->cancelSem = cancelSem;
    }

    if (result == ARUTILS_OK)
    {
        sprintf(newConnection->serverUrl, "ftp://%s:%d/", server, port);
    }

    if ((result == ARUTILS_OK) && (username != NULL))
    {
        strncpy(newConnection->username, username, ARUTILS_FTP_MAX_USER_SIZE);
        newConnection->username[ARUTILS_FTP_MAX_USER_SIZE - 1] = '\0';
    }

    if ((result == ARUTILS_OK) && (password != NULL))
    {
        strncpy(newConnection->password, password, ARUTILS_FTP_MAX_USER_SIZE);
        newConnection->password[ARUTILS_FTP_MAX_USER_SIZE - 1] = '\0';
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
        ARUTILS_WifiFtp_Connection_Delete(&newConnection);
    }

    *error = result;
    return newConnection;
}

void ARUTILS_WifiFtp_Connection_Delete(ARUTILS_WifiFtp_Connection_t **connectionPtrAddr)
{
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "");

    if (connectionPtrAddr != NULL)
    {
        ARUTILS_WifiFtp_Connection_t *connection = *connectionPtrAddr;

        if (connection != NULL)
        {
            if (connection->curl != NULL)
            {
                curl_easy_cleanup(connection->curl);
            }

            ARUTILS_WifiFtp_FreeCallbackData(&connection->cbdata);

            free(connection);
            *connectionPtrAddr = NULL;
        }
    }
}

eARUTILS_ERROR ARUTILS_WifiFtp_Connection_Disconnect(ARUTILS_WifiFtp_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (connection != NULL)
    {
        if (connection->curl != NULL)
        {
            curl_easy_cleanup(connection->curl);
            connection->curl = NULL;
        }
        else
        {
            result = ARUTILS_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Connection_Reconnect(ARUTILS_WifiFtp_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (connection != NULL)
    {
        if (connection->curl == NULL)
        {
            connection->curl = curl_easy_init();
            
            if (connection->curl == NULL)
            {
                result = ARUTILS_ERROR_CURL_ALLOC;
            }
        }
        else
        {
            result = ARUTILS_ERROR_BAD_PARAMETER;
        }
    }
    else
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Connection_Cancel(ARUTILS_WifiFtp_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    int resutlSys = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "");

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
    
    if (result == ARUTILS_OK)
    {
        if (connection->curlSocket != -1)
        {
            shutdown(connection->curlSocket, SHUT_RDWR);
        }
    }

    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Connection_Reset(ARUTILS_WifiFtp_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "");

    if ((connection == NULL) || (connection->cancelSem == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (result == ARUTILS_OK)
    {
        while (ARSAL_Sem_Trywait(connection->cancelSem) == 0)
        {
            /* Do nothing*/
        }
    }

    return result;
}


eARUTILS_ERROR ARUTILS_WifiFtp_List(ARUTILS_WifiFtp_Connection_t *connection, const char *namePath, char **resultList, uint32_t *resultListLen)
{
    char fileUrl[ARUTILS_FTP_MAX_URL_SIZE];
    char cmd[ARUTILS_FTP_MAX_PATH_SIZE];
    eARUTILS_ERROR result = ARUTILS_OK;
    struct curl_slist *slist = NULL;
    CURLcode code = CURLE_OK;
    long ftpCode = 0L;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%s", namePath ? namePath : "null");

    if ((connection == NULL) || (connection->curl == NULL) || (resultList == NULL) || (resultListLen == NULL))
    {
        result =  ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        *resultList = NULL;
        *resultListLen = 0;
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_IsCanceled(connection);
    }

    // check that the given folder exist on the server
    if (result == ARUTILS_OK)
    {
        if (*namePath == '\0')
        {
            strncpy(fileUrl, "/", ARUTILS_FTP_MAX_URL_SIZE);
            fileUrl[ARUTILS_FTP_MAX_URL_SIZE - 1] = '\0';
        }
        else
        {
            strncpy(fileUrl, namePath, ARUTILS_FTP_MAX_URL_SIZE);
            fileUrl[ARUTILS_FTP_MAX_URL_SIZE - 1] = '\0';
        }

        result = ARUTILS_WifiFtp_Cd(connection, fileUrl);

        if (result == ARUTILS_OK)
        {
            result = ARUTILS_WifiFtp_Cd(connection, "/");
        }
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_ResetOptions(connection);
    }

    if (result == ARUTILS_OK)
    {
        strncpy(fileUrl, connection->serverUrl, ARUTILS_FTP_MAX_URL_SIZE);
        fileUrl[ARUTILS_FTP_MAX_URL_SIZE - 1] = '\0';
        strncat(fileUrl, namePath, ARUTILS_FTP_MAX_URL_SIZE - strlen(fileUrl) - 1);
        if ((namePath != NULL) && (strlen(namePath) > 0))
        {
            strncat(fileUrl, "/", ARUTILS_FTP_MAX_URL_SIZE - strlen(fileUrl) - 1);
        }

        code = curl_easy_setopt(connection->curl, CURLOPT_URL, fileUrl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        strncpy(cmd, FTP_CMD_LIST, ARUTILS_FTP_MAX_PATH_SIZE);
        cmd[ARUTILS_FTP_MAX_PATH_SIZE - 1] = '\0';

        slist = curl_slist_append(slist, cmd);
        if (slist == NULL)
        {
            result = ARUTILS_ERROR_CURL_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_PREQUOTE, slist);
        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
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
        code = curl_easy_setopt(connection->curl, CURLOPT_WRITEFUNCTION, ARUTILS_WifiFtp_WriteDataCallback);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    //libcurl process
    if (result == ARUTILS_OK)
    {
        code = curl_easy_perform(connection->curl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_WifiFtp_GetErrorFromCode(connection, code);
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, &ftpCode);

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
        // NLST OK (226), LIST OK (226)
        if (ftpCode != 226)
        {
            result = ARUTILS_ERROR_FTP_CODE;
        }
    }

    if (result == ARUTILS_OK)
    {
        u_char *oldData = connection->cbdata.data;
        connection->cbdata.data = (u_char *)realloc(connection->cbdata.data, (connection->cbdata.dataSize + 1) * sizeof(char));
        if (connection->cbdata.data != NULL)
        {
            connection->cbdata.data[connection->cbdata.dataSize] = '\0';
            connection->cbdata.dataSize++;
        }
        else
        {
            connection->cbdata.data = oldData;
            result = ARUTILS_ERROR_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        *resultList = (char*)connection->cbdata.data;
        *resultListLen = connection->cbdata.dataSize;
        connection->cbdata.data = NULL;
        connection->cbdata.dataSize = 0;
    }

    //cleanup
    if (connection != NULL)
    {
        ARUTILS_WifiFtp_FreeCallbackData(&connection->cbdata);
    }

    if (slist != NULL)
    {
        curl_slist_free_all(slist);
    }

    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Rename(ARUTILS_WifiFtp_Connection_t *connection, const char *oldNamePath, const char *newNamePath)
{
    struct curl_slist *slist = NULL;
    char cmdRnfr[ARUTILS_FTP_MAX_PATH_SIZE];
    char cmdRnto[ARUTILS_FTP_MAX_PATH_SIZE];
    eARUTILS_ERROR result = ARUTILS_OK;
    CURLcode code = CURLE_OK;
    long ftpCode = 0L;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%s, %s", oldNamePath ? oldNamePath : "null", newNamePath ? newNamePath : "null");

    if ((connection == NULL) || (connection->curl == NULL) || (oldNamePath == NULL) || (newNamePath == NULL))
    {
        result =  ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_IsCanceled(connection);
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_ResetOptions(connection);
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_NOBODY, 1L);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        strncpy(cmdRnfr, FTP_CMD_RNFR, ARUTILS_FTP_MAX_PATH_SIZE);
        cmdRnfr[ARUTILS_FTP_MAX_PATH_SIZE - 1] = '\0';
        strncat(cmdRnfr, oldNamePath, ARUTILS_FTP_MAX_PATH_SIZE - strlen(cmdRnfr) - 1);

        slist = curl_slist_append(slist, cmdRnfr);

        if (slist == NULL)
        {
            result = ARUTILS_ERROR_CURL_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        strncpy(cmdRnto, FTP_CMD_RNTO, ARUTILS_FTP_MAX_PATH_SIZE);
        cmdRnto[ARUTILS_FTP_MAX_PATH_SIZE - 1] = '\0';
        strncat(cmdRnto, newNamePath, ARUTILS_FTP_MAX_PATH_SIZE - strlen(cmdRnto) - 1);

        slist = curl_slist_append(slist, cmdRnto);

        if (slist == NULL)
        {
            result = ARUTILS_ERROR_CURL_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_POSTQUOTE, slist);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
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
        code = curl_easy_setopt(connection->curl, CURLOPT_WRITEFUNCTION, ARUTILS_WifiFtp_WriteDataCallback);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    //libcurl process
    if (result == ARUTILS_OK)
    {
        code = curl_easy_perform(connection->curl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_WifiFtp_GetErrorFromCode(connection, code);
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, &ftpCode);

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
        //RNFR OK (350), RNTO OK (250)
        if (ftpCode != 250)
        {
            result = ARUTILS_ERROR_FTP_CODE;
        }
    }

    //cleanup
    if (connection != NULL)
    {
        ARUTILS_WifiFtp_FreeCallbackData(&connection->cbdata);
    }

    if (slist != NULL)
    {
        curl_slist_free_all(slist);
    }

    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Size(ARUTILS_WifiFtp_Connection_t *connection, const char *namePath, double *fileSize)
{
    struct curl_slist *slist = NULL;
    char cmd[ARUTILS_FTP_MAX_PATH_SIZE];
    char fileUrl[ARUTILS_FTP_MAX_URL_SIZE];
    eARUTILS_ERROR result = ARUTILS_OK;
    CURLcode code = CURLE_OK;
    long ftpCode = 0L;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%s", namePath ? namePath : "null");

    if ((connection == NULL) || (connection->curl == NULL) || (namePath == NULL) || (fileSize == NULL))
    {
        result =  ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        *fileSize = 0.f;
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_IsCanceled(connection);
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_ResetOptions(connection);
    }

    if (result == ARUTILS_OK)
    {
        strncpy(fileUrl, connection->serverUrl, ARUTILS_FTP_MAX_URL_SIZE);
        fileUrl[ARUTILS_FTP_MAX_URL_SIZE - 1] = '\0';
        strncat(fileUrl, namePath, ARUTILS_FTP_MAX_URL_SIZE - strlen(fileUrl) - 1);
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_URL, fileUrl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    // post process the SIZE command to get its status code, but get the file size form curl content-lenght mechanism
    if (result == ARUTILS_OK)
    {
        const char *idx = namePath;
        const char *name;
        strncpy(cmd, FTP_CMD_SIZE, ARUTILS_FTP_MAX_PATH_SIZE);
        cmd[ARUTILS_FTP_MAX_PATH_SIZE - 1] = '\0';

        do
        {
            if (*idx == '/')
                idx++;

            name = idx;
        }
        while ((idx = strstr(idx, "/")) != NULL);

        strncat(cmd, name, ARUTILS_FTP_MAX_PATH_SIZE - strlen(cmd) - 1);

        slist = curl_slist_append(slist, cmd);

        if (slist == NULL)
        {
            result = ARUTILS_ERROR_CURL_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_POSTQUOTE, slist);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_NOBODY, 1L);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
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
        code = curl_easy_setopt(connection->curl, CURLOPT_WRITEFUNCTION, ARUTILS_WifiFtp_WriteDataCallback);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_perform(connection->curl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_WifiFtp_GetErrorFromCode(connection, code);
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, &ftpCode);

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
        //SIZE OK (213) REST OK (350)
        if (ftpCode != 213)
        {
            result = ARUTILS_ERROR_FTP_CODE;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, fileSize);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_GETINFO;
        }
        else if (*fileSize == -1.0f)
        {
            result = ARUTILS_ERROR_FTP_CODE;
        }
    }

    //cleanup
    if (connection != NULL)
    {
        ARUTILS_WifiFtp_FreeCallbackData(&connection->cbdata);
    }

    if (slist != NULL)
    {
        curl_slist_free_all(slist);
    }

    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Command(ARUTILS_WifiFtp_Connection_t *connection, const char *namePath, const char *command, long *ftpCode)
{
    struct curl_slist *slist = NULL;
    char cmd[ARUTILS_FTP_MAX_PATH_SIZE];
    eARUTILS_ERROR result = ARUTILS_OK;
    CURLcode code = CURLE_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%s, %s", namePath ? namePath : "null", command ? command : "null");

    if ((connection == NULL) || (connection->curl == NULL) || (namePath == NULL) || (command == NULL) || (ftpCode == NULL))
    {
        result =  ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_IsCanceled(connection);
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_ResetOptions(connection);
    }

    if (result == ARUTILS_OK)
    {
        strncpy(cmd, command, ARUTILS_FTP_MAX_PATH_SIZE);
        cmd[ARUTILS_FTP_MAX_PATH_SIZE - 1] = '\0';
        strncat(cmd, namePath, ARUTILS_FTP_MAX_PATH_SIZE - strlen(cmd) - 1);

        slist = curl_slist_append(slist, cmd);

        if (slist == NULL)
        {
            result = ARUTILS_ERROR_CURL_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_POSTQUOTE, slist);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_NOBODY, 1L);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
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
        code = curl_easy_setopt(connection->curl, CURLOPT_WRITEFUNCTION, ARUTILS_WifiFtp_WriteDataCallback);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    //libcurl process
    if (result == ARUTILS_OK)
    {
        code = curl_easy_perform(connection->curl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_WifiFtp_GetErrorFromCode(connection, code);
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, ftpCode);

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

    //cleanup
    if (connection != NULL)
    {
        ARUTILS_WifiFtp_FreeCallbackData(&connection->cbdata);
    }

    if (slist != NULL)
    {
        curl_slist_free_all(slist);
    }

    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Delete(ARUTILS_WifiFtp_Connection_t *connection, const char *namePath)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    long ftpCode = 0L;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%s", namePath ? namePath : "null");

    result = ARUTILS_WifiFtp_Command(connection, namePath, FTP_CMD_DELE, &ftpCode);

    if (result == ARUTILS_OK)
    {
        //DELE OK (250)
        if (ftpCode != 250)
        {
            result = ARUTILS_ERROR_FTP_CODE;
        }
    }

    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Cd(ARUTILS_WifiFtp_Connection_t *connection, const char *namePath)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    long ftpCode = 0L;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%s", namePath ? namePath : "null");

    result = ARUTILS_WifiFtp_Command(connection, namePath, FTP_CMD_CWD, &ftpCode);

    if (result == ARUTILS_OK)
    {
        //CWD OK (250)
        if (ftpCode != 250)
        {
            result = ARUTILS_ERROR_FTP_CODE;
        }
    }

    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Get(ARUTILS_WifiFtp_Connection_t *connection, const char *namePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    return ARUTILS_WifiFtp_GetInternal(connection, namePath, dstFile, NULL, NULL, progressCallback, progressArg, resume);
}

eARUTILS_ERROR ARUTILS_WifiFtp_Get_WithBuffer(ARUTILS_WifiFtp_Connection_t *connection, const char *namePath, uint8_t **data, uint32_t *dataLen, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg)
{
    return ARUTILS_WifiFtp_GetInternal(connection, namePath, NULL, data, dataLen, progressCallback, progressArg, FTP_RESUME_FALSE);
}

eARUTILS_ERROR ARUTILS_WifiFtp_GetInternal(ARUTILS_WifiFtp_Connection_t *connection, const char *namePath, const char *dstFile, uint8_t **data, uint32_t *dataLen, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    char fileUrl[ARUTILS_FTP_MAX_URL_SIZE];
    eARUTILS_ERROR resultResume = ARUTILS_ERROR;
    eARUTILS_ERROR result = ARUTILS_OK;
    CURLcode code = CURLE_OK;
    long ftpCode = 0L;
    double remoteSize = 0.f;
    uint32_t localSize = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%s, %s, %d", namePath ? namePath : "null", dstFile ? dstFile : "null", resume);

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
        if ((data == NULL) || (dataLen == NULL) || (resume != FTP_RESUME_FALSE))
        {
            result =  ARUTILS_ERROR_BAD_PARAMETER;
        }
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_IsCanceled(connection);
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_Size(connection, namePath, &remoteSize);
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_ResetOptions(connection);
    }

    if ((result == ARUTILS_OK) && (resume == FTP_RESUME_TRUE))
    {
        resultResume = ARUTILS_FileSystem_GetFileSize(dstFile, &localSize);

        if ((resultResume == ARUTILS_OK)
            || (resultResume == ARUTILS_ERROR_FILE_NOT_FOUND))
        {
            if (localSize <= (uint32_t)remoteSize)
            {
                code = curl_easy_setopt(connection->curl, CURLOPT_RESUME_FROM, (long)localSize);

                if (code != CURLE_OK)
                {
                    result = ARUTILS_ERROR_CURL_SETOPT;
                }
            }
            else
            {
                result = ARUTILS_ERROR_FTP_RESUME;
            }
        }
    }

    if (result == ARUTILS_OK)
    {
        strncpy(fileUrl, connection->serverUrl, ARUTILS_FTP_MAX_URL_SIZE);
        fileUrl[ARUTILS_FTP_MAX_URL_SIZE - 1] = '\0';
        strncat(fileUrl, namePath, ARUTILS_FTP_MAX_URL_SIZE - strlen(fileUrl) - 1);

        code = curl_easy_setopt(connection->curl, CURLOPT_URL, fileUrl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if ((result == ARUTILS_OK) && (dstFile != NULL))
    {
        if ((resultResume == ARUTILS_OK) && (resume == FTP_RESUME_TRUE))
        {
            connection->cbdata.file = fopen(dstFile, "ab");
        }
        else
        {
            connection->cbdata.file = fopen(dstFile, "wb");
        }

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
        code = curl_easy_setopt(connection->curl, CURLOPT_WRITEFUNCTION, ARUTILS_WifiFtp_WriteDataCallback);

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
            code = curl_easy_setopt(connection->curl, CURLOPT_PROGRESSFUNCTION, ARUTILS_WifiFtp_ProgressCallback);

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
            result = ARUTILS_WifiFtp_GetErrorFromCode(connection, code);
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, &ftpCode);

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
        // GET OK (226)
        if (ftpCode == 226)
        {
            if (dstFile != NULL)
            {
                fflush(connection->cbdata.file);
            }
        }
        else if ((resultResume == ARUTILS_OK)
                 && (resume == FTP_RESUME_TRUE) && (localSize == (uint32_t)remoteSize)
                 && (ftpCode == 213))
        {
            //resume ok
            if (dstFile != NULL)
            {
                fflush(connection->cbdata.file);
            }
        }
        else
        {
            result = ARUTILS_ERROR_FTP_CODE;
        }
    }

    if ((result == ARUTILS_OK) && (dstFile != NULL))
    {
        result = ARUTILS_FileSystem_GetFileSize(dstFile, &localSize);

        if (result == ARUTILS_OK)
        {
            if (localSize != (uint32_t)remoteSize)
            {
                result = ARUTILS_ERROR_FTP_SIZE;
            }
        }
    }

    if ((result == ARUTILS_OK) && (data != NULL) && (dataLen != NULL))
    {
        if (result == ARUTILS_OK)
        {
            if (connection->cbdata.dataSize != (uint32_t)remoteSize)
            {
                result = ARUTILS_ERROR_FTP_SIZE;
            }
        }

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
        ARUTILS_WifiFtp_FreeCallbackData(&connection->cbdata);
    }

    return result;
}

eARUTILS_ERROR ARUTILS_WifiFtp_Put(ARUTILS_WifiFtp_Connection_t *connection, const char *namePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    char fileUrl[ARUTILS_FTP_MAX_URL_SIZE];
    eARUTILS_ERROR resultResume = ARUTILS_ERROR;
    eARUTILS_ERROR result = ARUTILS_OK;
    CURLcode code = CURLE_OK;
    long ftpCode = 0L;
    double remoteSize = 0.f;
    uint32_t localSize = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%s, %s, %d", namePath ? namePath : "null", srcFile ? srcFile : "null", resume);

    if ((connection == NULL) || (connection->curl == NULL) || (namePath == NULL) || (srcFile == NULL))
    {
        result =  ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_IsCanceled(connection);
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_FileSystem_GetFileSize(srcFile, &localSize);
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_ResetOptions(connection);
    }

    if ((result == ARUTILS_OK) && (resume == FTP_RESUME_TRUE))
    {
        resultResume = ARUTILS_WifiFtp_Size(connection, namePath, &remoteSize);

        if (result == ARUTILS_OK)
        {
            result = ARUTILS_WifiFtp_ResetOptions(connection);
        }

        if (resultResume == ARUTILS_OK)
        {
            if (localSize >= (uint32_t)remoteSize)
            {
                code = curl_easy_setopt(connection->curl, CURLOPT_RESUME_FROM, (long)remoteSize);

                if (code != CURLE_OK)
                {
                    result = ARUTILS_ERROR_CURL_SETOPT;
                }
            }
            else
            {
                result = ARUTILS_ERROR_FTP_RESUME;
            }
        }
    }

    if (result == ARUTILS_OK)
    {
        strncpy(fileUrl, connection->serverUrl, ARUTILS_FTP_MAX_URL_SIZE);
        fileUrl[ARUTILS_FTP_MAX_URL_SIZE - 1] = '\0';
        strncat(fileUrl, namePath, ARUTILS_FTP_MAX_URL_SIZE - strlen(fileUrl) - 1);

        code = curl_easy_setopt(connection->curl, CURLOPT_URL, fileUrl);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        connection->cbdata.file = fopen(srcFile, "rb");

        if (connection->cbdata.file == NULL)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }

        if ((result == ARUTILS_OK) && (resultResume == ARUTILS_OK) && (resume == FTP_RESUME_TRUE))
        {
            int fileResult = fseek(connection->cbdata.file, (long)remoteSize, SEEK_SET);

            if (fileResult != 0)
            {
                result = ARUTILS_ERROR_SYSTEM;
            }
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_INFILESIZE, (long)localSize);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        connection->cbdata.isUploading = 1;

        code = curl_easy_setopt(connection->curl, CURLOPT_UPLOAD, 1L);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
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
        code = curl_easy_setopt(connection->curl, CURLOPT_READFUNCTION, ARUTILS_WifiFtp_ReadDataCallback);

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
            code = curl_easy_setopt(connection->curl, CURLOPT_PROGRESSFUNCTION, ARUTILS_WifiFtp_ProgressCallback);

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
            result = ARUTILS_WifiFtp_GetErrorFromCode(connection, code);
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, &ftpCode);

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
        // PUT OK (226)
        if (ftpCode == 226)
        {
            //ok
        }
        else if ((resultResume == ARUTILS_OK)
                 && (resume == FTP_RESUME_TRUE) && (localSize == (uint32_t)remoteSize)
                 && (ftpCode == 229))
        {
            //ok
        }
        else
        {
            result = ARUTILS_ERROR_FTP_CODE;
        }
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_Size(connection, namePath, &remoteSize);
    }

    //cleanup
    if (connection != NULL)
    {
        ARUTILS_WifiFtp_FreeCallbackData(&connection->cbdata);
    }

    if (result == ARUTILS_OK)
    {
        if (localSize != (uint32_t)remoteSize)
        {
            result = ARUTILS_ERROR_FTP_SIZE;
        }
    }

    return result;
}

/* Drone3
drwxr-xr-x    4 0        0            32768 Jan  1  1980 AR.Drone\n
*/
/* Drone2
drwxr-xr-x    2 0        0             160 Jan  1  2000 data
-rw-r--r--    1 0        0               7 Jan  1  2000 data_20131001_235901.pub
*/
/* vsftp
drwxr-xr-x    4 122      128          4096 Jan 24 14:34 AR.Drone
drwxr-xr-x    4 122      128          4096 Jan 24 14:34 Jumping Sumo
*/
const char * ARUTILS_Ftp_List_GetNextItem(const char *list, const char **nextItem, const char *prefix, int isDirectory, const char **indexItem, int *itemLen)
{
    static char lineData[ARUTILS_FTP_MAX_LIST_LINE_SIZE];
    char *item = NULL;
    const char *line = NULL;
    const char *fileIdx;
    const char *endLine = NULL;
    const char *ptr;

    if ((list != NULL) && (nextItem != NULL))
    {
        if (*nextItem == NULL)
        {
            *nextItem = list;
        }

        ptr = *nextItem;
        while ((item == NULL) && (ptr != NULL))
        {
            line = *nextItem;
            endLine = line;
            ptr = strchr(line, '\n');
            if (ptr == NULL)
            {
                ptr = strchr(line, '\r');
            }

            if (ptr != NULL)
            {
                endLine = ptr;
                if (*(endLine - 1) == '\r')
                {
                    endLine--;
                }

                ptr++;
                *nextItem = ptr;
                fileIdx = line;
                if (*line == ((isDirectory  == 1) ? 'd' : '-'))
                {
                    int varSpace = 0;
                    while (((ptr = strchr(fileIdx, '\x20')) != NULL) && (ptr < endLine) && (varSpace < 8))
                    {
                        if (*(ptr + 1) != '\x20')
                        {
                            varSpace++;
                        }

                        fileIdx = ++ptr;
                    }

                    if ((prefix != NULL) && (*prefix != '\0'))
                    {
                        if (strncmp(fileIdx, prefix, strlen(prefix)) != 0)
                        {
                            fileIdx = NULL;
                        }
                    }

                    if (fileIdx != NULL)
                    {
                        int len = ((endLine - fileIdx) < ARUTILS_FTP_MAX_LIST_LINE_SIZE) ? (endLine - fileIdx) : (ARUTILS_FTP_MAX_LIST_LINE_SIZE - 1);
                        strncpy(lineData, fileIdx, len);
                        lineData[len] = '\0';
                        item = lineData;
                    }
                }
            }
        }

        if (indexItem != NULL)
        {
            *indexItem = line;
        }

        if (itemLen != NULL)
        {
            *itemLen = endLine - line;
        }
    }

    return item;
}

const char * ARUTILS_Ftp_List_GetItemSize(const char *line, int lineSize, double *size)
{
    const char *fileIdx;
    const char *sizeIdx;
    const char *endLine;
    const char *ptr;
    const char *item = NULL;
    int varSpace = 0;
    int len;

    if ((line != NULL) && (size != NULL))
    {
        *size = 0.f;
        endLine = line + lineSize;
        sizeIdx = NULL;
        fileIdx = line;
        while ((ptr = strchr(fileIdx, '\x20')) != NULL && (ptr < endLine) && (varSpace < 3))
        {
            if ((*(ptr - 1) == '\x20') && (*(ptr + 1) != '\x20'))
            {
                varSpace++;
                if ((*line == '-'))
                {
                    if ((varSpace == 3) && (sizeIdx == NULL))
                    {
                        sizeIdx = ptr + 1;
                        len = sscanf(sizeIdx, "%lf", size);

                        if (len <= 0)
                        {
                            *size = 0.f;
                        }

                        item = sizeIdx;
                    }
                }
            }
            fileIdx = ++ptr;
        }
    }

    return item;
}

/*****************************************
 *
 *             Private implementation:
 *
 *****************************************/

eARUTILS_ERROR ARUTILS_WifiFtp_ResetOptions(ARUTILS_WifiFtp_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    CURLcode code = CURLE_OK;

    if ((connection == NULL) || (connection->curl == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (result == ARUTILS_OK)
    {
        ARUTILS_WifiFtp_FreeCallbackData(&connection->cbdata);
    }

    if (result == ARUTILS_OK)
    {
        curl_easy_reset(connection->curl);
    }

    if (result == ARUTILS_OK)
    {
#if (ARUTILS_FTP_CURL_VERBOSE)
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

    if ((result == ARUTILS_OK) && (connection->username != NULL))
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_USERNAME, connection->username);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if ((result == ARUTILS_OK) && (connection->password != NULL))
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_PASSWORD, connection->password);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_NOSIGNAL, 1);
        
        if ((code != CURLE_OK) && (code != CURLE_UNKNOWN_OPTION))
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_MAXCONNECTS, 1);
        
        if ((code != CURLE_OK) && (code != CURLE_UNKNOWN_OPTION))
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_LOW_SPEED_LIMIT, ARUTILS_WIFIFTP_LOW_SPEED_LIMIT);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_LOW_SPEED_TIME, ARUTILS_WIFIFTP_LOW_SPEED_TIME);

        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_OPENSOCKETFUNCTION, ARUTILS_WifiFtp_OpensocketCallback);
        
        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }
    
    if (result == ARUTILS_OK)
    {
        code = curl_easy_setopt(connection->curl, CURLOPT_OPENSOCKETDATA, &connection->curlSocket);
        
        if (code != CURLE_OK)
        {
            result = ARUTILS_ERROR_CURL_SETOPT;
        }
    }

    return result;
}

size_t ARUTILS_WifiFtp_ReadDataCallback(void *ptr, size_t size, size_t nmemb, void *userData)
{
    ARUTILS_WifiFtp_Connection_t *connection = (ARUTILS_WifiFtp_Connection_t *)userData;
    size_t readSize = 0;
    size_t retSize = 0;

    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%d, %d", (int)size, (int)nmemb);

    if (connection != NULL)
    {
        connection->cbdata.error = ARUTILS_WifiFtp_IsCanceled(connection);

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

size_t ARUTILS_WifiFtp_WriteDataCallback(void *ptr, size_t size, size_t nmemb, void *userData)
{
    ARUTILS_WifiFtp_Connection_t *connection = (ARUTILS_WifiFtp_Connection_t *)userData;
    u_char *olddata = NULL;
    size_t retSize = 0;

    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%d, %d", (int)size, (int)nmemb);

    if (connection != NULL)
    {
        connection->cbdata.error = ARUTILS_WifiFtp_IsCanceled(connection);

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

int ARUTILS_WifiFtp_ProgressCallback(void *userData, double dltotal, double dlnow, double ultotal, double ulnow)
{
    ARUTILS_WifiFtp_Connection_t *connection = (ARUTILS_WifiFtp_Connection_t *)userData;
    float percent;

    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%.0f, %.0f, %.0f, %.0f", dltotal, dlnow, ultotal, ulnow);

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
                    percent = (ulnow / ultotal) * 100.f;
                    connection->cbdata.progressCallback(connection->cbdata.progressArg, percent);
                }
            }
        }
    }

    return 0;
}

curl_socket_t ARUTILS_WifiFtp_OpensocketCallback(void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
{
    curl_socket_t sock = 0;
    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%x", clientp);
    
    if ((address != NULL) && (purpose == CURLSOCKTYPE_IPCXN))
    {
        sock = socket(address->family, address->socktype, address->protocol);
        
        if (clientp != NULL)
        {
            *((int*)clientp) = sock;
        }
    }
    
    return sock;
}

void ARUTILS_WifiFtp_FreeCallbackData(ARUTILS_WifiFtp_CallbackData_t *cbdata)
{
    //ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_WIFIFTP_TAG, "%x", cbdata);

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

        memset(cbdata, 0, sizeof(ARUTILS_WifiFtp_CallbackData_t));
    }
}

eARUTILS_ERROR ARUTILS_WifiFtp_IsCanceled(ARUTILS_WifiFtp_Connection_t *connection)
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
            result = ARUTILS_ERROR_FTP_CANCELED;

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

eARUTILS_ERROR ARUTILS_WifiFtp_GetErrorFromCode(ARUTILS_WifiFtp_Connection_t *connection, CURLcode code)
{
    eARUTILS_ERROR result = ARUTILS_ERROR;
    long ftpCode = 0L;
    CURLcode codeInfo;

    switch (code)
    {
        case CURLE_WRITE_ERROR://write callback error
            result = ARUTILS_ERROR_FTP_CODE;
            if (connection->cbdata.error != 0)
            {
                result = connection->cbdata.error;
            }
            break;

        case CURLE_COULDNT_RESOLVE_HOST:
            result = ARUTILS_ERROR_FTP_CONNECT;
            break;

        case CURLE_QUOTE_ERROR: //(file or directory doesn't exist)
            result = ARUTILS_ERROR_CURL_PERFORM;
            codeInfo = curl_easy_getinfo(connection->curl, CURLINFO_RESPONSE_CODE, &ftpCode);

            if (codeInfo == CURLE_OK)
            {
                if (ftpCode == 550)
                {
                    result = ARUTILS_ERROR_FTP_CODE;
                }
            }
            break;

        default:
            result = ARUTILS_ERROR_CURL_PERFORM;
            break;
    }

    return result;
}

/*****************************************
 *
 *             Abstract implementation:
 *
 *****************************************/

eARUTILS_ERROR ARUTILS_Manager_InitWifiFtp(ARUTILS_Manager_t *manager, const char *server, int port, const char *username, const char* password)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    int resultSys = 0;

    if ((manager == NULL) || (manager->connectionObject != NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (result == ARUTILS_OK)
    {
        resultSys = ARSAL_Sem_Init(&manager->cancelSem, 0, 0);
        if (resultSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }

    if (result == ARUTILS_OK)
    {
        manager->connectionObject = ARUTILS_WifiFtp_Connection_New(&manager->cancelSem, server, port, username, password, &result);
    }

    if (result == ARUTILS_OK)
    {
        manager->ftpConnectionDisconnect = ARUTILS_WifiFtpAL_Connection_Disconnect;
        manager->ftpConnectionReconnect = ARUTILS_WifiFtpAL_Connection_Reconnect;
        manager->ftpConnectionCancel = ARUTILS_WifiFtpAL_Connection_Cancel;
        manager->ftpConnectionIsCanceled = ARUTILS_WifiFtpAL_Connection_IsCanceled;
        manager->ftpConnectionReset = ARUTILS_WifiFtpAL_Connection_Reset;

        manager->ftpList = ARUTILS_WifiFtpAL_List;
        manager->ftpGetWithBuffer = ARUTILS_WifiFtpAL_Get_WithBuffer;
        manager->ftpGet = ARUTILS_WifiFtpAL_Get;
        manager->ftpPut = ARUTILS_WifiFtpAL_Put;
        manager->ftpDelete = ARUTILS_WifiFtpAL_Delete;
        manager->ftpRename = ARUTILS_WifiFtpAL_Rename;
    }

    return result;
}

void ARUTILS_Manager_CloseWifiFtp(ARUTILS_Manager_t *manager)
{
    if (manager != NULL)
    {
        ARUTILS_WifiFtp_Connection_Delete((ARUTILS_WifiFtp_Connection_t **)&manager->connectionObject);

        ARSAL_Sem_Destroy(&manager->cancelSem);
    }
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Connection_Disconnect(ARUTILS_Manager_t *manager)
{
    return ARUTILS_WifiFtp_Connection_Disconnect((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Connection_Reconnect(ARUTILS_Manager_t *manager)
{
    return ARUTILS_WifiFtp_Connection_Reconnect((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Connection_Cancel(ARUTILS_Manager_t *manager)
{
    return ARUTILS_WifiFtp_Connection_Cancel((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Connection_IsCanceled(ARUTILS_Manager_t *manager)
{
    return ARUTILS_WifiFtp_IsCanceled((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Connection_Reset(ARUTILS_Manager_t *manager)
{
    return ARUTILS_WifiFtp_Connection_Reset((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_List(ARUTILS_Manager_t *manager, const char *namePath, char **resultList, uint32_t *resultListLen)
{
    return ARUTILS_WifiFtp_List((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject, namePath, resultList, resultListLen);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Get_WithBuffer(ARUTILS_Manager_t *manager, const char *namePath, uint8_t **data, uint32_t *dataLen,  ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg)
{
    return ARUTILS_WifiFtp_Get_WithBuffer((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject, namePath, data, dataLen, progressCallback, progressArg);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Get(ARUTILS_Manager_t *manager, const char *namePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    return ARUTILS_WifiFtp_Get((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject, namePath, dstFile, progressCallback, progressArg, resume);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Put(ARUTILS_Manager_t *manager, const char *namePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    return ARUTILS_WifiFtp_Put((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject, namePath, srcFile, progressCallback, progressArg, resume);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Delete(ARUTILS_Manager_t *manager, const char *namePath)
{
    return ARUTILS_WifiFtp_Delete((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject, namePath);
}

eARUTILS_ERROR ARUTILS_WifiFtpAL_Rename(ARUTILS_Manager_t *manager, const char *oldNamePath, const char *newNamePath)
{
    return ARUTILS_WifiFtp_Rename((ARUTILS_WifiFtp_Connection_t *)manager->connectionObject, oldNamePath, newNamePath);
}
