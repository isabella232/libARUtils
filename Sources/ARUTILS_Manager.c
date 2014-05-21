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

#include "libARUtils/ARUTILS_Error.h"
#include "libARUtils/ARUTILS_Manager.h"

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

