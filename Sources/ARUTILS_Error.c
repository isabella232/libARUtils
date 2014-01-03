/*
 * GENERATED FILE
 *  Do not modify this file, it will be erased during the next configure run
 */

/**
 * @file ARUTILS_Error.c
 * @brief ToString function for eARUTILS_ERROR enum
 */

#include <libARUtils/ARUTILS_Error.h>

char* ARUTILS_Error_ToString (eARUTILS_ERROR error)
{
    switch (error)
    {
    case ARUTILS_OK:
        return "No error";
        break;
    case ARUTILS_ERROR:
        return "Unknown generic error";
        break;
    case ARUTILS_ERROR_ALLOC:
        return "Memory allocation error";
        break;
    case ARUTILS_ERROR_BAD_PARAMETER:
        return "Bad parameters error";
        break;
    case ARUTILS_ERROR_SYSTEM:
        return "System error";
        break;
    case ARUTILS_ERROR_CURL_ALLOC:
        return "curl allocation error";
        break;
    case ARUTILS_ERROR_CURL_SETOPT:
        return "curl set option error";
        break;
    case ARUTILS_ERROR_CURL_GETINFO:
        return "curl get info error";
        break;
    case ARUTILS_ERROR_CURL_PERFORM:
        return "curl perform error";
        break;
    case ARUTILS_ERROR_FILE_NOT_FOUND:
        return "file not found error";
        break;
    case ARUTILS_ERROR_FTP_CONNECT:
        return "ftp connect error";
        break;
    case ARUTILS_ERROR_FTP_CODE:
        return "ftp code error";
        break;
    case ARUTILS_ERROR_FTP_SIZE:
        return "ftp file size error";
        break;
    case ARUTILS_ERROR_FTP_RESUME:
        return "ftp resume error";
        break;
    case ARUTILS_ERROR_FTP_CANCELED:
        return "ftp user canceled error";
        break;
    case ARUTILS_ERROR_HTTP_CONNECT:
        return "http connect error";
        break;
    case ARUTILS_ERROR_HTTP_CODE:
        return "http code error";
        break;
    case ARUTILS_ERROR_HTTP_SIZE:
        return "http file size error";
        break;
    case ARUTILS_ERROR_HTTP_RESUME:
        return "http resume error";
        break;
    case ARUTILS_ERROR_HTTP_CANCELED:
        return "http user canceled error";
        break;
    default:
        return "Unknown value";
        break;
    }
    return "Unknown value";
}
