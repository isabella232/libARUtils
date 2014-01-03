
package com.parrot.arsdk.arutils;

/**
 * Exception class: ARUtilsException of ARUtils library
 * @date 30/12/2013
 * @author david.flattin.ext@parrot.com
 */
public class ARUtilsException extends Exception
{	
    private ARUTILS_ERROR_ENUM error;
    
    /**
     * ARUtilsException constructor
     * @return void
     */
    public ARUtilsException()
    {
        error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR;
    }
    
    /**
     * ARUtilsException constructor
     * @param error ARUTILS_ERROR_ENUM error code
     * @return void
     */
    public ARUtilsException(ARUTILS_ERROR_ENUM error) 
    {
        this.error = error;
    }
    
    /**
     * ARUtilsException constructor
     * @param error int error code
     * @return void
     */
    public ARUtilsException(int error) 
    {
        this.error = ARUTILS_ERROR_ENUM.getFromValue(error);
    }
    
    /**
     * Gets ARUtils ERROR code
     * @return {@link ARUTILS_ERROR_ENUM} error code
     */
    public ARUTILS_ERROR_ENUM getError()
    {
        return error;
    }
    
    /**
     * Sets ARUtils ERROR code
     * @param error {@link ARUTILS_ERROR_ENUM} error code     
     * @return void
     */
    public void setError(ARUTILS_ERROR_ENUM error)
    {
        this.error = error;
    }
    
    /**
     * Gets ARUtilsException string representation
     * @return String Exception representation
     */
    public String toString ()
    {
        String str;
        
        if (null != error)
        {
            str = "ARUtilsException [" + error.toString() + "]";
        }
        else
        {
            str = super.toString();
        }
        
        return str;
    }
}
