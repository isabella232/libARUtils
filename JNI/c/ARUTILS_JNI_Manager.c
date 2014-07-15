/**
 * @file ARUTILS_JNI_Manager.c
 * @brief libARUtils JNI_Manager c file.
 * @date 30/12/2013
 * @author david.flattin.ext@parrot.com
 **/

#ifdef NDEBUG
/* Android ndk-build NDK_DEBUG=0*/
#else
/* Android ndk-build NDK_DEBUG=1*/
#ifndef DEBUG
#define DEBUG
#endif
#endif

#include <jni.h>
#include <inttypes.h>
#include <stdlib.h>

#include <libARSAL/ARSAL_Sem.h>
#include <libARSAL/ARSAL_Print.h>

#include "libARUtils/ARUTILS_Error.h"
#include "libARUtils/ARUTILS_Manager.h"
#include "libARUtils/ARUTILS_Http.h"
#include "libARUtils/ARUTILS_Ftp.h"
#include "ARUTILS_JNI_BLEFtp.h"

#include "ARUTILS_JNI.h"

#define ARUTILS_JNI_MANAGER_TAG       "JNI"

JavaVM* ARUTILS_JNI_Manager_VM = NULL;

jclass classException = NULL;
jmethodID methodId_Exception_Init = NULL;

jmethodID ftpListener_didFtpProgress_methodId = NULL;

typedef struct _ARUTILS_JNI_FtpCommandCallbacks_t_
{
    jobject jProgressListener;
    jobject jProgressArg;

} ARUTILS_JNI_FtpCommandCallbacks_t;

/*****************************************
 *
 *             JNI implementation :
 *
 ******************************************/

 /**
 * @brief save the reference to the java virtual machine
 * @note this function is automatically called on the JNI startup
 * @param[in] VM reference to the java virtual machine
 * @param[in] reserved data reserved
 * @return JNI version
 **/
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *VM, void *reserved)
{
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "Library has been loaded");

    /** Saving the reference to the java virtual machine */
    ARUTILS_JNI_Manager_VM = VM;

    ARSAL_PRINT(ARSAL_PRINT_WARNING, ARUTILS_JNI_MANAGER_TAG, "JNI_OnLoad ARUTILS_JNI_Manager_VM: %d ", ARUTILS_JNI_Manager_VM);

    /** Return the JNI version */
    return JNI_VERSION_1_6;
}

 /*
 * Class:     com_parrot_arsdk_arutils_ARUtilsManager
 * Method:    nativeStaticInit
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeStaticInit
  (JNIEnv *env, jclass class)
{

    return ARUTILS_JNI_InitFtpListenersJNI(env);
}

/*
 * Class:     com_parrot_arsdk_arutils_ARUtilsManager
 * Method:    nativeNew
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeNew
  (JNIEnv *env, jobject obj)
{
    /** -- Create a new manager -- */
    eARUTILS_ERROR error = ARUTILS_OK;
    /** local declarations */
    ARUTILS_Manager_t *manager = ARUTILS_Manager_New(&error);

    /** print error */
    if(error != ARUTILS_OK)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, " error: %d occurred \n", error);
    }

    return (long) manager;
}

/*
 * Class:     com_parrot_arsdk_arutils_ARUtilsManager
 * Method:    nativeDelete
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeDelete
  (JNIEnv *env, jobject obj, jlong jManager)
{
    /** -- Delete the Manager -- */

    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    ARUTILS_Manager_Delete(&manager);
}

/*
 * Class:     com_parrot_arsdk_arutils_ARUtilsManager
 * Method:    nativeInitWifiFtp
 * Signature: (JLjava/lang/String;ILjava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeInitWifiFtp
  (JNIEnv *env, jobject obj, jlong jManager, jstring jserver, jint port, jstring jusername, jstring jpassword)
{
    /** -- initialize UDP sockets of sending and receiving the data. -- */

    /** local declarations */
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    const char *nativeStrServer = (*env)->GetStringUTFChars(env, jserver, 0);
    const char *nativeStrUsername = (*env)->GetStringUTFChars(env, jusername, 0);
    const char *nativeStrPassword = (*env)->GetStringUTFChars(env, jpassword, 0);
    eARUTILS_ERROR error = ARUTILS_OK;

    error = ARUTILS_Manager_InitWifiFtp(manager, nativeStrServer, port, nativeStrUsername, nativeStrPassword);
    (*env)->ReleaseStringUTFChars( env, jserver, nativeStrServer );
    (*env)->ReleaseStringUTFChars( env, jusername, nativeStrUsername );
    (*env)->ReleaseStringUTFChars( env, jpassword, nativeStrPassword );

    return error;
}

/*
 * Class:     com_parrot_arsdk_arutils_ARUtilsManager
 * Method:    nativeCloseWifiFtp
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeCloseWifiFtp
  (JNIEnv *env, jobject obj, jlong jManager)
{
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, " nativeCloseWifiFtp");

    if(manager)
    {
        ARUTILS_Manager_CloseWifiFtp(manager);
    }

    return error;
}

/*
 * Class:     com_parrot_arsdk_arutils_ARUtilsManager
 * Method:    nativeInitBLEFtp
 */
JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeInitBLEFtp
  (JNIEnv *env, jobject obj, jlong jManager, jobject jBleFtp, jobject jCancelSem)
{
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;

    if (error == ARUTILS_OK)
    {
        manager->connectionObject = ARUTILS_BLEFtp_Connection_New(jBleFtp, jCancelSem, &error);
    }

    if (manager)
    {
        manager->ftpConnectionCancel = ARUTILS_BLEFtpAL_Connection_Cancel;
        manager->ftpConnectionIsCanceled = ARUTILS_BLEFtpAL_Connection_IsCanceled;
        manager->ftpConnectionReset = ARUTILS_BLEFtpAL_Connection_Reset;
        manager->ftpList = ARUTILS_BLEFtpAL_List;
        manager->ftpGetWithBuffer = ARUTILS_BLEFtpAL_Get_WithBuffer;
        manager->ftpGet = ARUTILS_BLEFtpAL_Get;
        manager->ftpPut = ARUTILS_BLEFtpAL_Put;
        manager->ftpDelete = ARUTILS_BLEFtpAL_Delete;
    }

    return error;
}

/*
 * Class:     com_parrot_arsdk_arutils_ARUtilsManager
 * Method:    nativeInitBLEFtp
 */
JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeCloseBLEFtp
  (JNIEnv *env, jobject obj, jlong jManager)
{
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;

    if (manager != NULL)
    {
        ARUTILS_BLEFtp_Connection_Delete((ARUTILS_BLEFtp_Connection_t **)&manager->connectionObject);

        ARSAL_Sem_Destroy(&manager->cancelSem);
    }
}

void ARUTILS_JNI_Ftp_ProgressCallback(void* arg, float percent)
{
    ARUTILS_JNI_FtpCommandCallbacks_t *callback = (ARUTILS_JNI_FtpCommandCallbacks_t*)arg;
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "");

    if (callback != NULL)
    {
        if ((ARUTILS_JNI_Manager_VM != NULL) && (callback->jProgressListener != NULL) && (ftpListener_didFtpProgress_methodId != NULL))
        {
            JNIEnv *env = NULL;
            jfloat jPercent = 0;
            jint jResultEnv = 0;
            int error = JNI_OK;

            jResultEnv = (*ARUTILS_JNI_Manager_VM)->GetEnv(ARUTILS_JNI_Manager_VM, (void **) &env, JNI_VERSION_1_6);

            if (jResultEnv == JNI_EDETACHED)
            {
                 (*ARUTILS_JNI_Manager_VM)->AttachCurrentThread(ARUTILS_JNI_Manager_VM, &env, NULL);
            }

            if (env == NULL)
            {
                error = JNI_FAILED;
            }

            if ((error == JNI_OK) && (ftpListener_didFtpProgress_methodId != NULL))
            {
                jPercent = percent;

                (*env)->CallVoidMethod(env, callback->jProgressListener, ftpListener_didFtpProgress_methodId, callback->jProgressArg, jPercent);
            }

            if ((jResultEnv == JNI_EDETACHED) && (env != NULL))
            {
                 (*ARUTILS_JNI_Manager_VM)->DetachCurrentThread(ARUTILS_JNI_Manager_VM);
            }
        }
    }

}

JNIEXPORT jstring JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeBLEFtpList
  (JNIEnv *env, jobject obj, jlong jManager, jstring jRemotePath)
{
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;
    jstring result = NULL;

    if (manager == NULL || jRemotePath == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "Wrong parameter: %d %d", manager, jRemotePath);
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (error == ARUTILS_OK)
    {
        const char *namePath = (*env)->GetStringUTFChars(env, jRemotePath, 0);

        char **resultList = malloc(sizeof(char*));

        uint32_t resultListLen;

        error = ARUTILS_BLEFtpAL_List(manager, namePath, resultList, &resultListLen);

        if (error == ARUTILS_OK)
        {
            result = (*env)->NewStringUTF(env, *resultList);
        }
        else
        {
            ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "ARUTILS_BLEFtpAL_List failed: %d", error);
        }

        (*env)->ReleaseStringUTFChars( env, jRemotePath, namePath);
    }
    return result;
}


JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeBLEFtpDelete
  (JNIEnv *env, jobject obj, jlong jManager, jstring jRemotePath)
{
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;

    if (manager == NULL || jRemotePath == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "Wrong parameter: %d %d", manager, jRemotePath);
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (error == ARUTILS_OK)
    {
        const char *namePath = (*env)->GetStringUTFChars(env, jRemotePath, 0);

        error = ARUTILS_BLEFtpAL_Delete(manager, namePath);

        (*env)->ReleaseStringUTFChars( env, jRemotePath, namePath);
    }
    return error;
}


JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeBLEFtpPut
  (JNIEnv *env, jobject obj, jlong jManager, jstring jRemotePath, jstring jSrcFile, jobject jProgressListener, jobject jProgressArg, jboolean resume)
{
    ARUTILS_JNI_FtpCommandCallbacks_t *callback = NULL;
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;

    if (manager == NULL || jRemotePath == NULL || jSrcFile == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "Wrong parameter: %d %d %d", manager, jRemotePath, jSrcFile);
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (error == ARUTILS_OK)
    {
        callback = calloc(1, sizeof(ARUTILS_JNI_FtpCommandCallbacks_t));
        if (callback == NULL)
        {
            error = ARUTILS_ERROR;
        }
    }

    if (error == ARUTILS_OK)
    {
        if (jProgressListener != NULL)
        {
            callback->jProgressListener = (*env)->NewGlobalRef(env, jProgressListener);
        }
        if (jProgressArg != NULL)
        {
            callback->jProgressArg = (*env)->NewGlobalRef(env, jProgressArg);
        }

        const char *namePath = (*env)->GetStringUTFChars(env, jRemotePath, 0);
        const char *srcFile = (*env)->GetStringUTFChars(env, jSrcFile, 0);

        error = ARUTILS_BLEFtpAL_Put(manager, namePath, srcFile, ARUTILS_JNI_Ftp_ProgressCallback, callback, resume);

        (*env)->ReleaseStringUTFChars( env, jRemotePath, namePath);
        (*env)->ReleaseStringUTFChars( env, jSrcFile, srcFile);
    }

    if (callback != NULL)
    {
        if (callback->jProgressListener != NULL)
        {
            (*env)->DeleteGlobalRef(env, callback->jProgressListener);
        }
        if (callback->jProgressArg != NULL)
        {
            (*env)->DeleteGlobalRef(env, callback->jProgressArg);
        }
        free(callback);
        callback = NULL;
    }

    return error;
}


JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeBLEFtpGet
  (JNIEnv *env, jobject obj, jlong jManager, jstring jRemotePath, jstring jDestFile, jobject jProgressListener, jobject jProgressArg, jboolean resume)
{
    ARUTILS_JNI_FtpCommandCallbacks_t *callback = NULL;
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;

    if (manager == NULL || jRemotePath == NULL || jDestFile == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "Wrong parameter: %d %d %d", manager, jRemotePath, jDestFile);
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (error == ARUTILS_OK)
    {
        callback = calloc(1, sizeof(ARUTILS_JNI_FtpCommandCallbacks_t));
        if (callback == NULL)
        {
            error = ARUTILS_ERROR;
        }
    }

    if (error == ARUTILS_OK)
    {
        if (jProgressListener != NULL)
        {
            callback->jProgressListener = (*env)->NewGlobalRef(env, jProgressListener);
        }
        if (jProgressArg != NULL)
        {
            callback->jProgressArg = (*env)->NewGlobalRef(env, jProgressArg);
        }

        const char *namePath = (*env)->GetStringUTFChars(env, jRemotePath, 0);
        const char *destFile = (*env)->GetStringUTFChars(env, jDestFile, 0);

        error = ARUTILS_BLEFtpAL_Get(manager, namePath, destFile, ARUTILS_JNI_Ftp_ProgressCallback, callback, resume);

        (*env)->ReleaseStringUTFChars( env, jRemotePath, namePath);
        (*env)->ReleaseStringUTFChars( env, jDestFile, destFile);
    }

    if (callback != NULL)
    {
        if (callback->jProgressListener != NULL)
        {
            (*env)->DeleteGlobalRef(env, callback->jProgressListener);
        }
        if (callback->jProgressArg != NULL)
        {
            (*env)->DeleteGlobalRef(env, callback->jProgressArg);
        }
        free(callback);
        callback = NULL;
    }

    return error;
}


JNIEXPORT jbyteArray JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeBLEFtpGetWithBuffer
  (JNIEnv *env, jobject obj, jlong jManager, jstring jRemotePath, jobject jProgressListener, jobject jProgressArg)
{
    ARUTILS_JNI_FtpCommandCallbacks_t *callback = NULL;
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;
    jbyteArray result = NULL;

    if (manager == NULL || jRemotePath == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "Wrong parameter: %d %d", manager, jRemotePath);
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (error == ARUTILS_OK)
    {
        callback = calloc(1, sizeof(ARUTILS_JNI_FtpCommandCallbacks_t));
        if (callback == NULL)
        {
            error = ARUTILS_ERROR;
        }
    }

    if (error == ARUTILS_OK)
    {
        if (jProgressListener != NULL)
        {
            callback->jProgressListener = (*env)->NewGlobalRef(env, jProgressListener);
        }
        if (jProgressArg != NULL)
        {
            callback->jProgressArg = (*env)->NewGlobalRef(env, jProgressArg);
        }

        const char *namePath = (*env)->GetStringUTFChars(env, jRemotePath, 0);

        int dataLen;
        uint8_t *data;

        error = ARUTILS_BLEFtpAL_Get_WithBuffer(manager, namePath, &data, &dataLen, ARUTILS_JNI_Ftp_ProgressCallback, callback);

        (*env)->ReleaseStringUTFChars( env, jRemotePath, namePath);

        if (error == ARUTILS_OK)
        {
            result = (*env)->NewByteArray(env, dataLen);
            (*env)->SetByteArrayRegion(env, result, 0, dataLen, data);
        }
    }

    if (callback != NULL)
    {
        if (callback->jProgressListener != NULL)
        {
            (*env)->DeleteGlobalRef(env, callback->jProgressListener);
        }
        if (callback->jProgressArg != NULL)
        {
            (*env)->DeleteGlobalRef(env, callback->jProgressArg);
        }
        free(callback);
        callback = NULL;
    }

    return result;
}

JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeBLEFtpConnectionCancel
  (JNIEnv *env, jobject obj, jlong jManager)
{
    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "%x", jManager);
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;

    if (manager == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "Wrong parameter: %x", manager);
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (error == ARUTILS_OK)
    {
        error = ARUTILS_BLEFtpAL_Connection_Cancel(manager);
    }
    return error;
}

JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeBLEFtpIsConnectionCanceled
  (JNIEnv *env, jobject obj, jlong jManager)
{
    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "%x", jManager);
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;

    if (manager == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "Wrong parameter: %x", manager);
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (error == ARUTILS_OK)
    {
        error = ARUTILS_BLEFtpAL_Connection_IsCanceled(manager);
    }
    return error;
}

JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeBLEFtpConnectionReset
  (JNIEnv *env, jobject obj, jlong jManager)
{
    ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "%x", jManager);
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;

    if (manager == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "Wrong parameter: %x", manager);
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (error == ARUTILS_OK)
    {
        error = ARUTILS_BLEFtpAL_Connection_Reset(manager);
    }
    return error;
}

JNIEXPORT jint JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsManager_nativeBLEFtpRename
  (JNIEnv *env, jobject obj, jlong jManager, jstring jOldNamePath, jstring jNewNamePath)
{
    ARUTILS_Manager_t *manager = (ARUTILS_Manager_t*) (intptr_t) jManager;
    eARUTILS_ERROR error = ARUTILS_OK;

    if (manager == NULL || jOldNamePath == NULL || jNewNamePath == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_ERROR, ARUTILS_JNI_MANAGER_TAG, "Wrong parameter: %x %x %x", manager, jOldNamePath, jNewNamePath);
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (error == ARUTILS_OK)
    {
        const char *oldNamePath = (*env)->GetStringUTFChars(env, jOldNamePath, 0);
        const char *newNamePath = (*env)->GetStringUTFChars(env, jNewNamePath, 0);

        error = ARUTILS_BLEFtpAL_Rename(manager, oldNamePath, newNamePath);

        (*env)->ReleaseStringUTFChars( env, jOldNamePath, oldNamePath);
        (*env)->ReleaseStringUTFChars( env, jNewNamePath, newNamePath);
    }
    return error;
}

/*****************************************
 *
 *             Private implementation:
 *
 *****************************************/

int ARUTILS_JNI_NewARUtilsExceptionJNI(JNIEnv *env)
{
    jclass locClassException = NULL;
    int error = JNI_OK;

    if (classException == NULL)
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "");

        if (env == NULL)
        {
           error = JNI_FAILED;
        }

        if (error == JNI_OK)
        {
            locClassException = (*env)->FindClass(env, "com/parrot/arsdk/arutils/ARUtilsException");

            if (locClassException == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "ARUtilsException class not found");
                error = JNI_FAILED;
            }
            else
            {
                classException = (*env)->NewGlobalRef(env, locClassException);

                if (classException == NULL)
                {
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "NewGlobalRef class failed");
                    error = JNI_FAILED;
                }
            }
        }

        if (error == JNI_OK)
        {
            methodId_Exception_Init = (*env)->GetMethodID(env, classException, "<init>", "(I)V");

            if (methodId_Exception_Init == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "init method not found");
                error = JNI_FAILED;
            }
        }
    }

    return error;
}

void ARUTILS_JNI_FreeARUtilsExceptionJNI(JNIEnv *env)
{
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "");

    if (env != NULL)
    {
        if (classException != NULL)
        {
            (*env)->DeleteGlobalRef(env, classException);
            classException = NULL;
        }

        methodId_Exception_Init = NULL;
    }
}

jobject ARUTILS_JNI_NewARUtilsException(JNIEnv *env, eARUTILS_ERROR nativeError)
{
    jobject jException = NULL;
    jint jError = JNI_OK;
    int error = JNI_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "%d", nativeError);

    if (env == NULL)
    {
       error = JNI_FAILED;
    }

    if (error == JNI_OK)
    {
        error = ARUTILS_JNI_NewARUtilsExceptionJNI(env);
    }

    if (error == JNI_OK)
    {
        jError = nativeError;

        jException = (*env)->NewObject(env, classException, methodId_Exception_Init, jError);
    }

    return jException;
}

void ARUTILS_JNI_ThrowARUtilsException(JNIEnv *env, eARUTILS_ERROR nativeError)
{
    jthrowable jThrowable = NULL;
    int error = JNI_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "%d", error);

    if (env == NULL)
    {
       error = JNI_FAILED;
    }

    if (error == JNI_OK)
    {
        jThrowable = ARUTILS_JNI_NewARUtilsException(env, nativeError);

        if (jThrowable == NULL)
        {
           error = JNI_FAILED;
        }
    }

    if (error == JNI_OK)
    {
        (*env)->Throw(env, jThrowable);
    }
}


int ARUTILS_JNI_InitFtpListenersJNI(JNIEnv *env)
{
    jclass classFtpProgressListener = NULL;
    int error = JNI_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "");

    if (env == NULL)
    {
        error = JNI_FAILED;
    }

    if (ftpListener_didFtpProgress_methodId == NULL)
    {
        if (error == JNI_OK)
        {
            classFtpProgressListener = (*env)->FindClass(env, "com/parrot/arsdk/arutils/ARUtilsFtpProgressListener");

            if (classFtpProgressListener == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "ARUtilsFtpProgressListener class not found");
                error = JNI_FAILED;
            }
        }

        if (error == JNI_OK)
        {
            ftpListener_didFtpProgress_methodId = (*env)->GetMethodID(env, classFtpProgressListener, "didFtpProgress", "(Ljava/lang/Object;F)V");

            if (ftpListener_didFtpProgress_methodId == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_MANAGER_TAG, "Listener didFtpProgress method not found");
            }
        }
    }

    return error;
}
