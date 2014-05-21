/**
 * @file ARUTILS_JNI.h
 * @brief libARUtils JNI header file.
 * @date 30/12/2013
 * @author david.flattin.ext@parrot.com
 **/

#ifndef _ARUTILS_JNI_H_
#define _ARUTILS_JNI_H_

#ifndef JNI_OK
#define JNI_OK      0
#endif
#ifndef JNI_FAILED
#define JNI_FAILED  -1
#endif

/**
 * @brief JNI JavaVM struct
 */
extern JavaVM* ARUTILS_JNI_Manager_VM;

/**
 * @brief JNI FtpConnection structure
 * @param cancelSem The Ftp cancel semaphore
 * @param ftpConnection The Ftp connection
 * @see Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeNewFtpConnection
 */
typedef struct _ARUTILS_JNI_FtpConnection_t_
{
    ARSAL_Sem_t cancelSem;
    ARUTILS_WifiFtp_Connection_t *ftpConnection;

} ARUTILS_JNI_FtpConnection_t;

/**
 * @brief FtpConnection Callbacks structure
 * @param jProgressListener The progress Listener
 * @param jProgressArg The progress Arg object
 * @see Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeGet
 */
typedef struct _ARUTILS_JNI_FtpConnectionCallbacks_t_
{
    jobject jProgressListener;
    jobject jProgressArg;

} ARUTILS_JNI_FtpConnectionCallbacks_t;

/**
 * @brief JNI HttpConnection structure
 * @param cancelSem The Http cancel semaphore
 * @param httpConnection The Http connection
 * @see Java_com_parrot_arsdk_arutils_ARUtilsHttpConnection_nativeNewHttpConnection
 */
typedef struct _ARUTILS_JNI_HttpConnection_t_
{
    ARSAL_Sem_t cancelSem;
    ARUTILS_Http_Connection_t *httpConnection;

} ARUTILS_JNI_HttpConnection_t;

/**
 * @brief HttpConnection Callbacks structure
 * @param jProgressListener The progress Listener
 * @param jProgressArg The progress Arg object
 * @see Java_com_parrot_arsdk_arutils_ARUtilsHttpConnection_nativeGet
 */
typedef struct _ARUTILS_JNI_HttpConnectionCallbacks_t_
{
    jobject jProgressListener;
    jobject jProgressArg;

} ARUTILS_JNI_HttpConnectionCallbacks_t;

/**
 * @brief Throw a new ARUtilsException
 * @param env The java env
 * @param nativeError The error
 * @retval void
 * @see ARUTILS_JNI_NewARUtilsException
 */
void ARUTILS_JNI_ThrowARUtilsException(JNIEnv *env, eARUTILS_ERROR nativeError);

/**
 * @brief Create a new ARUtilsException
 * @param env The java env
 * @param nativeError The error
 * @retval the new ARUtilsException
 * @see ARUTILS_JNI_ThrowARUtilsException
 */
jobject ARUTILS_JNI_NewARUtilsException(JNIEnv *env, eARUTILS_ERROR nativeError);

/**
 * @brief Get the ARUtilsException JNI class
 * @param env The java env
 * @retval JNI_TRUE if Success, else JNI_FALSE
 * @see ARUTILS_JNI_FreeARUtilsExceptionJNI
 */
int ARUTILS_JNI_NewARUtilsExceptionJNI(JNIEnv *env);

/**
 * @brief Free the ARUtilsException JNI class
 * @param env The java env
 * @retval void
 * @see ARDATATRANSFER_JNI_Manager_NewARDataTransferExceptionJNI
 */
void ARUTILS_JNI_FreeARUtilsExceptionJNI(JNIEnv *env);

/**
 * @brief Get the ARUtilsFtpProgressListener JNI class
 * @param env The java env
 * @retval JNI_TRUE if Success, else JNI_FALSE
 * @see ARUTILS_JNI_FreeListenersJNI
 */
int ARUTILS_JNI_NewFtpListenersJNI(JNIEnv *env);

/**
 * @brief Free the ARUtilsFtpProgressListener JNI class
 * @param env The java env
 * @retval void
 * @see ARUTILS_JNI_NewListenersJNI
 */
void ARUTILS_JNI_FreeFtpListenersJNI(JNIEnv *env);

/**
 * @brief Get the ARUtilsHttpProgressListener JNI class
 * @param env The java env
 * @retval JNI_TRUE if Success, else JNI_FALSE
 * @see ARUTILS_JNI_FreeListenersJNI
 */
int ARUTILS_JNI_NewHttpListenersJNI(JNIEnv *env);

/**
 * @brief Free the ARUtilsHttpProgressListener JNI class
 * @param env The java env
 * @retval void
 * @see ARUTILS_JNI_NewListenersJNI
 */
void ARUTILS_JNI_FreeHttpListenersJNI(JNIEnv *env);

/**
 * @brief Callback that give the file download progress percent
 * @param arg The arg
 * @param percent The progress percent
 * @retval void
 * @see ARDATATRANSFER_JNI_MediasDownloader_FreeListenersJNI
 */
void ARUTILS_JNI_FtpConnection_ProgressCallback(void* arg, uint8_t percent);

/**
 * @brief Callback that give the file download progress percent
 * @param arg The arg
 * @param percent The progress percent
 * @retval void
 * @see ARDATATRANSFER_JNI_MediasDownloader_FreeListenersJNI
 */
void ARUTILS_JNI_HttpConnection_ProgressCallback(void* arg, uint8_t percent);

#endif /* _ARUTILS_JNI_H_ */
