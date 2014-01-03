/**
 * @file ARUTILS_JNI_FileSystem.c
 * @brief libARUtils JNI_FileSystem c file.
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
#include "libARUtils/ARUTILS_Ftp.h"

#include "ARUTILS_JNI.h"

#define ARUTILS_JNI_FILESYSTEM_TAG       "JNI"

jclass classException = NULL;
jmethodID methodId_Exception_Init = NULL;

/*****************************************
 *
 *             Public implementation:
 *
 *****************************************/

JNIEXPORT jboolean JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFileSystem_nativeStaticInit(JNIEnv *env, jclass jClass)
{
    jboolean jret = JNI_FALSE;
    int error = JNI_OK;

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
        jret = JNI_TRUE;
    }

    return jret;
}

JNIEXPORT jlong JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFileSystem_nativeGetFileSize(JNIEnv *env, jobject jThis, jstring jNamePath)
{
    const char *nativeNamePath = (*env)->GetStringUTFChars(env, jNamePath, 0);
    uint32_t nativeSize = 0;
    jlong jSize = 0;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "");

    result = ARUTILS_FileSystem_GetFileSize(nativeNamePath, &nativeSize);

    if (result == ARUTILS_OK)
    {
        jSize = (jlong)nativeSize;
    }
    else
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARUTILS_JNI_FILESYSTEM_TAG, "error: %d occurred", result);

        ARUTILS_JNI_ThrowARUtilsException(env, result);
    }

    if (nativeNamePath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jNamePath, nativeNamePath);
    }

    return jSize;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFileSystem_nativeRename(JNIEnv *env, jobject jThis, jstring jOldName, jstring jNewName)
{
    const char *nativeOldName = (*env)->GetStringUTFChars(env, jOldName, 0);
    const char *nativeNewName = (*env)->GetStringUTFChars(env, jNewName, 0);
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "");

    result = ARUTILS_FileSystem_Rename(nativeOldName, nativeNewName);

    if (nativeOldName != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jOldName, nativeOldName);
    }

    if (nativeNewName != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jNewName, nativeNewName);
    }

    return result;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFileSystem_nativeRemoveFile(JNIEnv *env, jobject jThis, jstring jLocalPath)
{
    const char *nativeLocalPath = (*env)->GetStringUTFChars(env, jLocalPath, 0);
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "");

    result = ARUTILS_FileSystem_RemoveFile(nativeLocalPath);

    if (nativeLocalPath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jLocalPath, nativeLocalPath);
    }

    return result;
}

JNIEXPORT jint JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFileSystem_nativeRemoveDir(JNIEnv *env, jobject jThis, jstring jLocalPath)
{
    const char *nativeLocalPath = (*env)->GetStringUTFChars(env, jLocalPath, 0);
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "");

    result = ARUTILS_FileSystem_RemoveDir(nativeLocalPath);

    if (nativeLocalPath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jLocalPath, nativeLocalPath);
    }

    return result;
}

JNIEXPORT jdouble JNICALL Java_com_parrot_arsdk_arutils_ARUtilsFileSystem_nativeGetFreeSpace(JNIEnv *env, jobject jThis, jstring jLocalPath)
{
    const char *nativeLocalPath = (*env)->GetStringUTFChars(env, jLocalPath, 0);
    jdouble jFreeSpace = 0.f;
    double nativeFreeSpace = 0.f;
    eARUTILS_ERROR result = ARUTILS_OK;

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "");

    result = ARUTILS_FileSystem_GetFreeSpace(nativeLocalPath, &nativeFreeSpace);

    if (result == ARUTILS_OK)
    {
        jFreeSpace = (jdouble)nativeFreeSpace;
    }
    else
    {
        ARSAL_PRINT (ARSAL_PRINT_ERROR, ARUTILS_JNI_FILESYSTEM_TAG, "error: %d occurred", result);

        ARUTILS_JNI_ThrowARUtilsException(env, result);
    }

    if (nativeLocalPath != NULL)
    {
        (*env)->ReleaseStringUTFChars(env, jLocalPath, nativeLocalPath);
    }

    return jFreeSpace;
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
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "");

        if (env == NULL)
        {
           error = JNI_FAILED;
        }

        if (error == JNI_OK)
        {
            locClassException = (*env)->FindClass(env, "com/parrot/arsdk/arutils/ARUtilsException");

            if (locClassException == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "ARUtilsException class not found");
                error = JNI_FAILED;
            }
            else
            {
                classException = (*env)->NewGlobalRef(env, locClassException);

                if (classException == NULL)
                {
                    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "NewGlobalRef class failed");
                    error = JNI_FAILED;
                }
            }
        }

        if (error == JNI_OK)
        {
            methodId_Exception_Init = (*env)->GetMethodID(env, classException, "<init>", "(I)V");

            if (methodId_Exception_Init == NULL)
            {
                ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "init method not found");
                error = JNI_FAILED;
            }
        }
    }

    return error;
}

void ARUTILS_JNI_FreeARUtilsExceptionJNI(JNIEnv *env)
{
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "");

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

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "%d", nativeError);

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

    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_FILESYSTEM_TAG, "%d", error);

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

