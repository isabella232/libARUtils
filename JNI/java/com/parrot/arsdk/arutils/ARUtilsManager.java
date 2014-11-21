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

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.pm.PackageManager;
import com.parrot.arsdk.arsal.ARSALBLEManager;
import com.parrot.arsdk.arsal.ARSAL_ERROR_ENUM;
import java.util.concurrent.Semaphore;

/**
 * ARUtils Manager module
 */
public class ARUtilsManager
{
    /* Native Functions */
    private native static boolean nativeStaticInit();
    private native long nativeNew() throws ARUtilsException;
    private native int nativeDelete(long jManager);
    private native int nativeInitWifiFtp(long jManager, String jserver, int port, String jusername, String jpassword);
    private native int nativeCloseWifiFtp(long jManager);
    private native int nativeInitBLEFtp(long jManager, ARUtilsBLEFtp bleFtp, Semaphore cancelSem);
    private native void nativeCloseBLEFtp(long jManager);

    /*Test Methods*/
    private native int nativeBLEFtpConnectionCancel(long jManager);
    private native int nativeBLEFtpIsConnectionCanceled(long jManager);    
    private native int nativeBLEFtpConnectionReset(long jManager);
    private native String nativeBLEFtpList(long jManager, String remotePath);
    private native byte[] nativeBLEFtpGetWithBuffer(long jManager, String remotePath, ARUtilsFtpProgressListener progressListener, Object progressArg);
    private native int nativeBLEFtpGet(long jManager, String remotePath, String destFile, ARUtilsFtpProgressListener progressListener, Object progressArg, boolean resume);
    private native int nativeBLEFtpPut(long jManager, String remotePath, String srcFile, ARUtilsFtpProgressListener progressListener, Object progressArg, boolean resume);
    private native int nativeBLEFtpDelete(long jManager, String remotePath);    
    private native int nativeBLEFtpRename(long jManager, String oldNamePath, String newNamePath);

    private long m_managerPtr;
    private boolean m_initOk;

    static
    {
        nativeStaticInit();
    }
    
    /*  Java Methods */
    
    /**
     * Constructor
     */
    public ARUtilsManager() throws ARUtilsException
    {
        m_initOk = false;
        m_managerPtr = nativeNew();

        if( m_managerPtr != 0 )
        {
            m_initOk = true;
        }
    }

    /**
     * Dispose
     */
    public void dispose()
    {
        if(m_initOk == true)
        {
            nativeDelete(m_managerPtr);
            m_managerPtr = 0;
            m_initOk = false;
        }
    }


    /**
     * Destructor
     */
    public void finalize () throws Throwable
    {
        try
        {
            dispose ();
        }
        finally
        {
            super.finalize ();
        }
    }

    /**
     * Get the pointer C on the network manager
     * @return  Pointer C on the network manager
     */
    public long getManager ()
    {
        return m_managerPtr;
    }

    /**
     * Get is the Manager is correctly initialized and if it is usable
     * @return true is the Manager is usable
     */
    public boolean isCorrectlyInitialized ()
    {
        return m_initOk;
    }

    /**
     * Initialize Wifi network to send and receive data
     */
    public ARUTILS_ERROR_ENUM initWifiFtp(String addr, int port, String username, String password)
    {
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR;
        
        if(addr != null)
        {
            int intError = nativeInitWifiFtp(m_managerPtr, addr, port, username, password);
            error =  ARUTILS_ERROR_ENUM.getFromValue(intError);
        }

        return error;
    }

    /**
     * Closes Wifi network
     */
    public ARUTILS_ERROR_ENUM closeWifiFtp()
    {
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR;
        int intError = nativeCloseWifiFtp(m_managerPtr);
        error = ARUTILS_ERROR_ENUM.getFromValue(intError);
        return error;
    }
    
    /**
     * Initialize BLE network to send and receive data
     */
    public ARUTILS_ERROR_ENUM initBLEFtp(Context context, BluetoothGatt deviceGatt, int port)
    {
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.ARUTILS_OK;
        
        /* check parameters */
        if (context == null)
        {
            error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR_BAD_PARAMETER;
        }

        if (deviceGatt == null)
        {
            error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR_BAD_PARAMETER;
        }

        if ((port == 0) || ((port % 10) != 1))
        {
            error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR_BAD_PARAMETER;
        }
        
        if (error == ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            /* check if the BLE is available*/
            if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE) != true)
            {
                error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR_NETWORK_TYPE;
            }
        }
        if (error == ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            // No need to connect because we will always be connected before reaching this step (in FreeFlight!)
            //if (bleManager.connect(device) != ARSAL_ERROR_ENUM.ARSAL_OK) {
            //    error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR_BLE_FAILED;
            //}
        }
        if (error == ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            ARUtilsBLEFtp bleFtp = ARUtilsBLEFtp.getInstance(context);
            bleFtp.registerDevice(deviceGatt, port);
            bleFtp.registerCharacteristics();
            Semaphore cancelSem = new Semaphore(0);
            nativeInitBLEFtp(m_managerPtr, bleFtp, cancelSem);
        }
        
        return error;
    }
    
    /**
     * cancel BLE network connection
     */
    /*public ARUTILS_ERROR_ENUM cancelBLEFtp()
    {
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.ARUTILS_OK;
        
        if(m_initOk == true)
        {
            error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR;
        }
        
        if (error == ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            //int intError = nativeCancelBLEFtp(m_managerPtr);
            //error = ARUTILS_ERROR_ENUM.getFromValue(intError);
        }
        
        return error;
    }*/
    
    /**
     * Closes BLE network
     */
    public ARUTILS_ERROR_ENUM closeBLEFtp(Context context)
    {
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.ARUTILS_OK;
        
        /* check parameters */
        if (context == null)
        {
            error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR_BAD_PARAMETER;
        }
        
        if (error == ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            /* check if the BLE is available*/
            if (context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE) != true)
            {
                error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR_NETWORK_TYPE;
            }
        }
        
        if (error == ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            ARUtilsBLEFtp bleFtp = ARUtilsBLEFtp.getInstance(context);
            bleFtp.unregisterCharacteristics();
            bleFtp.unregisterDevice();
            nativeCloseBLEFtp(m_managerPtr);
        }
        return error;
    }
    
    public ARUTILS_ERROR_ENUM BLEFtpConnectionCancel()
    {
        return ARUTILS_ERROR_ENUM.getFromValue(nativeBLEFtpConnectionCancel(m_managerPtr));
    }
    
    public ARUTILS_ERROR_ENUM BLEFtpIsConnectionCanceled()
    {
        return ARUTILS_ERROR_ENUM.getFromValue(nativeBLEFtpIsConnectionCanceled(m_managerPtr));
    }

    public ARUTILS_ERROR_ENUM BLEFtpConnectionReset()
    {
        return ARUTILS_ERROR_ENUM.getFromValue(nativeBLEFtpConnectionReset(m_managerPtr));
    }

    public String BLEFtpListFile(String remotePath)
    {
        return nativeBLEFtpList(m_managerPtr, remotePath);
    }

    public ARUTILS_ERROR_ENUM BLEFtpPut(String remotePath, String srcFile, ARUtilsFtpProgressListener progressListener, Object progressArg, boolean resume)
    {
        return ARUTILS_ERROR_ENUM.getFromValue(nativeBLEFtpPut(m_managerPtr, remotePath, srcFile, progressListener, progressArg, resume));
    }

    public ARUTILS_ERROR_ENUM BLEFtpGet(String remotePath, String destFile, ARUtilsFtpProgressListener progressListener, Object progressArg, boolean resume)
    {
        return ARUTILS_ERROR_ENUM.getFromValue(nativeBLEFtpGet(m_managerPtr, remotePath, destFile, progressListener, progressArg, resume));
    }

    public byte[] BLEFtpGetWithBuffer(String remotePath, ARUtilsFtpProgressListener progressListener, Object progressArg)
    {
        return nativeBLEFtpGetWithBuffer(m_managerPtr, remotePath, progressListener, progressArg);
    }

    public ARUTILS_ERROR_ENUM BLEFtpDelete(String remotePath)
    {
        return ARUTILS_ERROR_ENUM.getFromValue(nativeBLEFtpDelete(m_managerPtr, remotePath));
    }    

    public ARUTILS_ERROR_ENUM BLEFtpRename(String oldNamePath, String newNamePath)
    {
        return ARUTILS_ERROR_ENUM.getFromValue(nativeBLEFtpRename(m_managerPtr, oldNamePath, newNamePath));
    }
    
}

