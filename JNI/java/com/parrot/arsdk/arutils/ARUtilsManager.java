
package com.parrot.arsdk.arutils;

import android.bluetooth.BluetoothDevice;
import android.content.Context;
import android.content.pm.PackageManager;

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
    public ARUTILS_ERROR_ENUM closeWifiNetwork()
    {
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR;
        int intError = nativeCloseWifiFtp(m_managerPtr);
        error = ARUTILS_ERROR_ENUM.getFromValue(intError);
        return error;
    }
    
    /**
     * Initialize BLE network to send and receive data
     */
    public ARUTILS_ERROR_ENUM initBLEFtp(Context context, BluetoothDevice device)
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
            /*ARNetworkALBLEManager bleManager = new ARNetworkALBLEManager(context.getApplicationContext());
            ARSALBLEManager bleManager = ARSALBLEManager.getInstance(context.getApplicationContext());

            ARUtilsBLEFtp 

            */

            //int intError = nativeInitBLEFtp(m_managerPtr, device);
            error =  ARUTILS_ERROR_ENUM.ARUTILS_OK;
        }
        
        return error;
    }
    
    /**
     * cancel BLE network connection
     */
    public ARUTILS_ERROR_ENUM cancelBLEFtp()
    {
        ARUTILS_ERROR_ENUM error = ARUTILS_ERROR_ENUM.ARUTILS_OK;
        
        if(m_initOk == true)
        {
            error = ARUTILS_ERROR_ENUM.ARUTILS_ERROR;
        }
        
        if (error == ARUTILS_ERROR_ENUM.ARUTILS_OK)
        {
            /* close the ARNetworkALBLEManager */
            //int intError = nativeCancelBLEFtp(m_managerPtr);
            //error = ARUTILS_ERROR_ENUM.getFromValue(intError);
        }
        
        return error;
    }
    
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
            /* close the ARNetworkALBLEManager */
            //int intError = nativeCloseBLEFtp(m_managerPtr);
            //error = ARUTILS_ERROR_ENUM.getFromValue(intError);
        }
        
        return error;
    }
    
}

