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
 * @file ARUTILS_Manager.c
 * @brief libARUtils Manager c file.
 * @date 19/12/2013
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
#include "libARUtils/ARUTILS_Manager.h"
#include "libARUtils/ARUTILS_Ftp.h"
#include "ARUTILS_Manager.h"

#define ARUTILS_MANAGER_TAG "Manager"



ARUTILS_Manager_t* ARUTILS_Manager_New(eARUTILS_ERROR *error)
{
    ARUTILS_Manager_t *newManager = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_MANAGER_TAG, "");

    newManager = calloc(1, sizeof(ARUTILS_Manager_t));
    if (newManager == NULL)
    {
        result = ARUTILS_ERROR_ALLOC;
    }

    *error = result;
    return newManager;
}

void ARUTILS_Manager_Delete(ARUTILS_Manager_t **managerAddr)
{
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_MANAGER_TAG, "");

    if (managerAddr != NULL)
    {
        ARUTILS_Manager_t *manager = *managerAddr;
        if (manager != NULL)
        {
            free(manager);
        }
        *managerAddr = NULL;
    }
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Connection_Disconnect(ARUTILS_Manager_t *manager)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpConnectionDisconnect == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpConnectionDisconnect(manager);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Connection_Reconnect(ARUTILS_Manager_t *manager)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpConnectionReconnect == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpConnectionReconnect(manager);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Connection_Cancel(ARUTILS_Manager_t *manager)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpConnectionCancel == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpConnectionCancel(manager);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Connection_IsCanceled(ARUTILS_Manager_t *manager)
{
    eARUTILS_ERROR result = ARUTILS_OK;

    if ((manager == NULL) || (manager->ftpConnectionIsCanceled == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpConnectionIsCanceled(manager);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Connection_Reset(ARUTILS_Manager_t *manager)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpConnectionReset == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpConnectionReset(manager);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_List(ARUTILS_Manager_t *manager, const char *namePath, char **resultList, uint32_t *resultListLen)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpList == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpList(manager, namePath, resultList, resultListLen);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Size(ARUTILS_Manager_t *manager, const char *namePath, double *fileSize)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpSize == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpSize(manager, namePath, fileSize);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Get_WithBuffer(ARUTILS_Manager_t *manager, const char *namePath, uint8_t **data, uint32_t *dataLen,  ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpGetWithBuffer == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpGetWithBuffer(manager, namePath, data, dataLen, progressCallback, progressArg);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Get(ARUTILS_Manager_t *manager, const char *namePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpGet == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpGet(manager, namePath, dstFile, progressCallback, progressArg, resume);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Put(ARUTILS_Manager_t *manager, const char *namePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpPut == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpPut(manager, namePath, srcFile, progressCallback, progressArg, resume);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Delete(ARUTILS_Manager_t *manager, const char *namePath)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpDelete == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpDelete(manager, namePath);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_RemoveDir(ARUTILS_Manager_t *manager, const char *namePath)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpRemoveDir == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpRemoveDir(manager, namePath);
    }
    return result;
}

eARUTILS_ERROR ARUTILS_Manager_Ftp_Rename(ARUTILS_Manager_t *manager, const char *oldNamePath, const char *newNamePath)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    if ((manager == NULL) || (manager->ftpRename == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    else
    {
        result = manager->ftpRename(manager, oldNamePath, newNamePath);
    }
    return result;
}
