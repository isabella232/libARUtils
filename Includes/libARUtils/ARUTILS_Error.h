
/**
 * @file libARUtils/ARUTILS_Error.h
 * @brief libARUtils error header, This file contains all error of ARUTILS
 * @date 19/12/2013
 * @author david.flattin.ext@parrot.com
 **/

#ifndef _LIBARUTILS_ERROR_H_
#define _LIBARUTILS_ERROR_H_

/**
 * @brief libARUtils errors known.
 */
typedef enum
{
    ARUTILS_OK = 0, /**< No error */
    ARUTILS_ERROR = -1000, /**< Unknown generic error */
    ARUTILS_ERROR_ALLOC, /**< Memory allocation error */
    ARUTILS_ERROR_BAD_PARAMETER, /**< Bad parameters error */
    ARUTILS_ERROR_SYSTEM, /**< System error */

    ARUTILS_ERROR_CURL_ALLOC = -2000, /**< curl allocation error */
    ARUTILS_ERROR_CURL_SETOPT, /**< curl set option error */
    ARUTILS_ERROR_CURL_GETINFO, /**< curl get info error */
    ARUTILS_ERROR_CURL_PERFORM, /**< curl perform error */

    ARUTILS_ERROR_FILE_NOT_FOUND = -3000, /**< file not found error */

    ARUTILS_ERROR_FTP_CONNECT = -4000, /**< ftp connect error */
    ARUTILS_ERROR_FTP_CODE, /**< ftp code error */
    ARUTILS_ERROR_FTP_SIZE, /**< ftp file size error */
    ARUTILS_ERROR_FTP_RESUME, /**< ftp resume error */
    ARUTILS_ERROR_FTP_CANCELED, /**< ftp user canceled error */
    ARUTILS_ERROR_FTP_FILE, /**< ftp file error */
    ARUTILS_ERROR_FTP_MD5, /**< ftp md5 error */

    ARUTILS_ERROR_HTTP_CONNECT = -5000, /**< http connect error */
    ARUTILS_ERROR_HTTP_CODE, /**< http code error */
    ARUTILS_ERROR_HTTP_AUTHORIZATION_REQUIRED, /**< http authorization required */
    ARUTILS_ERROR_HTTP_ACCESS_DENIED, /**< http access denied */
    ARUTILS_ERROR_HTTP_SIZE, /**< http file size error */
    ARUTILS_ERROR_HTTP_RESUME, /**< http resume error */
    ARUTILS_ERROR_HTTP_CANCELED, /**< http user canceled error */

    ARUTILS_ERROR_BLE_FAILED = -6000, /**< BLE ftp failed error */

    ARUTILS_ERROR_NETWORK_TYPE = -7000, /**< Network type, not available for the platform error */

} eARUTILS_ERROR;

/**
 * @brief Gets the error string associated with an eARUTILS_ERROR
 * @param error The error to describe
 * @return A static string describing the error
 *
 * @note User should NEVER try to modify a returned string
 */
char* ARUTILS_Error_ToString (eARUTILS_ERROR error);

#endif /* _LIBARUTILS_ERROR_H_ */
