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
 * ARUtils Ftp module
 * @author david.flattin.ext@parrot.com
 * @date 30/12/2013
 */
public class ARUtilsFtpConnection
{
    /* Native Functions */
    private native static boolean nativeStaticInit();
    private native long nativeNewFtpConnection(String server, int port, String username, String password) throws ARUtilsException;
    private native void nativeDeleteFtpConnection(long fptConnection);
    private native int nativeDisconnect(long fptConnection);
    private native int nativeReconnect(long fptConnection);
    private native int nativeCancel(long fptConnection);
    private native int nativeIsCanceled(long fptConnection);
    private native int nativeReset(long fptConnection);
    private native String nativeList(long fptConnection, String namePath) throws ARUtilsException;
    private native int nativeRename(long fptConnection, String oldNamePath, String newNamePath);
    private native double nativeSize(long fptConnection, String namePath);
    private native int nativeDelete(long fptConnection, String namePath);
    private native int nativeRemoveDir(long fptConnection, String namePath);
    private native int nativeGet(long fptConnection, String namePath, String dstFile, ARUtilsFtpProgressListener progressListener, Object progressArg, int resume);
    private native byte[] nativeGetWithBuffer(long fptConnection, String namePath, ARUtilsFtpProgressListener progressListener, Object progressArg) throws ARUtilsException;
    private native int nativePut(long fptConnection, String namePath, String srcFile, ARUtilsFtpProgressListener progressListener, Object progressArg, int resume);

    /*  Members  */
    private boolean isInit = false;
    private long nativeFtpConnection = 0;

    /* Public Constants */
    public final static String FTP_ANONYMOUS = "anonymous";

    /*  Java Methods */

    /**
     * Creates ARUtils Ftp
     * @return void
     * @throws ARUtilsException if error
     */
    public void createFtpConnection(String server, int port, String username, String password) throws ARUtilsException
    {
        if (isInit == false)
        {
            nativeFtpConnection = nativeNewFtpConnection(server, port, username, password);
        }

        if (0 != nativeFtpConnection)
        {
            isInit = true;
        }
    }

    /**
     * Closes ARUtils Ftp
     * @return void
     */
    public void closeFtpConnection()
    {
    	if (nativeFtpConnection != 0)
        {
            nativeDeleteFtpConnection(nativeFtpConnection);
            nativeFtpConnection = 0;
            isInit = false;
        }
    }

    /**
     * Gets the ARUtils Ftp Connection status Initialized or not
     * @return true if the Ftp Connection is already created else false
     */
    public boolean isInitialized()
    {
        return isInit;
    }

    /**
     * Disconnects the ARUtils Ftp Connection
     * @return ARUTILS_OK if success, else an {@link ARUTILS_ERROR_ENUM} error code
     */
     public ARUTILS_ERROR_ENUM disconnect(long fptConnection)
    {
        int result = nativeDisconnect(nativeFtpConnection);

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        return error;
    }

    /**
     * Reconnects the ARUtils Ftp Connection
     * @return ARUTILS_OK if success, else an {@link ARUTILS_ERROR_ENUM} error code
     */
    public ARUTILS_ERROR_ENUM reconnect(long fptConnection)
    {
        int result = nativeReconnect(nativeFtpConnection);

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        return error;
    }

    /**
     * Cancels the ARUtils Ftp Connection
     * @return ARUTILS_OK if success, else an {@link ARUTILS_ERROR_ENUM} error code
     */
    public ARUTILS_ERROR_ENUM cancel()
    {
        int result = nativeCancel(nativeFtpConnection);

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        return error;
    }

    /**
     * Get Canceled status of the ARUtils Ftp Connection
     * @return ARUTILS_OK if success, else an {@link ARUTILS_ERROR_ENUM} error code, ARUTILS_ERROR_FTP_CANCELED if canceled
     */
    public ARUTILS_ERROR_ENUM isCanceled()
    {
        int result = nativeIsCanceled(nativeFtpConnection);
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        return error;
    }

    /**
     * Resets the ARUtils Ftp Connection
     * @return ARUTILS_OK if success, else an {@link ARUTILS_ERROR_ENUM} error code
     */
    public ARUTILS_ERROR_ENUM reset()
    {
        int result = nativeReset(nativeFtpConnection);

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        return error;
    }

    /**
     * Gets file list of a directory on the Ftp server
     * @param namePath The file path name
     * @return the directory files list
     * @throws ARUtilsException if error
     */
    public String list(String namePath) throws ARUtilsException
    {
        return nativeList(nativeFtpConnection, namePath);
    }

    /**
     * Renames file on the Ftp server
     * @param oldNamePath The old file path name
     * @param newNamePath The new file path name
     * @return void
     * @throws ARUtilsException if error
     */
    public void rename(String oldNamePath, String newNamePath) throws ARUtilsException
    {
        int result = nativeRename(nativeFtpConnection, oldNamePath, newNamePath);

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);

        if (error != ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            throw new ARUtilsException(error);
        }
    }

    /**
     * Gets file size on the Ftp server
     * @param namePath The file path name
     * @return the file size
     * @throws ARUtilsException if error
     */
    public double size(String namePath) throws ARUtilsException
    {
        return nativeSize(nativeFtpConnection, namePath);
    }

    /**
     * Deletes file on the Ftp server
     * @param namePath The file path name
     * @return void
     * @throws ARUtilsException if error
     */
    public void delete(String namePath) throws ARUtilsException
    {
        int result = nativeDelete(nativeFtpConnection, namePath);

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);

        if (error != ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            throw new ARUtilsException(error);
        }
    }
    
    /**
     * Removes directory on the Ftp server
     * @param namePath The directory path name
     * @return void
     * @throws ARUtilsException if error
     */
    public void removeDir(String namePath) throws ARUtilsException
    {
        int result = nativeRemoveDir(nativeFtpConnection, namePath);
        
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);

        if (error != ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            throw new ARUtilsException(error);
        }
    }

    /**
     * Gets file on the Ftp server
     * @param namePath The server file path name
     * @return dstFile The destination local file path name
     * @return progressListener The progress listener
     * @return progressArg The progress arg
     * @return resume The resume mode requested
     * @return void
     * @throws ARUtilsException if error
     */
    public void get(String namePath, String dstFile, ARUtilsFtpProgressListener progressListener, Object progressArg, ARUTILS_FTP_RESUME_ENUM resume) throws ARUtilsException
    {
        int result = nativeGet(nativeFtpConnection, namePath, dstFile, progressListener, progressArg, resume.getValue());

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);

        if (error != ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            throw new ARUtilsException(error);
        }
    }

    /**
     * Gets file on the Ftp server
     * @param namePath The server file path name
     * @return progressListener The progress listener
     * @return progressArg The progress arg
     * @return resume The resume mode requested
     * @return the bytes array of the file data
     * @throws ARUtilsException if error
     */
    public byte[] getWithBuffer(String namePath, ARUtilsFtpProgressListener progressListener, Object progressArg) throws ARUtilsException
    {
        return nativeGetWithBuffer(nativeFtpConnection, namePath, progressListener, progressArg);
    }

    /**
     * Puts file on the Ftp server
     * @param namePath The server file path name
     * @return dstFile The destination local file path name
     * @return progressListener The progress listener
     * @return progressArg The progress arg
     * @return resume The resume mode requested
     * @return void
     * @throws ARUtilsException if error
     */
    public void put(String namePath, String srcFile, ARUtilsFtpProgressListener progressListener, Object progressArg, ARUTILS_FTP_RESUME_ENUM resume) throws ARUtilsException
    {
        int result = nativePut(nativeFtpConnection, namePath, srcFile, progressListener, progressArg, resume.getValue());

        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);

        if (error != ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            throw new ARUtilsException(error);
        }
    }

    /*  Static Block */
    static
    {
        nativeStaticInit();
    }
}

