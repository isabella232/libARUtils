
package com.parrot.arsdk.arutils;

/**
 * ARUtils FileSystem module
 * @author david.flattin.ext@parrot.com
 * @date 30/12/2013
 */
public class ARUtilsFileSystem
{
    /* Native Functions */
    private native static boolean nativeStaticInit();
    private native long nativeGetFileSize(String namePath) throws ARUtilsException;
    private native int nativeRename(String oldName, String newName);
    private native int nativeRemoveFile(String localPath);
    private native int nativeRemoveDir(String localPath);
    private native double nativeGetFreeSpace(String localPath) throws ARUtilsException;
    
    /*  Java Methods */
    
    /**
     * Gets file size
     * @param namePath The file path name
     * @return the file size
     * @throws ARUtilsException if error
     */
    public long getFileSize(String namePath) throws ARUtilsException
    {
        return nativeGetFileSize(namePath);
    }
    
    /**
     * Renames file
     * @param oldName The old file path name
     * @param newName The new file path name
     * @return void
     * @throws ARUtilsException if error
     */
    public void rename(String oldName, String newName)  throws ARUtilsException
    {
        int result = nativeRename(oldName, newName);
        
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        
        if (error != ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            throw new ARUtilsException(error);
        }
    }
    
    /**
     * Remove file
     * @param localPath The local file path name
     * @return void
     * @throws ARUtilsException if error
     */
    public void removeFile(String localPath) throws ARUtilsException
    {
        int result = nativeRemoveFile(localPath);
        
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        
        if (error != ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            throw new ARUtilsException(error);
        }
    }
    
    /**
     * Removes directory and its content recursively
     * @param localPath The local file path name
     * @return void
     * @throws ARUtilsException if error
     */
    public void removeDir(String localPath) throws ARUtilsException
    {
        int result = nativeRemoveDir(localPath);
        
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.getFromValue(result);
        
        if (error != ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            throw new ARUtilsException(error);
        }
    }
    
    /**
     * Gets file memory free space available
     * @param localPath The local file path name
     * @return void
     * @throws ARUtilsException if error
     */
    public double getFreeSpace(String localPath) throws ARUtilsException
    {
        return nativeGetFreeSpace(localPath);
    }
    
    /*  Static Block */
    static
    {
        nativeStaticInit();
    }
}

