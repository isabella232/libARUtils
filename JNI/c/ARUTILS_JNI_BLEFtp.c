/**
 * @file  ARNETWORKAL_JNIBLENetwork.c
 * @brief private headers of BLE network manager allow to send over ble network.
 * @date 
 * @author 
 */

/*****************************************
 * 
 *             include file :
 *
 *****************************************/

#include <jni.h>
#include <stdlib.h>

#include <libARSAL/ARSAL_Print.h>
#include <libARUtils/ARUTILS_Manager.h>
#include "ARUTILS_JNI_BLEFtp.h"
//#include "../../Sources/BLE/ARNETWORKAL_BLENetwork.h"

/*****************************************
 *
 *             define :
 *
 *****************************************/

#define ARUTILS_JNI_BLEFTP_TAG "JNIBLEFtp"

extern JavaVM *ARUTILS_JNI_Manager_VM; /** reference to the java virtual machine */
 
static jmethodID ARUTILS_JNI_BLEFTP_METHOD_FTP_LIST;
static jmethodID ARUTILS_JNI_BLEFTP_METHOD_GET_WITH_BUFFER;
static jmethodID ARUTILS_JNI_BLEFTP_METHOD_GET; 
static jmethodID ARUTILS_JNI_BLEFTP_METHOD_PUT;
static jmethodID ARUTILS_JNI_BLEFTP_METHOD_DELETE;



/*****************************************
 *
 *             implementation :
 *
 *****************************************/


JNIEXPORT void JNICALL
Java_com_parrot_arsdk_arutils_ARUtilsBLEFtp_nativeJNIInit(JNIEnv *env, jobject obj)
{
    /* -- initialize the JNI part -- */
    /* load the references of java methods */
    
    jclass jBLEFtpCls = (*env)->FindClass(env, "com/parrot/arsdk/arutils/ARUtilsBLEFtp");
    
    ARUTILS_JNI_BLEFTP_METHOD_FTP_LIST = (*env)->GetMethodID(env, jBLEFtpCls, "listFiles", "(Ljava/lang/String;)Z");
    ARUTILS_JNI_BLEFTP_METHOD_GET_WITH_BUFFER = (*env)->GetMethodID(env, jBLEFtpCls, "getWithBuffer", "()V");
    ARUTILS_JNI_BLEFTP_METHOD_GET = (*env)->GetMethodID(env, jBLEFtpCls, "getFile", "(Ljava/lang/String;Ljava/lang/String;)Z");
    ARUTILS_JNI_BLEFTP_METHOD_PUT = (*env)->GetMethodID(env, jBLEFtpCls, "putFile", "(Ljava/lang/String;Ljava/lang/String;)Z");
    ARUTILS_JNI_BLEFTP_METHOD_DELETE = (*env)->GetMethodID(env, jBLEFtpCls, "deleteFile", "(Ljava/lang/String;)Z");
    
    /* cleanup */
    (*env)->DeleteLocalRef (env, jBLEFtpCls);
}


/**
 * @brief Create a new Ftp Connection
 * @warning This function allocates memory
 * @param cancelSem The pointer of the Ftp get/put cancel semaphore or null
 * @param device The BLE Ftp device
 * @param[out] error The pointer of the error code: if success ARUTILS_OK, otherwise an error number of eARUTILS_ERROR
 * @retval On success, returns an ARUTILS_FtpAL_Connection_t. Otherwise, it returns null.
 * @see ARUTILS_FtpAL_DeleteConnection ()
 */
ARUTILS_BLEFtp_Connection_t * ARUTILS_BLEFtp_Connection_New(ARSAL_Sem_t *cancelSem, ARUTILS_BLEManager_t bleManager, ARUTILS_BLEDevice_t device, int port, eARUTILS_ERROR *error)
{

    jclass bleFtpObjectCls = NULL;
    jobject bleFtpObject = NULL;

    /* Create the jniBLEFtp */

    ARUTILS_BLEFtp_Connection_t *newConnection = NULL;

    /* local declarations */
    JNIEnv *env = NULL;
    jint getEnvResult = JNI_OK;

    /* Check parameters */
    /**/
    

    /* get the environment */
    if (ARUTILS_JNI_Manager_VM != NULL)
    {
        getEnvResult = (*ARUTILS_JNI_Manager_VM)->GetEnv(ARUTILS_JNI_Manager_VM, (void **) &env, JNI_VERSION_1_6);
    }
    /* if no environment then attach the thread to the virtual machine */
    if (getEnvResult == JNI_EDETACHED)
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, "attach the thread to the virtual machine ...");
        (*ARUTILS_JNI_Manager_VM)->AttachCurrentThread(ARUTILS_JNI_Manager_VM, &env, NULL);
    }
    /* check the environment  */
    if (env == NULL)
    {
        *error = ARUTILS_ERROR;
    }

    
    if((port == 0) || (((port - 1) % 10) != 1))
    {
        *error = ARUTILS_ERROR_BAD_PARAMETER;
    }

    if (*error == ARUTILS_OK)
    {
        newConnection = calloc(1, sizeof(ARUTILS_BLEFtp_Connection_t));
        if (newConnection == NULL)
        {
            *error = ARUTILS_ERROR_ALLOC;
            
        }
        else 
        {
            // Init BLEFtp java and store in newConnection ?
            bleFtpObjectCls = (*env)->FindClass(env, "com/parrot/arsdk/arutils/ARUtilsBLEFtp");
            jmethodID bleFtpObjectMethodConstructor = (*env)->GetMethodID(env, bleFtpObjectCls, "<init>", "()V");
            bleFtpObject = (*env)->NewObject(env, bleFtpObjectCls, bleFtpObjectMethodConstructor);

            
        }
        if (bleFtpObject == NULL)
        {
            *error = ARUTILS_ERROR_ALLOC;
        }
        else
        {
            newConnection->bleFtpObject = (jobject) (*env)->NewGlobalRef(env, bleFtpObject);
            newConnection->bleManager = (jobject) (*env)->NewGlobalRef(env, bleManager);

            jmethodID bleFtpInitMethod = (*env)->GetMethodID(env, bleFtpObjectCls, "initWithDevice", "(Lcom/parrot/arsdk/arnetworkal/ARNetworkALBLEManager;Landroid/bluetooth/BluetoothGatt;I)V");
            (*env)->CallVoidMethod(env, bleFtpObject, bleFtpInitMethod, bleManager, device, port);

            jmethodID bleFtpRegisterMethod = (*env)->GetMethodID(env, bleFtpObjectCls, "registerCharacteristics", "()V");
            (*env)->CallVoidMethod(env, bleFtpObject, bleFtpRegisterMethod);
        }
    }

    /* if the thread has been attached then detach the thread from the virtual machine */
    if ((getEnvResult == JNI_EDETACHED) && (env != NULL))
    {
        (*ARUTILS_JNI_Manager_VM)->DetachCurrentThread(ARUTILS_JNI_Manager_VM);
    }

    /* cleanup */

    if (bleFtpObjectCls != NULL)
    {
        (*env)->DeleteLocalRef (env, bleFtpObjectCls );
    }

    
    if (bleFtpObject != NULL)
    {
        (*env)->DeleteLocalRef (env, bleFtpObject);    
    }
    
    
    return newConnection;
}

/**
 * @brief Delete an Ftp Connection
 * @warning This function frees memory
 * @param connection The address of the pointer on the Ftp Connection
 * @see ARUTILS_FtpAL_NewConnection ()
 */
void ARUTILS_BLEFtp_Connection_Delete(ARUTILS_BLEFtp_Connection_t **connectionAddr)
{

    eARUTILS_ERROR error = ARUTILS_OK;

    /* local declarations */
    JNIEnv *env = NULL;
    jint getEnvResult = JNI_OK;

    /* Check parameters */
    /**/
    

    /* get the environment */
    if (ARUTILS_JNI_Manager_VM != NULL)
    {
        getEnvResult = (*ARUTILS_JNI_Manager_VM)->GetEnv(ARUTILS_JNI_Manager_VM, (void **) &env, JNI_VERSION_1_6);
    }
    /* if no environment then attach the thread to the virtual machine */
    if (getEnvResult == JNI_EDETACHED)
    {
        ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, "attach the thread to the virtual machine ...");
        (*ARUTILS_JNI_Manager_VM)->AttachCurrentThread(ARUTILS_JNI_Manager_VM, &env, NULL);
    }
    /* check the environment  */
    if (env == NULL)
    {
        error = ARUTILS_ERROR;
    }

    if ((error == ARUTILS_OK) && (connectionAddr != NULL))
    {
        ARUTILS_BLEFtp_Connection_t *connection = *connectionAddr;
        if (connection != NULL)
        {
            // bleFTP java unregisterCharacteristics
            (*env)->DeleteGlobalRef (env, connection->bleFtpObject);
            (*env)->DeleteGlobalRef (env, connection->bleManager);

            connection->bleFtpObject = NULL;
            connection->bleManager = NULL;
            
            free(connection);
        }
        *connectionAddr = NULL;
    }
}

/**
 * @brief Cancel an Ftp Connection command in progress (get, put, list etc)
 * @param connection The address of the pointer on the Ftp Connection
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Ftp_NewConnection ()
 */
eARUTILS_ERROR ARUTILS_BLEFtp_Connection_Cancel(ARUTILS_BLEFtp_Connection_t *connection)
{
    /* -- cancel the BLEFtp connection -- */
    
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, " BLEFtp_Connection_Cancel ");

    eARUTILS_ERROR result = ARUTILS_OK;
    int resutlSys = 0;
    
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, "");
    
    if ((connection == NULL) || (connection->cancelSem == NULL))
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (result == ARUTILS_OK)
    {
        resutlSys = ARSAL_Sem_Post(connection->cancelSem);
        
        if (resutlSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }
    
    return result;
}

/**
 * @brief Check if the connection has received a cancel to it's semaphore
 * @param connection The address of the pointer on the Ftp Connection
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see cURL
 */
eARUTILS_ERROR ARUTILS_BLEFtp_IsCanceled(ARUTILS_BLEFtp_Connection_t *connection)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (connection == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if ((connection != NULL) && (connection->cancelSem != NULL))
    {
        int resultSys = ARSAL_Sem_Trywait(connection->cancelSem);
        
        if (resultSys == 0)
        {
            result = ARUTILS_ERROR_FTP_CANCELED;
            
            //give back the signal state lost from trywait
            resultSys = ARSAL_Sem_Post(connection->cancelSem);
        }
        else if (resultSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }
    
    return result;
}

/**
 * @brief Execute Ftp List command to retrieve directory content
 * @warning This function allocates memory
 * @param connection The address of the pointer on the Ftp Connection
 * @param namePath The string of the directory path on the remote Ftp server
 * @param resultList The pointer of the string of the directory content null terminated
 * @param resultListLen The pointer of the lenght of the resultList string including null terminated
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_BLEFtp_NewConnection ()
 */
eARUTILS_ERROR ARUTILS_BLEFtp_List(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, char **resultList, uint32_t *resultListLen)
{
    
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, " BLEFtp_list ");

    /* local declarations */
    JNIEnv *env = NULL;
    jint getEnvResult = JNI_OK;
    eARUTILS_ERROR error = ARUTILS_OK;

    /* Check parameters */
    if ((connection == NULL) || (remotePath == NULL) || (resultList == NULL) || (connection->bleFtpObject == NULL) )
    {
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (error == ARUTILS_OK)
    {
        /* get the environment */
        if (ARUTILS_JNI_Manager_VM != NULL)
        {
            getEnvResult = (*ARUTILS_JNI_Manager_VM)->GetEnv(ARUTILS_JNI_Manager_VM, (void **) &env, JNI_VERSION_1_6);
        }
        /* if no environment then attach the thread to the virtual machine */
        if (getEnvResult == JNI_EDETACHED)
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, "attach the thread to the virtual machine ...");
            (*ARUTILS_JNI_Manager_VM)->AttachCurrentThread(ARUTILS_JNI_Manager_VM, &env, NULL);
        }
        /* check the environment  */
        if (env == NULL)
        {
            error = ARUTILS_ERROR;
        }
    }
    
    if (error == ARUTILS_OK)
    {
        
        jobject bleFtpObject = connection->bleFtpObject;
        
        jstring jRemotePath = (*env)->NewStringUTF(env, remotePath);

        jstring jList = (*env)->CallObjectMethod(env, bleFtpObject, ARUTILS_JNI_BLEFTP_METHOD_FTP_LIST, jRemotePath);
        
        if (jList == NULL)
        {
            error = ARUTILS_ERROR_BLE_FAILED;
        }
        else
        {
            const char *dataList = (*env)->GetStringUTFChars(env, jList, 0);

            int dataLen = strlen(dataList);

            char *stringList = malloc(dataLen + 1);
            if (stringList == NULL)
            {
                error = ARUTILS_ERROR_ALLOC;
            }
        
            if (error == ARUTILS_OK)
            {
                memcpy(stringList, dataList, dataLen);
                stringList[dataLen] = '\0';
                
                *resultList = stringList;
                *resultListLen = dataLen + 1;
            }
            
            if (dataList != NULL)
            {
                (*env)->ReleaseStringUTFChars(env, jList, dataList);
            }
        }
        
    }
    
    /* if the thread has been attached then detach the thread from the virtual machine */
    if ((getEnvResult == JNI_EDETACHED) && (env != NULL))
    {
        (*ARUTILS_JNI_Manager_VM)->DetachCurrentThread(ARUTILS_JNI_Manager_VM);
    }

    return error;
}

/**
 * @brief Delete an remote Ftp server file
 * @param connection The address of the pointer on the Ftp Connection
 * @param namePath The string of the file name path on the remote Ftp server
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Ftp_NewConnection ()
 */
eARUTILS_ERROR ARUTILS_BLEFtp_Delete(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath)
{
    
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, " BLEFtp_Delete ");

    /* local declarations */
    JNIEnv *env = NULL;
    jint getEnvResult = JNI_OK;
    eARUTILS_ERROR error = ARUTILS_OK;
    jboolean ret = 0;

    /* Check parameters */
    if ((connection == NULL) || (connection->bleFtpObject == NULL) || (remotePath == NULL))
    {
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (error == ARUTILS_OK)
    {
        /* get the environment */
        if (ARUTILS_JNI_Manager_VM != NULL)
        {
            getEnvResult = (*ARUTILS_JNI_Manager_VM)->GetEnv(ARUTILS_JNI_Manager_VM, (void **) &env, JNI_VERSION_1_6);
        }
        /* if no environment then attach the thread to the virtual machine */
        if (getEnvResult == JNI_EDETACHED)
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, "attach the thread to the virtual machine ...");
            (*ARUTILS_JNI_Manager_VM)->AttachCurrentThread(ARUTILS_JNI_Manager_VM, &env, NULL);
        }
        /* check the environment  */
        if (env == NULL)
        {
            error = ARUTILS_ERROR;
        }
    }
    
    if (error == ARUTILS_OK)
    {   
        jobject bleFtpObject = connection->bleFtpObject;
        jstring jRemotePath = (*env)->NewStringUTF(env, remotePath);

        ret = (*env)->CallBooleanMethod(env, bleFtpObject, ARUTILS_JNI_BLEFTP_METHOD_DELETE, jRemotePath);
        if (ret == 0)
        {
            error = ARUTILS_ERROR_BLE_FAILED;
        }
    }
    
    /* if the thread has been attached then detach the thread from the virtual machine */
    if ((getEnvResult == JNI_EDETACHED) && (env != NULL))
    {
        (*ARUTILS_JNI_Manager_VM)->DetachCurrentThread(ARUTILS_JNI_Manager_VM);
    }

    return error;
}

/**
 * @brief Get an remote Ftp server file
 * @warning This function allocates memory
 * @param connection The address of the pointer on the Ftp Connection
 * @param namePath The string of the file name path on the remote Ftp server
 * @param data The pointer of the data buffer of the file data
 * @param dataLen The pointer of the length of the data buffer
 * @param progressCallback The progress callback function
 * @param progressArg The progress callback function arg
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_BLEFtp_NewConnection (), ARUTILS_Ftp_ProgressCallback_t, eARUTILS_FTP_RESUME
 */
eARUTILS_ERROR ARUTILS_BLEFtp_Get_WithBuffer(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, uint8_t **data, uint32_t *dataLen,  ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg)
{
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, " BLEFtp_Get_WithBuffer ");

    /* local declarations */
    JNIEnv *env = NULL;
    jint getEnvResult = JNI_OK;
    eARUTILS_ERROR error = ARUTILS_OK;
    jboolean ret = 0;

    /* Check parameters */
    if ((connection == NULL) || (connection->bleFtpObject == NULL) || (remotePath == NULL))
    {
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (error == ARUTILS_OK)
    {
        /* get the environment */
        if (ARUTILS_JNI_Manager_VM != NULL)
        {
            getEnvResult = (*ARUTILS_JNI_Manager_VM)->GetEnv(ARUTILS_JNI_Manager_VM, (void **) &env, JNI_VERSION_1_6);
        }
        /* if no environment then attach the thread to the virtual machine */
        if (getEnvResult == JNI_EDETACHED)
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, "attach the thread to the virtual machine ...");
            (*ARUTILS_JNI_Manager_VM)->AttachCurrentThread(ARUTILS_JNI_Manager_VM, &env, NULL);
        }
        /* check the environment  */
        if (env == NULL)
        {
            error = ARUTILS_ERROR;
        }
    }
    
    if (error == ARUTILS_OK)
    {
        
        jobject bleFtpObject = connection->bleFtpObject;
        jstring jRemotePath = (*env)->NewStringUTF(env, remotePath);

        //ret = (*env)->CallBooleanMethod(env, bleFtpObject, ARUTILS_JNI_BLEFTP_METHOD_GET_WITH_BUFFER);
        if (ret == 0)
        {
            error = ARUTILS_ERROR_BLE_FAILED;
        }
    }
    
    /* if the thread has been attached then detach the thread from the virtual machine */
    if ((getEnvResult == JNI_EDETACHED) && (env != NULL))
    {
        (*ARUTILS_JNI_Manager_VM)->DetachCurrentThread(ARUTILS_JNI_Manager_VM);
    }

    return error;
}

/**
 * @brief Get an remote Ftp server file
 * @param connection The address of the pointer on the Ftp Connection
 * @param namePath The string of the file name path on the remote Ftp server
 * @param dstFile The string of the local file name path to be get
 * @param progressCallback The progress callback function
 * @param progressArg The progress callback function arg
 * @param resume The resume capability requested
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_BLEFtp_NewConnection (), ARUTILS_Ftp_ProgressCallback_t, eARUTILS_FTP_RESUME
 */
eARUTILS_ERROR ARUTILS_BLEFtp_Get(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, " BLEFtp_Get ");

    /* local declarations */
    JNIEnv *env = NULL;
    jint getEnvResult = JNI_OK;
    eARUTILS_ERROR error = ARUTILS_OK;
    jboolean ret = 0;

    /* Check parameters */
    if ((connection == NULL) || (connection->bleFtpObject == NULL) || (remotePath == NULL) || (dstFile == NULL))
    {
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (error == ARUTILS_OK)
    {
        /* get the environment */
        if (ARUTILS_JNI_Manager_VM != NULL)
        {
            getEnvResult = (*ARUTILS_JNI_Manager_VM)->GetEnv(ARUTILS_JNI_Manager_VM, (void **) &env, JNI_VERSION_1_6);
        }
        /* if no environment then attach the thread to the virtual machine */
        if (getEnvResult == JNI_EDETACHED)
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, "attach the thread to the virtual machine ...");
            (*ARUTILS_JNI_Manager_VM)->AttachCurrentThread(ARUTILS_JNI_Manager_VM, &env, NULL);
        }
        /* check the environment  */
        if (env == NULL)
        {
            error = ARUTILS_ERROR;
        }
    }
    
    if (error == ARUTILS_OK)
    {   
        jobject bleFtpObject = connection->bleFtpObject;
        jstring jRemotePath = (*env)->NewStringUTF(env, remotePath);
        jstring jDstFile = (*env)->NewStringUTF(env, dstFile);

        ret = (*env)->CallBooleanMethod(env, bleFtpObject, ARUTILS_JNI_BLEFTP_METHOD_GET, jRemotePath, jDstFile);
        if (ret == 0)
        {
            error = ARUTILS_ERROR_BLE_FAILED;
        }
    }
    
    /* if the thread has been attached then detach the thread from the virtual machine */
    if ((getEnvResult == JNI_EDETACHED) && (env != NULL))
    {
        (*ARUTILS_JNI_Manager_VM)->DetachCurrentThread(ARUTILS_JNI_Manager_VM);
    }

    return error;
}

/**
 * @brief Put an remote Ftp server file
 * @param connection The address of the pointer on the Ftp Connection
 * @param namePath The string of the file name path on the remote Ftp server
 * @param srcFile The string of the local file name path to be put
 * @param progressCallback The progress callback function
 * @param progressArg The progress callback function arg
 * @param resume The resume capability requested
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_BLEFtp_NewConnection (), ARUTILS_Ftp_ProgressCallback_t, eARUTILS_FTP_RESUME
 */
eARUTILS_ERROR ARUTILS_BLEFtp_Put(ARUTILS_BLEFtp_Connection_t *connection, const char *remotePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, " BLEFtp_Put ");

    /* local declarations */
    JNIEnv *env = NULL;
    jint getEnvResult = JNI_OK;
    eARUTILS_ERROR error = ARUTILS_OK;
    jboolean ret = 0;

    /* Check parameters */
    if ((connection == NULL) || (connection->bleFtpObject == NULL) || (remotePath == NULL) || (srcFile == NULL))
    {
        error = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if (error == ARUTILS_OK)
    {
        /* get the environment */
        if (ARUTILS_JNI_Manager_VM != NULL)
        {
            getEnvResult = (*ARUTILS_JNI_Manager_VM)->GetEnv(ARUTILS_JNI_Manager_VM, (void **) &env, JNI_VERSION_1_6);
        }
        /* if no environment then attach the thread to the virtual machine */
        if (getEnvResult == JNI_EDETACHED)
        {
            ARSAL_PRINT(ARSAL_PRINT_DEBUG, ARUTILS_JNI_BLEFTP_TAG, "attach the thread to the virtual machine ...");
            (*ARUTILS_JNI_Manager_VM)->AttachCurrentThread(ARUTILS_JNI_Manager_VM, &env, NULL);
        }
        /* check the environment  */
        if (env == NULL)
        {
            error = ARUTILS_ERROR;
        }
    }
    
    if (error == ARUTILS_OK)
    {   
        jobject bleFtpObject = connection->bleFtpObject;
        jstring jRemotePath = (*env)->NewStringUTF(env, remotePath);
        jstring jSrcFile = (*env)->NewStringUTF(env, srcFile);

        ret = (*env)->CallBooleanMethod(env, bleFtpObject, ARUTILS_JNI_BLEFTP_METHOD_PUT, jRemotePath, jSrcFile);
        if (ret == 0)
        {
            error = ARUTILS_ERROR_BLE_FAILED;
        }
    }
    
    /* if the thread has been attached then detach the thread from the virtual machine */
    if ((getEnvResult == JNI_EDETACHED) && (env != NULL))
    {
        (*ARUTILS_JNI_Manager_VM)->DetachCurrentThread(ARUTILS_JNI_Manager_VM);
    }

    return error;
}

/**
 * @brief Cancel an Ftp Connection command in progress (get, put, list etc)
 * @param cancelSem The address of the pointer on the Ftp Connection
 * @retval On success, returns ARUTILS_OK. Otherwise, it returns an error number of eARUTILS_ERROR.
 * @see ARUTILS_Manager_NewBLEFtp ()
 */
eARUTILS_ERROR ARUTILS_BLEFtp_IsCanceledSem(ARSAL_Sem_t *cancelSem)
{
    eARUTILS_ERROR result = ARUTILS_OK;
    
    if (cancelSem == NULL)
    {
        result = ARUTILS_ERROR_BAD_PARAMETER;
    }
    
    if ((cancelSem != NULL))
    {
        int resultSys = ARSAL_Sem_Trywait(cancelSem);
        
        if (resultSys == 0)
        {
            result = ARUTILS_ERROR_FTP_CANCELED;
            
            //give back the signal state lost from trywait
            resultSys = ARSAL_Sem_Post(cancelSem);
        }
        else if (resultSys != 0)
        {
            result = ARUTILS_ERROR_SYSTEM;
        }
    }
    
    return result;
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Connection_Cancel(ARUTILS_Manager_t *manager)
{
    return ARUTILS_BLEFtp_Connection_Cancel((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Connection_IsCanceled(ARUTILS_Manager_t *manager)
{
    return ARUTILS_BLEFtp_IsCanceled((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_List(ARUTILS_Manager_t *manager, const char *namePath, char **resultList, uint32_t *resultListLen)
{
    return ARUTILS_BLEFtp_List((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, resultList, resultListLen);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Get_WithBuffer(ARUTILS_Manager_t *manager, const char *namePath, uint8_t **data, uint32_t *dataLen,  ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg)
{
    return ARUTILS_BLEFtp_Get_WithBuffer((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, data, dataLen, progressCallback, progressArg);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Get(ARUTILS_Manager_t *manager, const char *namePath, const char *dstFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    return ARUTILS_BLEFtp_Get((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, dstFile, progressCallback, progressArg, resume);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Put(ARUTILS_Manager_t *manager, const char *namePath, const char *srcFile, ARUTILS_Ftp_ProgressCallback_t progressCallback, void* progressArg, eARUTILS_FTP_RESUME resume)
{
    return ARUTILS_BLEFtp_Put((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath, srcFile, progressCallback, progressArg, resume);
}

eARUTILS_ERROR ARUTILS_BLEFtpAL_Delete(ARUTILS_Manager_t *manager, const char *namePath)
{
    return ARUTILS_BLEFtp_Delete((ARUTILS_BLEFtp_Connection_t *)manager->connectionObject, namePath);
}
