/**
 * @file ARUTILS_Http.h
 * @brief libARUtils Http header file.
 * @date 26/12/2013
 * @author david.flattin.ext@parrot.com
 **/

#ifndef _ARUTILS_HTTP_H_
#define _ARUTILS_HTTP_H_

#include <inttypes.h>
#include <libARSAL/ARSAL_Sem.h>

#include "libARUtils/ARUTILS_Error.h"

/**
 * @brief Ftp max url string size
 */
#define ARUTILS_HTTP_MAX_URL_SIZE      512

/**
 * @brief Http secured enum
 * @see ARUTILS_Http_Connection_New
 */
typedef enum
{
    HTTPS_PROTOCOL_FALSE = 0,
    HTTPS_PROTOCOL_TRUE,

} eARUTILS_HTTPS_PROTOCOL;

/**
 * @brief Http Connection structure
 * @see ARUTILS_Http_NewConnection
 */
typedef struct ARUTILS_Http_Connection_t ARUTILS_Http_Connection_t;

/**
 * @brief Progress callback of the Http download
 * @param arg The pointer of the user custom argument
 * @param percent The percent size of the media file already downloaded
 * @see ARUTILS_Http_Get ()
 */
typedef void (*ARUTILS_Http_ProgressCallback_t)(void* arg, uint8_t percent);

/**
 * @brief Create a new Http Connection
 * @warning This function allocates memory
 * @param cancelSem The pointer of the Http get/put cancel semaphore or null
 * @param server The Http server IP address
 * @param port The Http server port
 * @param security The security flag: to indicate HTTP or HTTPS connection
 * @param username The Http server account name
 * @param password The Http server account password
 * @param[out] error The pointer of the error code: if success ARUTILS_OK, otherwise an error number of eARUTILS_ERROR
 * @retval On success, returns an ARUTILS_Http_Connection_t. Otherwise, it returns null.
 * @see ARUTILS_Http_DeleteConnection ()
 */
ARUTILS_Http_Connection_t * ARUTILS_Http_Connection_New(ARSAL_Sem_t *cancelSem, const char *server, int port, eARUTILS_HTTPS_PROTOCOL security, const char *username, const char* password, eARUTILS_ERROR *error);

/**
 * @brief Delete an Http Connection
 * @warning This function frees memory
 * @param connection The address of the pointer on the Http Connection
 * @see ARUTILS_Http_NewConnection ()
 */
void ARUTILS_Http_Connection_Delete(ARUTILS_Http_Connection_t **connection);

/**
 * @brief Cancel an Http Connection command in progress (get, put, list etc)
 * @param connection The address of the pointer on the Http Connection
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Http_NewConnection ()
 */
eARUTILS_ERROR ARUTILS_Http_Connection_Cancel(ARUTILS_Http_Connection_t *connection);

/**
 * @brief Check if the connection has received a cancel to it's semaphore
 * @param connection The address of the pointer on the Http Connection
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see cURL
 */
eARUTILS_ERROR ARUTILS_Http_IsCanceled(ARUTILS_Http_Connection_t *connection);

/**
 * @brief Get an remote Http server file
 * @param connection The address of the pointer on the Http Connection
 * @param namePath The string of the file name path on the remote Http server
 * @param dstFile The string of the local file name path to be get
 * @param progressCallback The progress callback function
 * @param progressArg The progress callback function arg
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Http_NewConnection (), ARUTILS_Http_ProgressCallback_t
 */
eARUTILS_ERROR ARUTILS_Http_Get(ARUTILS_Http_Connection_t *connection, const char *namePath, const char *dstFile, ARUTILS_Http_ProgressCallback_t progressCallback, void* progressArg);

/**
 * @brief Get an remote Http server file
 * @warning This function allocates memory
 * @param connection The address of the pointer on the Http Connection
 * @param namePath The string of the file name path on the remote Http server
 * @param[out] data Returns byte buffer data address if data mode else give null pointer
 * @param[out] dataLen Returns byte buffer data length else give null pointer
 * @param progressCallback The progress callback function
 * @param progressArg The progress callback function arg
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Http_NewConnection (), ARUTILS_Http_ProgressCallback_t
 */
eARUTILS_ERROR ARUTILS_Http_Get_WithBuffer(ARUTILS_Http_Connection_t *connection, const char *namePath, uint8_t **data, uint32_t *dataLen, ARUTILS_Http_ProgressCallback_t progressCallback, void* progressArg);

#endif /* _ARUTILS_HTTP_H_ */
