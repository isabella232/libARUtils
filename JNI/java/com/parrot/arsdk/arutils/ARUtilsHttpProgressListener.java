
package com.parrot.arsdk.arutils;

/**
 * ARUtils Http ProgressListener
 * @author david.flattin.ext@parrot.com
 * @date 30/12/2013
 */
public interface ARUtilsHttpProgressListener
{
    /**
     * Gives the ARUtils Http download progress state
     * @param arg Object progress Listener arg
     * @param percent The percent size of the file already downloaded
     * @return void
     */
     void didHttpProgress(Object arg, int percent);
}
