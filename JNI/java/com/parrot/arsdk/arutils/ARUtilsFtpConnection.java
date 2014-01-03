
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
    private native int nativeCancel(long fptConnection);
    private native String nativeList(long fptConnection, String namePath) throws ARUtilsException;
    private native int nativeRename(long fptConnection, String oldNamePath, String newNamePath);
    private native double nativeSize(long fptConnection, String namePath);
    private native int nativeDelete(long fptConnection, String namePath);
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

