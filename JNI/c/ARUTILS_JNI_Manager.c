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

#include "ARUTILS_JNI.h"

#define ARUTILS_JNI_MANAGER_TAG       "JNI"

JavaVM* ARUTILS_JNI_Manager_VM = NULL;

jclass classException = NULL;
jmethodID methodId_Exception_Init = NULL;

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
    return 0;
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
