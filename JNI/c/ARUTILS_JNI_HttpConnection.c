/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
/**
 * @file ARUTILS_JNI_HttpConnection.c
 * @brief libARUtils JNI_HttpConnection c file.
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

#define ARUTILS_JNI_HTTPCONNECTION_TAG       "JNI"

jmethodID methodId_HttpListener_didHttpProgress = NULL;

/*****************************************
 *
 *             Public implementation:
 *
 *****************************************/

JNIEXPORT jboolean JNICALL Java_com_parrot_arsdk_arutils_ARUtilsHttpConnection_nativeStaticInit(JNIEnv *env, jclass jClass)
{
    jboolean jret = JNI_FALSE;
    int error = JNI_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "");

    if (env == NULL)
    {
        error = JNI_FAILED;
    }

    if (error == JNI_OK)
    {
        error = ARUTILS_JNI_NewHttpListenersJNI(env);
    }

    if (error == JNI_OK)
    {
        jret = JNI_TRUE;
    }

    return jret;
}

JNIEXPORT jlong JNICALL Java_com_parrot_arsdk_arutils_ARUtilsHttpConnection_nativeNewHttpConnection(JNIEnv *env, jobject jThis, jstring jServer, jint jPort, jint jSecurity, jstring jUsername, jstring jPassword)
{
    ARUTILS_JNI_HttpConnection_t *nativeHttpConnection = NULL;
    const char *nativeServer = (*env)->GetStringUTFChars(env, jServer, 0);
    const char *nativeUsername = NULL;
    const char *nativePassword = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;
    int resultSys = 0;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "");

    if (jUsername != NULL)
    {
        nativeUsername = (*env)->GetStringUTFChars(env, jUsername, 0);
    }

    if (jPassword != NULL)
    {
        nativePassword = (*env)->GetStringUTFChars(env, jPassword, 0);
    }

    nativeHttpConnection = calloc(1, sizeof(ARUTILS_JNI_HttpConnection_t));

    if (nativeHttpConnection == NULL)
    {
        result = ARUTILS_ERROR_ALLOC;
    }

    if (result == ARUTILS_OK)
    {
        resultSys = ARSAL_Sem_Init(&nativeHttpConnection->cancelSem, 0, 0);

        if (resultSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }

    if (result == ARUTILS_OK)
    {
        nativeHttpConnection->httpConnection = ARUTILS_Http_Connection_New(&nativeHttpConnection->cancelSem, nativeServer, (int)jPort, (int)jSecurity, nativeUsername, nativePassword, &result);
    }

    if (result != ARUTILS_OK)
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARUTILS_JNI_HTTPCONNECTION_TAG, "error: %d occurred", result);

        if (nativeHttpConnection != NULL)
        {
            ARUTILS_Http_Connection_Delete(&nativeHttpConnection->httpConnection);
            ARSAL_Sem_Destroy(&nativeHttpConnection->cancelSem);

            free(nativeHttpConnection);
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

    return (long)nativeHttpConnection;
}

JNIEXPORT void JNICALL Java_com_parrot_arsdk_arutils_ARUtilsHttpConnection_nativeDeleteHttpConnection(JNIEnv *env, jobject jThis, jlong jHttpConnection)
{
    ARUTILS_JNI_HttpConnection_t *nativeHttpConnection = (ARUTILS_JNI_HttpConnection_t *)(intptr_t) jHttpConnection;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "");

    if (nativeHttpConnection != NULL)
    {
        ARUTILS_Http_Connection_Delete(&nativeHttpConnection->httpConnection);

        ARSAL_Sem_Destroy(&nativeHttpConnection->cancelSem);
        free(nativeHttpConnection);
    }
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsHttpConnection_nativeCancel(JNIEnv *env, jobject jThis, jlong jHttpConnection)
{
    ARUTILS_JNI_HttpConnection_t *nativeHttpConnection = (ARUTILS_JNI_HttpConnection_t *)(intptr_t) jHttpConnection;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "");

    if (nativeHttpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }
    else
    {
        result = ARUTILS_Http_Connection_Cancel(nativeHttpConnection->httpConnection);
    }

    return result;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsHttpConnection_nativeIsCanceled(JNIEnv *env, jobject jThis, jlong jHttpConnection)
{
    ARUTILS_JNI_HttpConnection_t *nativeHttpConnection = (ARUTILS_JNI_HttpConnection_t *)(intptr_t) jHttpConnection;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "");

    if (nativeHttpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }
    else
    {
        result = ARUTILS_Http_IsCanceled(nativeHttpConnection->httpConnection);
    }

    return result;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsHttpConnection_nativeGet(JNIEnv *env, jobject jThis, jlong jHttpConnection, jstring jNamePath, jstring jDstFile, jobject jProgressListener, jobject jProgressArg)
{
    ARUTILS_JNI_HttpConnection_t *nativeHttpConnection = (ARUTILS_JNI_HttpConnection_t *)(intptr_t) jHttpConnection;
    const char *nativeNamePath = (*env)->GetStringUTFChars(env, jNamePath, 0);
    const char *nativeDstFile = (*env)->GetStringUTFChars(env, jDstFile, 0);
    ARUTILS_JNI_HttpConnectionCallbacks_t *callbacks = NULL;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "");

    if (nativeHttpConnection == NULL)
    {
        result = ARUTILS_ERROR_SYSTEM;
    }

    if (result == ARUTILS_OK)
    {
        callbacks = calloc(1, sizeof(ARUTILS_JNI_HttpConnectionCallbacks_t));

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
        result = ARUTILS_Http_Get(nativeHttpConnection->httpConnection, nativeNamePath, nativeDstFile, ARUTILS_JNI_HttpConnection_ProgressCallback, callbacks);
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

JNIEXPORT jbyteArray JNICALL Java_com_parrot_arsdk_arutils_ARUtilsHttpConnection_nativeGetWithBuffer(JNIEnv *env, jobject jThis, jlong jHttpConnection, jstring jNamePath, jobject jProgressListener, jobject jProgressArg)
{
    return 0;
}

/*****************************************
 *
 *             Private implementation:
 *
 *****************************************/

void ARUTILS_JNI_HttpConnection_ProgressCallback(void* arg, float percent)
{
    ARUTILS_JNI_HttpConnectionCallbacks_t *callbacks = (ARUTILS_JNI_HttpConnectionCallbacks_t *)arg;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "");

    if (callbacks != NULL)
    {
        if ((ARUTILS_JNI_Manager_VM != NULL) && (callbacks->jProgressListener != NULL) && (methodId_HttpListener_didHttpProgress != NULL))
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

            if ((error == JNI_OK) && (methodId_HttpListener_didHttpProgress != NULL))
            {
                jPercent = percent;

                (*env)->CallVoidMethod(env, callbacks->jProgressListener, methodId_HttpListener_didHttpProgress, callbacks->jProgressArg, jPercent);
            }

            if ((jResultEnv == JNI_EDETACHED) && (env != NULL))
            {
                 (*ARUTILS_JNI_Manager_VM)->DetachCurrentThread(ARUTILS_JNI_Manager_VM);
            }
        }
    }
}

int ARUTILS_JNI_NewHttpListenersJNI(JNIEnv *env)
{
    jclass classHttpProgressListener = NULL;
    int error = JNI_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "");

    if (env == NULL)
    {
        error = JNI_FAILED;
    }

    if (methodId_HttpListener_didHttpProgress == NULL)
    {
        if (error == JNI_OK)
        {
            classHttpProgressListener = (*env)->FindClass(env, "com/parrot/arsdk/arutils/ARUtilsHttpProgressListener");

            if (classHttpProgressListener == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "ARUtilsHttpProgressListener class not found");
                error = JNI_FAILED;
            }
        }

        if (error == JNI_OK)
        {
            methodId_HttpListener_didHttpProgress = (*env)->GetMethodID(env, classHttpProgressListener, "didHttpProgress", "(Ljava/lang/Object;F)V");

            if (methodId_HttpListener_didHttpProgress == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_HTTPCONNECTION_TAG, "Listener didHttpProgress method not found");
            }
        }
    }

    return error;
}
