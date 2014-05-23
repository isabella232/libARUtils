/**
 * @file ARUTILS_Manager.h
 * @brief libARUtils Manager header file.
 * @date 21/05/2014
 * @author david.flattin.ext@parrot.com
 **/

#ifndef _ARUTILS_MANAGER_PRIVATE_H_
#define _ARUTILS_MANAGER_PRIVATE_H_

/**
 * @brief Cancel an Ftp Connection command in progress (get, put, list etc)
 * @param connection The address of the pointer on the Ftp Connection
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Manager_New ()
 */
typedef eARUTILS_ERROR (*ARUTILS_Manager_Ftp_Connection_Cancel_t) (ARUTILS_Manager_t *manager);

/**
 * @brief Execute Ftp List command to retrieve directory content
 * @warning This function allocates memory
 * @param manager The address of the pointer on the Ftp Connection
 * @param namePath The string of the directory path on the remote Ftp server
 * @param resultList The pointer of the string of the directory content null terminated
 * @param resultListLen The pointer of the lenght of the resultList string including null terminated
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Manager_New ()
 */
typedef eARUTILS_ERROR (*ARUTILS_Manager_Ftp_List_t) (ARUTILS_Manager_t *manager, const char *namePath, char **resultList, uint32_t *resultListLen);

/**
 * @brief Get an remote Ftp server file
 * @warning This function allocates memory
 * @param manager The address of the pointer on the Ftp Connection
 * @param namePath The string of the file name path on the remote Ftp server
 * @param data The pointer of the data buffer of the file data
 * @param dataLen The pointer of the length of the data buffer
 * @param progressCallback The progress callback function
 * @param progressArg The progress callback function arg
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Manager_New (), ARUTILS_Ftp_ProgressCallback_t, eARUTILS_FTP_RESUME
 */
typedef eARUTILS_ERROR (*ARUTILS_Manager_Get_WithBuffer_t) (ARUTILS_Manager_t *manager, const char *namePath, uint8_t **data, uint32_t *dataLen,  ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg);

/**
 * @brief Get an remote Ftp server file
 * @param manager The address of the pointer on the Ftp Connection
 * @param namePath The string of the file name path on the remote Ftp server
 * @param dstFile The string of the local file name path to be get
 * @param progressCallback The progress callback function
 * @param progressArg The progress callback function arg
 * @param resume The resume capability requested
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Manager_New (), ARUTILS_Ftp_ProgressCallback_t, eARUTILS_FTP_RESUME
 */
typedef eARUTILS_ERROR (*ARUTILS_Manager_Ftp_Get_t) (ARUTILS_Manager_t *manager, const char *namePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume);

/**
 * @brief Put an remote Ftp server file
 * @param manager The address of the pointer on the Ftp Connection
 * @param namePath The string of the file name path on the remote Ftp server
 * @param srcFile The string of the local file name path to be put
 * @param progressCallback The progress callback function
 * @param progressArg The progress callback function arg
 * @param resume The resume capability requested
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Manager_New (), ARUTILS_Ftp_ProgressCallback_t, eARUTILS_FTP_RESUME
 */
typedef eARUTILS_ERROR (*ARUTILS_Manager_Ftp_Put_t) (ARUTILS_Manager_t *manager, const char *namePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume);

/**
 * @brief Delete an remote Ftp server file
 * @param delete The address of the pointer on the Ftp Connection
 * @param namePath The string of the file name path on the remote Ftp server
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Manager_New ()
 */
typedef eARUTILS_ERROR (*ARUTILS_Manager_Ftp_Delete_t) (ARUTILS_Manager_t *manager, const char *namePath);

/**
 * @brief Ftp Manager structure
 * @param ftpConnectionCancel The ARUTILS_Manager_Ftp_Connection_Cancel
 * @param ftpList The ARUTILS_Manager_Ftp_List
 * @param ftpGetWithBuffer The ARUTILS_Manager_Get_WithBuffer
 * @param ftpGet The ARUTILS_Manager_Ftp_Get
 * @param ftpPut The ARUTILS_Manager_Ftp_Put
 * @param ftpDelete The ARUTILS_Manager_Ftp_Delete
 * @see ARUTILS_Manager_New ()
 */
struct ARUTILS_Manager_t
{
    ARUTILS_Manager_Ftp_Connection_Cancel_t ftpConnectionCancel;
    ARUTILS_Manager_Ftp_List_t ftpList;
    ARUTILS_Manager_Get_WithBuffer_t ftpGetWithBuffer;
    ARUTILS_Manager_Ftp_Get_t ftpGet;
    ARUTILS_Manager_Ftp_Put_t ftpPut;
    ARUTILS_Manager_Ftp_Delete_t ftpDelete;
    
    ARSAL_Sem_t cancelSem;
    void *connectionObject;
};

#endif /* _ARUTILS_MANAGER_PRIVATE_H_ */

