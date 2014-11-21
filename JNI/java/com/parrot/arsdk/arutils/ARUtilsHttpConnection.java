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

package com.parrot.arsdk.arutils;

/**
 * ARUtils Http module
 * @author david.flattin.ext@parrot.com
 * @date 30/12/2013
 */
public class ARUtilsHttpConnection
{
    /* Native Functions */
    private native static boolean nativeStaticInit();
    private native long nativeNewHttpConnection(String server, int port, int security, String username, String password) throws ARUtilsException;
    private native void nativeDeleteHttpConnection(long fptConnection);
    private native int nativeCancel(long fptConnection);
    private native int nativeIsCanceled(long fptConnection);
    private native int nativeGet(long fptConnection, String namePath, String dstFile, ARUtilsHttpProgressListener progressListener, Object progressArg);
    private native byte[] nativeGetWithBuffer(long fptConnection, String namePath, ARUtilsHttpProgressListener progressListener, Object progressArg) throws ARUtilsException;

    /*  Members  */
    private boolean isInit = false;
    private long nativeHttpConnection = 0;

    public static int HTTP_PORT = 80;
    public static int HTTPS_PORT = 443;

    /*  Java Methods */

    /**
     * Creates ARUtils Http
     * @return void
     * @throws ARUtilsException if error
     */
    public void createHttpConnection(String server, int port, ARUTILS_HTTPS_PROTOCOL_ENUM security, String username, String password) throws ARUtilsException
    {
        if (isInit == false)
        {
            nativeHttpConnection = nativeNewHttpConnection(server, port, security.getValue(), username, password);
        }

        if (0 != nativeHttpConnection)
        {
            isInit = true;
        }
    }

    /**
     * Closes ARUtils Http
     * @return void
     */
    public void closeHttpConnection()
    {
    	if (nativeHttpConnection != 0)
        {
            nativeDeleteHttpConnection(nativeHttpConnection);
            nativeHttpConnection = 0;
            isInit = false;
        }
    }

    /**
     * Gets the ARUtils Http Connection status Initialized or not
     * @return true if the Http Connection is already created else false
     */
    public boolean isInitialized()
    {
        return isInit;
    }

    /**
     * Cancels the ARUtils Http Connection
     * @return ARUTILS_OK if success, else an {@link ARUTILS_ERROR_ENUM} error code
     */
    public ARUTILS_ERROR_ENUM cancel()
    {
        int result = nativeCancel(nativeHttpConnection);

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        return error;
    }
    
    /**
     * Get Canceled status of the ARUtils Http Connection
     * @return ARUTILS_OK if success, else an {@link ARUTILS_ERROR_ENUM} error code, ARUTILS_ERROR_HTTP_CANCELED if canceled
     */
    public ARUTILS_ERROR_ENUM isCanceled()
    {
        int result = nativeIsCanceled(nativeHttpConnection);
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        return error;        
    }

    /**
     * Gets file on the Http server
     * @param namePath The server file path name
     * @return dstFile The destination local file path name
     * @return progressListener The progress listener
     * @return progressArg The progress arg
     * @return resume The resume mode requested
     * @return void
     * @throws ARUtilsException if error
     */
    //public void get(String namePath, String dstFile, ARUtilsHttpProgressListener progressListener, Object progressArg, ARUTILS_Http_RESUME_ENUM resume) throws ARUtilsException
    public void get(String namePath, String dstFile, ARUtilsHttpProgressListener progressListener, Object progressArg) throws ARUtilsException
    {
        //int result = nativeGet(nativeHttpConnection, namePath, dstFile, progressListener, progressArg, resume.getValue());
        int result = nativeGet(nativeHttpConnection, namePath, dstFile, progressListener, progressArg);

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);

        if (error != ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            throw new ARUtilsException(error);
        }
    }

    /**
     * Gets file on the Http server
     * @param namePath The server file path name
     * @return progressListener The progress listener
     * @return progressArg The progress arg
     * @return resume The resume mode requested
     * @return the bytes array of the file data
     * @throws ARUtilsException if error
     */
    public byte[] getWithBuffer(String namePath, ARUtilsHttpProgressListener progressListener, Object progressArg) throws ARUtilsException
    {
        return nativeGetWithBuffer(nativeHttpConnection, namePath, progressListener, progressArg);
    }

    /*  Static Block */
    static
    {
        nativeStaticInit();
    }
}

