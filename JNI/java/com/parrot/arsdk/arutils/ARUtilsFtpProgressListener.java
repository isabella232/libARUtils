
package com.parrot.arsdk.arutils;

/**
 * ARUtils Ftp ProgressListener
 * @author david.flattin.ext@parrot.com
 * @date 30/12/2013
 */
public interface ARUtilsFtpProgressListener
{
    /**
     * Gives the ARUtilsFtp Ftp download progress state
     * @param arg Object progress Listener arg
     * @param percent The percent size of the file already downloaded
     * @return void
     */
     void didFtpProgress(Object arg, int percent);
}
