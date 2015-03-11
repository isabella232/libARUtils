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
package com.parrot.testbleftp;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.os.IBinder;
import android.os.Looper;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import com.parrot.arsdk.ardiscovery.ARDiscoveryDeviceBLEService;
import com.parrot.arsdk.ardiscovery.ARDiscoveryDeviceService;
import com.parrot.arsdk.ardiscovery.ARDiscoveryService;
import com.parrot.arsdk.ardiscovery.receivers.ARDiscoveryServicesDevicesListUpdatedReceiver;
import com.parrot.arsdk.ardiscovery.receivers.ARDiscoveryServicesDevicesListUpdatedReceiverDelegate;
import com.parrot.arsdk.arsal.ARSALBLEManager;
import com.parrot.arsdk.arsal.ARSAL_ERROR_ENUM;
import com.parrot.arsdk.arsal.ARUUID;

public class LoopConnectActivity extends Activity implements ARDiscoveryServicesDevicesListUpdatedReceiverDelegate
{
    private String name = "RS_R000387";
    
    private static final String TAG = LoopConnectActivity.class.getSimpleName();
 
    private ARDiscoveryService ardiscoveryService;
    private boolean ardiscoveryServiceBound = false;
    private BroadcastReceiver ardiscoveryServicesDevicesListUpdatedReceiver;
    private ServiceConnection ardiscoveryServiceConnection;
    public IBinder discoveryServiceBinder;
    BluetoothDevice mLoopDevice = null;

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);
        System.loadLibrary("arsal");
        System.loadLibrary("arsal_android");
        System.loadLibrary("ardiscovery");
        System.loadLibrary("ardiscovery_android");
        System.loadLibrary("arutils");
        System.loadLibrary("arutils_android");
        initServiceConnection();
        initServices();
        initBroadcastReceiver();
        registerReceivers();
    }

    private void initServiceConnection()
    {
        ardiscoveryServiceConnection = new ServiceConnection()
        {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service)
            {
                discoveryServiceBinder = service;
                ardiscoveryService = ((ARDiscoveryService.LocalBinder) service).getService();
                ardiscoveryServiceBound = true;

            }

            @Override
            public void onServiceDisconnected(ComponentName name)
            {
                ardiscoveryService = null;
                ardiscoveryServiceBound = false;
            }
        };
    }

    private void initServices()
    {
        if (discoveryServiceBinder == null)
        {
            Intent i = new Intent(getApplicationContext(), ARDiscoveryService.class);
            getApplicationContext().bindService(i, ardiscoveryServiceConnection, Context.BIND_AUTO_CREATE);
        }
        else
        {
            ardiscoveryService = ((ARDiscoveryService.LocalBinder) discoveryServiceBinder).getService();
            //ardiscoveryServiceBound = true;
        }
    }

    private void initBroadcastReceiver()
    {
        ardiscoveryServicesDevicesListUpdatedReceiver = new ARDiscoveryServicesDevicesListUpdatedReceiver(this);
    }
    
    private void registerReceivers()
    {
        LocalBroadcastManager localBroadcastMgr = LocalBroadcastManager.getInstance(getApplicationContext());
        localBroadcastMgr.registerReceiver(ardiscoveryServicesDevicesListUpdatedReceiver, new IntentFilter(ARDiscoveryService.kARDiscoveryServiceNotificationServicesDevicesListUpdated));
    }

    @Override
    public void onServicesDevicesListUpdated()
    {
        if (Looper.getMainLooper().getThread() == Thread.currentThread())
        {
            // On UI thread.
            Log.d(TAG, "WE ARE ON UI THREAD");
        }
        else
        {
            // Not on UI thread.
            Log.d(TAG, "WE ARE NOT ON UI THREAD :-(");
        }
        List<ARDiscoveryDeviceService> list = ardiscoveryService.getDeviceServicesArray();
        Iterator<ARDiscoveryDeviceService> iterator = list.iterator();
        while (iterator.hasNext())
        {
            ARDiscoveryDeviceService service = iterator.next();
            if (service.getName().equals(name))
            {
                Log.d(TAG, "Setting new bluetooth device");
                ARDiscoveryDeviceBLEService serviceBle = (ARDiscoveryDeviceBLEService)service.getDevice();
                //mThread.setBluetoothDevice(serviceBle.getBluetoothDevice());
                if (mLoopDevice == null)
                {
                    mLoopDevice = serviceBle.getBluetoothDevice();
                    testConnect(mLoopDevice);
                    mLoopDevice = null;
                }
            }
        }
    }

    private void testConnect(BluetoothDevice device)
    {
        ARSALBLEManager bleManager = ARSALBLEManager.getInstance(getApplicationContext());
        Log.d(TAG, "Connecting...");
        if (ARSAL_ERROR_ENUM.ARSAL_OK == bleManager.connect(device))
        {
            try
            {
                Thread.sleep(1000);
            }
            catch (InterruptedException e)
            {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            testServices(bleManager);
            Log.d(TAG, "Connection ok, disconnecting...");
            bleManager.disconnect();
        }
        else
        {
            Log.e(TAG, "Connection nok !");
        }

        

    }

    private void testServices(ARSALBLEManager bleManager)
    {
        Log.d(TAG, "Will discover services...");
        ARSAL_ERROR_ENUM resultSat = bleManager.discoverBLENetworkServices();
        Log.d(TAG, "Did discover services " + resultSat);

        BluetoothGatt gattDevice = bleManager.getGatt();

        List<BluetoothGattService> serviceList = gattDevice.getServices();
        Log.d(TAG, "Services count " + serviceList.size());

        // 07-03 10:11:49.494: D/DBG(5070): BLEFtp service
        // 9a66fd21-0800-9191-11e4-012d1540cb8e

        Iterator<BluetoothGattService> iterator = serviceList.iterator();
        while (iterator.hasNext())
        {
            BluetoothGattService service = iterator.next();

            String name = ARUUID.getShortUuid(service.getUuid());
            // String name = service.getUuid().toString();
            Log.d(TAG, "Service " + name);

            String serviceUuid = ARUUID.getShortUuid(service.getUuid());
            // String serviceUuid = service.getUuid().toString();

            if (serviceUuid.startsWith("fd21") || serviceUuid.startsWith("fd51"))
            {
                List<BluetoothGattCharacteristic> characteristics = service.getCharacteristics();
                Iterator<BluetoothGattCharacteristic> it = characteristics.iterator();

                while (it.hasNext())
                {
                    BluetoothGattCharacteristic characteristic = it.next();
                    String characteristicUuid = ARUUID.getShortUuid(characteristic.getUuid());
                    // String characteristicUuid =
                    // characteristic.getUuid().toString();
                    Log.d(TAG, "Characteristic " + characteristicUuid);

                    /*
                    if (characteristicUuid.startsWith("fd23") || characteristicUuid.startsWith("fd53"))
                    {
                        error = bleManager.setCharacteristicNotification(service, characteristic);
                        if (error != ARSAL_ERROR_ENUM.ARSAL_OK)
                        {
                            Log.d(TAG, "Set " + error.toString());
                        }
                        Log.d(TAG, "Set " + error.toString());
                    }*/
                }
            }
        }
    }

}
