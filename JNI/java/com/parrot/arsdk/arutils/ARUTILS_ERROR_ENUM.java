/*
 * GENERATED FILE
 *  Do not modify this file, it will be erased during the next configure run
 */

package com.parrot.arsdk.arutils;

import java.util.HashMap;

/**
 * Java copy of the eARUTILS_ERROR enum
 */
public enum ARUTILS_ERROR_ENUM {
   /** No error */
    ARUTILS_OK (0, "No error"),
   /** Unknown generic error */
    ARUTILS_ERROR (-1000, "Unknown generic error"),
   /** Memory allocation error */
    ARUTILS_ERROR_ALLOC (-999, "Memory allocation error"),
   /** Bad parameters error */
    ARUTILS_ERROR_BAD_PARAMETER (-998, "Bad parameters error"),
   /** System error */
    ARUTILS_ERROR_SYSTEM (-997, "System error"),
   /** curl allocation error */
    ARUTILS_ERROR_CURL_ALLOC (-2000, "curl allocation error"),
   /** curl set option error */
    ARUTILS_ERROR_CURL_SETOPT (-1999, "curl set option error"),
   /** curl get info error */
    ARUTILS_ERROR_CURL_GETINFO (-1998, "curl get info error"),
   /** curl perform error */
    ARUTILS_ERROR_CURL_PERFORM (-1997, "curl perform error"),
   /** file not found error */
    ARUTILS_ERROR_FILE_NOT_FOUND (-1996, "file not found error"),
   /** ftp connect error */
    ARUTILS_ERROR_FTP_CONNECT (-1995, "ftp connect error"),
   /** ftp code error */
    ARUTILS_ERROR_FTP_CODE (-1994, "ftp code error"),
   /** ftp file size error */
    ARUTILS_ERROR_FTP_SIZE (-1993, "ftp file size error"),
   /** ftp resume error */
    ARUTILS_ERROR_FTP_RESUME (-1992, "ftp resume error"),
   /** ftp user canceled error */
    ARUTILS_ERROR_FTP_CANCELED (-1991, "ftp user canceled error"),
   /** http connect error */
    ARUTILS_ERROR_HTTP_CONNECT (-1990, "http connect error"),
   /** http code error */
    ARUTILS_ERROR_HTTP_CODE (-1989, "http code error"),
   /** http file size error */
    ARUTILS_ERROR_HTTP_SIZE (-1988, "http file size error"),
   /** http resume error */
    ARUTILS_ERROR_HTTP_RESUME (-1987, "http resume error"),
   /** http user canceled error */
    ARUTILS_ERROR_HTTP_CANCELED (-1986, "http user canceled error");

    private final int value;
    private final String comment;
    static HashMap<Integer, ARUTILS_ERROR_ENUM> valuesList;

    ARUTILS_ERROR_ENUM (int value) {
        this.value = value;
        this.comment = null;
    }

    ARUTILS_ERROR_ENUM (int value, String comment) {
        this.value = value;
        this.comment = comment;
    }

    /**
     * Gets the int value of the enum
     * @return int value of the enum
     */
    public int getValue () {
        return value;
    }

    /**
     * Gets the ARUTILS_ERROR_ENUM instance from a C enum value
     * @param value C value of the enum
     * @return The ARUTILS_ERROR_ENUM instance, or null if the C enum value was not valid
     */
    public static ARUTILS_ERROR_ENUM getFromValue (int value) {
        if (null == valuesList) {
            ARUTILS_ERROR_ENUM [] valuesArray = ARUTILS_ERROR_ENUM.values ();
            valuesList = new HashMap<Integer, ARUTILS_ERROR_ENUM> (valuesArray.length);
            for (ARUTILS_ERROR_ENUM entry : valuesArray) {
                valuesList.put (entry.getValue (), entry);
            }
        }
        return valuesList.get (value);
    }

    /**
     * Returns the enum comment as a description string
     * @return The enum description
     */
    public String toString () {
        if (this.comment != null) {
            return this.comment;
        }
        return super.toString ();
    }
}
