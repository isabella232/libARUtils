/**
 * @file ARUTILS_FileSystem.h
 * @brief libARUtils FileSystem header file.
 * @date 19/12/2013
 * @author david.flattin.ext@parrot.com
 **/
 
#ifndef _ARUTILS_FILESYSTEM_H_
#define _ARUTILS_FILESYSTEM_H_

#include "libARUtils/ARUTILS_Error.h" 

/**
 * @brief The maximum directory depth for ftw/ARSAL_Ftw calls
 * @see ARSAL_Ftw ()
 */
#define ARUTILS_FILE_SYSTEM_MAX_FD_FOR_FTW    20

/**
 * @brief Get local file system file size in bytes
 * @param namePath The string name path
 * @param[out] size The size of the file if success
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see 
 */
eARUTILS_ERROR ARUTILS_FileSystem_GetFileSize(const char *namePath, uint32_t *size);

/**
 * @brief Rename local file system file
 * @param oldName The old file name path string
 * @param newName The new file name path string
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see stat ()
 */
eARUTILS_ERROR ARUTILS_FileSystem_Rename(const char *oldName, const char *newName);

/**
 * @brief Remove local file system file
 * @param localPath The file name path string
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see remove
 */
eARUTILS_ERROR ARUTILS_FileSystem_RemoveFile(const char *localPath);

/**
 * @brief Remove recurcively a directory and its content
 * @param localPath The file name path string
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARSAL_Ftw (), rmdir
 */
eARUTILS_ERROR ARUTILS_FileSystem_RemoveDir(const char *localPath);

/**
 * @brief Get free space in bytes of the local file system directory
 * @param localPath The file name path string
 * @param[out] freeSpace The free space of the local directory
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see
 */
eARUTILS_ERROR ARUTILS_FileSystem_GetFreeSpace(const char *localPath, double *freeSpace);

#endif /* _ARUTILS_FILESYSTEM_H_ */
