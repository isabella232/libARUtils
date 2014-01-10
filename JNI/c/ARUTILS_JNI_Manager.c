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
#include "libARUtils/ARUTILS_Http.h"
#include "libARUtils/ARUTILS_Ftp.h"

#include "ARUTILS_JNI.h"

#define ARUTILS_JNI_MANAGER_TAG       "JNI"

JavaVM* ARUTILS_JNI_Manager_VM = NULL;

jclass classException = NULL;
jmethodID methodId_Exception_Init = NULL;

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
