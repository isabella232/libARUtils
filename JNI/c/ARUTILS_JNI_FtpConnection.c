/**
 * @file ARUTILS_JNI_FtpConnection.c
 * @brief libARUtils JNI_FtpConnection c file.
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
#include "libARUtils/ARUTILS_Http.h"
#include "libARUtils/ARUTILS_Ftp.h"

#include "ARUTILS_JNI.h"

#define ARUTILS_JNI_FTPCONNECTION_TAG       "JNI"


jmethodID methodId_FtpListener_didFtpProgress = NULL;

/*****************************************
 *
 *             Public implementation:
 *
 *****************************************/


JNIEXPORT jboolean JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeStaticInit(JNIEnv *env, jclass jClass)
{
    jboolean jret = JNI_FALSE;
    int error = JNI_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (env == NULL)
    {
        error = JNI_FAILED;
    }

    if (error == JNI_OK)
    {
        error = ARUTILS_JNI_NewFtpListenersJNI(env);
    }

    if (error == JNI_OK)
    {
        jret = JNI_TRUE;
    }

    return jret;
}

JNIEXPORT jlong JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeNewFtpConnection(JNIEnv *env, jobject jThis, jstring jServer, jint jPort, jstring jUsername, jstring jPassword)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = NULL;
    const char *nativeServer = (*env)->GetStringUTFChars(env, jServer, 0);
    const char *nativeUsername = NULL;
    const char *nativePassword = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;
    int resultSys = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (jUsername != NULL)
    {
        nativeUsername = (*env)->GetStringUTFChars(env, jUsername, 0);
    }

    if (jPassword != NULL)
    {
        nativePassword = (*env)->GetStringUTFChars(env, jPassword, 0);
    }

    nativeFtpConnection = calloc(1, sizeof(ARUTILS_JNI_FtpConnection_t));

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_ALLOC;
    }

    if (result == ARUTILS_OK)
    {
        resultSys = ARSAL_Sem_Init(&nativeFtpConnection->cancelSem, 0, 0);

        if (resultSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }

    if (result == ARUTILS_OK)
    {
        nativeFtpConnection->ftpConnection = ARUTILS_WifiFtp_Connection_New(&nativeFtpConnection->cancelSem, nativeServer, (int)jPort, nativeUsername, nativePassword, &result);
    }

    if (result != ARUTILS_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARUTILS_JNI_FTPCONNECTION_TAG, "error: %d occurred", result);

        if (nativeFtpConnection != NULL)
        {
            ARUTILS_WifiFtp_Connection_Delete(&nativeFtpConnection->ftpConnection);
            ARSAL_Sem_Destroy(&nativeFtpConnection->cancelSem);

            free(nativeFtpConnection);
        }

        ARUTILS_JNI_ThrowARUtilsException(env, result);
    }

    if (nativeServer != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jServer, nativeServer);
    }

    if (nativeUsername != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jUsername, nativeUsername);
    }

    if (nativePassword != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jPassword, nativePassword);
    }

    return (long)nativeFtpConnection;
}

JNIEXPORT void JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeDeleteFtpConnection(JNIEnv *env, jobject jThis, jlong jFtpConnection)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection != NULL)
    {
        ARUTILS_WifiFtp_Connection_Delete(&nativeFtpConnection->ftpConnection);

        ARSAL_Sem_Destroy(&nativeFtpConnection->cancelSem);
        free(nativeFtpConnection);
    }
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeCancel(JNIEnv *env, jobject jThis, jlong jFtpConnection)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }
    else
    {
        result = ARUTILS_WifiFtp_Connection_Cancel(nativeFtpConnection->ftpConnection);
    }

    return result;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeIsCanceled(JNIEnv *env, jobject jThis, jlong jFtpConnection)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }
    else
    {
        result = ARUTILS_WifiFtp_IsCanceled(nativeFtpConnection->ftpConnection);
    }

    return result;
}

JNIEXPORT jstring JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeList(JNIEnv *env, jobject jThis, jlong jFtpConnection, jstring jNamePath)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    const char *nativeNamePath = (*env)->GetStringUTFChars(env, jNamePath, 0);
    char *nativeResultList = NULL;
    uint32_t nativeResultListLen = 0;
    jstring jResultList = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }
    else
    {
        result = ARUTILS_WifiFtp_List(nativeFtpConnection->ftpConnection, nativeNamePath, &nativeResultList, &nativeResultListLen);
    }

    if (result == ARUTILS_OK)
    {
        jResultList = (*env)->NewStringUTF(env, nativeResultList);
    }
    else
    {
        ARUTILS_JNI_ThrowARUtilsException(env, result);
    }

    if (nativeResultList != NULL)
    {
        free(nativeResultList);
    }

    if (nativeNamePath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jNamePath, nativeNamePath);
    }

    return jResultList;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeRename(JNIEnv *env, jobject jThis, jlong jFtpConnection, jstring jOldNamePath, jstring jNewNamePath)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    const char *nativeOldNamePath = (*env)->GetStringUTFChars(env, jOldNamePath, 0);
    const char *nativeNewNamePath = (*env)->GetStringUTFChars(env, jNewNamePath, 0);
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }
    else
    {
        result = ARUTILS_WifiFtp_Rename(nativeFtpConnection->ftpConnection, nativeOldNamePath, nativeNewNamePath);
    }

    if (nativeOldNamePath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jOldNamePath, nativeOldNamePath);
    }

    if (nativeNewNamePath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jNewNamePath, nativeNewNamePath);
    }

    return result;
}

JNIEXPORT jdouble JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeSize(JNIEnv *env, jobject jThis, jlong jFtpConnection, jstring jNamePath)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    const char *nativeNamePath = (*env)->GetStringUTFChars(env, jNamePath, 0);
    jdouble jSize = 0.f;
    double nativeSize = 0.f;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }
    else
    {
        result = ARUTILS_WifiFtp_Size(nativeFtpConnection->ftpConnection, nativeNamePath, &nativeSize);
    }

    if (result == ARUTILS_OK)
    {
        jSize = (jdouble)nativeSize;
    }
    else
    {
        ARUTILS_JNI_ThrowARUtilsException(env, result);
    }

    if (nativeNamePath)
    {
        (*env)->ReleaseStringUTFChars(env, jNamePath, nativeNamePath);
    }

    return jSize;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeDelete(JNIEnv *env, jobject jThis, jlong jFtpConnection, jstring jNamePath)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    const char *nativeNamePath = (*env)->GetStringUTFChars(env, jNamePath, 0);
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }
    else
    {
        result = ARUTILS_WifiFtp_Delete(nativeFtpConnection->ftpConnection, nativeNamePath);
    }

    if (nativeNamePath)
    {
        (*env)->ReleaseStringUTFChars(env, jNamePath, nativeNamePath);
    }

    return result;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeGet(JNIEnv *env, jobject jThis, jlong jFtpConnection, jstring jNamePath, jstring jDstFile, jobject jProgressListener, jobject jProgressArg, jint jResume)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    const char *nativeNamePath = (*env)->GetStringUTFChars(env, jNamePath, 0);
    const char *nativeDstFile = (*env)->GetStringUTFChars(env, jDstFile, 0);
    ARUTILS_JNI_FtpConnectionCallbacks_t *callbacks = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }

    if (result == ARUTILS_OK)
    {
        callbacks = calloc(1, sizeof(ARUTILS_JNI_FtpConnectionCallbacks_t));

        if (callbacks == NULL)
        {
            result = ARUTILS_ERROR_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        if (jProgressListener != NULL)
        {
            callbacks->jProgressListener = (*env)->NewGlobalRef(env, jProgressListener);
        }

        if (jProgressArg != NULL)
        {
            callbacks->jProgressArg = (*env)->NewGlobalRef(env, jProgressArg);
        }
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_Get(nativeFtpConnection->ftpConnection, nativeNamePath, nativeDstFile, ARUTILS_JNI_FtpConnection_ProgressCallback, callbacks, (eARUTILS_FTP_RESUME)(int)jResume);
    }

    if (callbacks != NULL)
    {
        if (callbacks->jProgressListener != NULL)
        {
            (*env)->DeleteGlobalRef(env, callbacks->jProgressListener);
        }

        if (callbacks->jProgressArg != NULL)
        {
            (*env)->DeleteGlobalRef(env, callbacks->jProgressArg);
        }

        free(callbacks);
    }

    if (nativeNamePath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jNamePath, nativeNamePath);
    }

    if (nativeDstFile != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jDstFile, nativeDstFile);
    }

    return result;
}

JNIEXPORT jbyteArray JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativeGetWithBuffer(JNIEnv *env, jobject jThis, jlong jFtpConnection, jstring jNamePath, jobject jProgressListener, jobject jProgressArg)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    const char *nativeNamePath = (*env)->GetStringUTFChars(env, jNamePath, 0);
    ARUTILS_JNI_FtpConnectionCallbacks_t *callbacks = NULL;
    uint8_t *nativeData = NULL;
    uint32_t nativeDataLen = 0;
    jbyteArray jData = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }

    if (result == ARUTILS_OK)
    {
        callbacks = calloc(1, sizeof(ARUTILS_JNI_FtpConnectionCallbacks_t));

        if (callbacks == NULL)
        {
            result = ARUTILS_ERROR_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        if (jProgressListener != NULL)
        {
            callbacks->jProgressListener = (*env)->NewGlobalRef(env, jProgressListener);
        }

        if (jProgressArg != NULL)
        {
            callbacks->jProgressArg = (*env)->NewGlobalRef(env, jProgressArg);
        }
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_Get_WithBuffer(nativeFtpConnection->ftpConnection, nativeNamePath, &nativeData, &nativeDataLen,  ARUTILS_JNI_FtpConnection_ProgressCallback, callbacks);
    }

    if (result == ARUTILS_OK)
    {
        jData = (*env)->NewByteArray(env, nativeDataLen);

        if (jData == NULL)
        {
            result = ARUTILS_ERROR_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        (*env)->SetByteArrayRegion(env, jData, 0, nativeDataLen, (jbyte*)nativeData);
    }

    if (result != ARUTILS_OK)
    {
        ARUTILS_JNI_ThrowARUtilsException(env, result);
    }

    if (nativeData != NULL)
    {
        free(nativeData);
    }

    if (callbacks != NULL)
    {
        if (callbacks->jProgressListener != NULL)
        {
            (*env)->DeleteGlobalRef(env, callbacks->jProgressListener);
        }

        if (callbacks->jProgressArg != NULL)
        {
            (*env)->DeleteGlobalRef(env, callbacks->jProgressArg);
        }

        free(callbacks);
    }

    if (nativeNamePath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jNamePath, nativeNamePath);
    }

    return jData;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFtpConnection_nativePut(JNIEnv *env, jobject jThis, jlong jFtpConnection, jstring jNamePath, jstring jSrcFile, jobject jProgressListener, jobject jProgressArg, jint jResume)
{
    ARUTILS_JNI_FtpConnection_t *nativeFtpConnection = (ARUTILS_JNI_FtpConnection_t *)(intptr_t) jFtpConnection;
    const char *nativeNamePath = (*env)->GetStringUTFChars(env, jNamePath, 0);
    const char *nativeSrcFile = (*env)->GetStringUTFChars(env, jSrcFile, 0);
    ARUTILS_JNI_FtpConnectionCallbacks_t *callbacks = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (nativeFtpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }

    if (result == ARUTILS_OK)
    {
        callbacks = calloc(1, sizeof(ARUTILS_JNI_FtpConnectionCallbacks_t));

        if (callbacks == NULL)
        {
            result = ARUTILS_ERROR_ALLOC;
        }
    }

    if (result == ARUTILS_OK)
    {
        if (jProgressListener != NULL)
        {
            callbacks->jProgressListener = (*env)->NewGlobalRef(env, jProgressListener);
        }

        if (jProgressArg != NULL)
        {
            callbacks->jProgressArg = (*env)->NewGlobalRef(env, jProgressArg);
        }
    }

    if (result == ARUTILS_OK)
    {
        result = ARUTILS_WifiFtp_Put(nativeFtpConnection->ftpConnection, nativeNamePath, nativeSrcFile, ARUTILS_JNI_FtpConnection_ProgressCallback, callbacks, (eARUTILS_FTP_RESUME)(int)jResume);
    }

    if (callbacks != NULL)
    {
        if (callbacks->jProgressListener != NULL)
        {
            (*env)->DeleteGlobalRef(env, callbacks->jProgressListener);
        }

        if (callbacks->jProgressArg != NULL)
        {
            (*env)->DeleteGlobalRef(env, callbacks->jProgressArg);
        }

        free(callbacks);
    }

    if (nativeNamePath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jNamePath, nativeNamePath);
    }

    if (nativeSrcFile != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jSrcFile, nativeSrcFile);
    }

    return result;
}

/*****************************************
 *
 *             Private implementation:
 *
 *****************************************/

void ARUTILS_JNI_FtpConnection_ProgressCallback(void* arg, float percent)
{
    ARUTILS_JNI_FtpConnectionCallbacks_t *callbacks = (ARUTILS_JNI_FtpConnectionCallbacks_t *)arg;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (callbacks != NULL)
    {
        if ((ARUTILS_JNI_Manager_VM != NULL) && (callbacks->jProgressListener != NULL) && (methodId_FtpListener_didFtpProgress != NULL))
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

            if ((error == JNI_OK) && (methodId_FtpListener_didFtpProgress != NULL))
            {
                jPercent = percent;

                (*env)->CallVoidMethod(env, callbacks->jProgressListener, methodId_FtpListener_didFtpProgress, callbacks->jProgressArg, jPercent);
            }

            if ((jResultEnv == JNI_EDETACHED) && (env != NULL))
            {
                 (*ARUTILS_JNI_Manager_VM)->DetachCurrentThread(ARUTILS_JNI_Manager_VM);
            }
        }
    }
}

int ARUTILS_JNI_NewFtpListenersJNI(JNIEnv *env)
{
    jclass classFtpProgressListener = NULL;
    int error = JNI_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "");

    if (env == NULL)
    {
        error = JNI_FAILED;
    }

    if (methodId_FtpListener_didFtpProgress == NULL)
    {
        if (error == JNI_OK)
        {
            classFtpProgressListener = (*env)->FindClass(env, "com/parrot/arsdk/arutils/ARUtilsFtpProgressListener");

            if (classFtpProgressListener == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "ARUtilsFtpProgressListener class not found");
                error = JNI_FAILED;
            }
        }

        if (error == JNI_OK)
        {
            methodId_FtpListener_didFtpProgress = (*env)->GetMethodID(env, classFtpProgressListener, "didFtpProgress", "(Ljava/lang/Object;F)V");

            if (methodId_FtpListener_didFtpProgress == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FTPCONNECTION_TAG, "Listener didFtpProgress method not found");
            }
        }
    }

    return error;
}

