/**
 * @file ARUTILS_Http.h
 * @brief libARUtils Http header file.
 * @date 26/12/2013
 * @author david.flattin.ext@parrot.com
 **/

#ifndef _ARUTILS_HTTP_PRIVATE_H_
#define _ARUTILS_HTTP_PRIVATE_H_

/**
 * @brief Http max user name string size
 */
#define ARUTILS_HTTP_MAX_USER_SIZE     64

/**
 * @brief Http CallbackData structure
 * @param isUploading Is set to 1 if upploading else 0 of downloading
 * @param data The byte buffer data if data mode else null
 * @param dataIndex The byte buffer data index if data mode else 0
 * @param dataSize The byte buffer data size if data mode else 0
 * @param file The file data if file mode else null
 * @param error The last error
 * @param progressCallback The progress callback
 * @param progressArg The progress arg
 * @see ARUTILS_Http_ReadDataCallback (), ARUTILS_Http_WriteDataCallback ()
 */
typedef struct _ARUTILS_Http_CallbackData_t_
{
    int isUploading;
    uint8_t *data;
    uint32_t dataIndex;
    uint32_t dataSize;
    FILE *file;
    eARUTILS_ERROR error;
    ARUTILS_Http_ProgressCallback_t progressCallback;
    void *progressArg;
    
} ARUTILS_Http_CallbackData_t;

/**
 * @brief Http Connection structure
 * @param cancelSem The semaphore to cancel Http command
 * @param curl The cURL connection
 * @param serverUrl The Http url connection string
 * @param username The Http connection user name
 * @param passwordThe Http connection user password
 * @param cbdata The Http connection data for callbacks
 * @see ARUTILS_Http_ReadDataCallback (), ARUTILS_Http_WriteDataCallback ()
 */
struct ARUTILS_Http_Connection_t
{
    ARSAL_Sem_t *cancelSem;
    CURL *curl;
    char serverUrl[ARUTILS_HTTP_MAX_URL_SIZE];
    char username[ARUTILS_HTTP_MAX_USER_SIZE];
    char password[ARUTILS_HTTP_MAX_USER_SIZE];
    ARUTILS_Http_CallbackData_t cbdata;
};

/**
 * @brief ReadData callback of cURL connection
 * @param ptr The pointer of read data
 * @param size The size of the read data type (byte)
 * @param nmemb The number of read data present
 * @param userData The pointer of the user custom argument
 * @retval On success, returns nmemb. Otherwise, it returns an error code.
 * @see cURL
 */
size_t ARUTILS_Http_ReadDataCallback(void *ptr, size_t size, size_t nmemb, void *userData);

/**
 * @brief WriteData callback of cURL connection
 * @param ptr The pointer of write data
 * @param size The size of the write data type (byte)
 * @param nmemb The number of write data present
 * @param userData The pointer of the user custom argument
 * @retval On success, returns nmemb. Otherwise, it returns an error code.
 * @see cURL
 */
size_t ARUTILS_Http_WriteDataCallback(void *ptr, size_t size, size_t nmemb, void *userData);

/**
 * @brief Progress callback of cURL connection
 * @param userData The pointer of the user custom argument
 * @param dltotal The total size to be downloaded
 * @param dlnow The current size already donloaded
 * @param ultotal The total size to be uploaded
 * @param ulnow The current size already uploaded
 * @retval On success, returns 0. Otherwise, it returns an error code.
 * @see cURL
 */
int ARUTILS_Http_ProgressCallback(void *userData, double dltotal, double dlnow, double ultotal, double ulnow);

/**
 * @brief Reset the Http connection values
 * @param connection The address of the pointer on the Http Connection
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see cURL
 */
eARUTILS_ERROR ARUTILS_Http_ResetOptions(ARUTILS_Http_Connection_t *connection);

/**
 * @brief Free CallbackData structure
 * @warning This function frees memory
 * @param connection The address of the pointer on the Http Connection
 * @param namePath The string of the file name path
 * @see cURL
 */
void ARUTILS_Http_FreeCallbackData(ARUTILS_Http_CallbackData_t *cbdata);

/**
 * @brief Check if the connection has received a cancel to it's semaphore
 * @param connection The address of the pointer on the Http Connection
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see cURL
 */
eARUTILS_ERROR ARUTILS_Http_IsCanceled(ARUTILS_Http_Connection_t *connection);

/**
 * @brief Translate cURL error code to an eARUTILS_ERROR enum error
 * @param connection The address of the pointer on the Http Connection
 * @param code The cURL error code
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see cURL
 */
eARUTILS_ERROR ARUTILS_Http_GetErrorFromCode(ARUTILS_Http_Connection_t *connection, CURLcode code);

/**
 * @brief Get an remote Http server file
 * @param connection The address of the pointer on the Http Connection
 * @param namePath The string of the file name path on the remote Http server
 * @param dstFile The string of the local file name path to be get
 * @param[out] data Returns byte buffer data address if data mode else give null pointer
 * @param[out] dataLen Returns byte buffer data length else give null pointer 
 * @param progressCallback The progress callback function
 * @param progressArg The progress callback function arg
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see cURL
 */
 eARUTILS_ERROR ARUTILS_Http_Get_Internal(ARUTILS_Http_Connection_t *connection, const char *namePath, const char *dstFile, uint8_t **data, uint32_t *dataLen, ARUTILS_Http_ProgressCallback_t progressCallback, void* progressArg);

#endif /* _ARUTILS_HTTP_PRIVATE_H_ */

