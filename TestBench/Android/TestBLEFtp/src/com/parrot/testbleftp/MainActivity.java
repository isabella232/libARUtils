package com.parrot.testbleftp;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.concurrent.Semaphore;

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
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.Window;
import android.widget.Button;

import com.parrot.arsdk.ardiscovery.ARDISCOVERY_PRODUCT_ENUM;
import com.parrot.arsdk.ardiscovery.ARDiscoveryDeviceBLEService;
import com.parrot.arsdk.ardiscovery.ARDiscoveryDeviceService;
import com.parrot.arsdk.ardiscovery.ARDiscoveryService;
import com.parrot.arsdk.ardiscovery.receivers.ARDiscoveryServicesDevicesListUpdatedReceiver;
import com.parrot.arsdk.ardiscovery.receivers.ARDiscoveryServicesDevicesListUpdatedReceiverDelegate;
import com.parrot.arsdk.arsal.ARSALBLEManager;
import com.parrot.arsdk.arsal.ARSALException;
import com.parrot.arsdk.arsal.ARSALMd5;
import com.parrot.arsdk.arsal.ARSALMd5Manager;
import com.parrot.arsdk.arsal.ARSAL_ERROR_ENUM;
import com.parrot.arsdk.arsal.ARUUID;
import com.parrot.arsdk.arutils.ARUtilsBLEFtp;
import com.parrot.arsdk.arutils.ARUtilsException;
import com.parrot.arsdk.arutils.ARUtilsFtpProgressListener;
import com.parrot.arsdk.arutils.ARUtilsManager;

public class MainActivity extends Activity implements ARDiscoveryServicesDevicesListUpdatedReceiverDelegate 
{
	public static String APP_TAG = "BLEFtp ";
	    
	private ARDiscoveryService ardiscoveryService;
    private boolean ardiscoveryServiceBound = false;
    private BroadcastReceiver ardiscoveryServicesDevicesListUpdatedReceiver;
    private ServiceConnection ardiscoveryServiceConnection;
    public IBinder discoveryServiceBinder;
    
    private BluetoothDevice mDevice;
    private BluetoothGatt mDeviceGatt;
    private ARUtilsManager mUtilsManager;

    Button testBleALButton;
    Button testMd5Button;
    Button testListButton;
    Button testGetButton;
    Button testGetWithBufferButton;
    Button testPutButton;
    Button testDeleteButton;
    Button testRenameButton;
    Button currentButton = null;

    private Handler mHandler = new Handler();
	
	@Override
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_INDETERMINATE_PROGRESS);
		setContentView(R.layout.activity_main);
	    
		Log.d("DBG", APP_TAG + "onCreate");
	    
        System.loadLibrary("arsal");
        System.loadLibrary("arsal_android");
        //System.loadLibrary("arnetworkal");
        //System.loadLibrary("arnetworkal_android");
        //System.loadLibrary("arnetwork");
        //System.loadLibrary("arnetwork_android");
        System.loadLibrary("ardiscovery");
        System.loadLibrary("ardiscovery_android");
		
        System.loadLibrary("arutils");
        System.loadLibrary("arutils_android");
	    
	    //startService(new Intent(this, ARDiscoveryService.class));
        //initServiceConnection();
        //initServices();
	        
	    testBleALButton = (Button)this.findViewById(R.id.testBle);
	    testBleALButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                TestBleAL();
            }
        });
	    
	    testMd5Button = (Button)this.findViewById(R.id.testMd5);
	    testMd5Button.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                TestMd5();
            }
        });

        testListButton = (Button)this.findViewById(R.id.testList);
        testListButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                testBleListFile();
            }
        });

        testGetButton = (Button)this.findViewById(R.id.testGet);
        testGetButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                testBleGetFile();
            }
        });

        testGetWithBufferButton = (Button)this.findViewById(R.id.testGetWithBuffer);
        testGetWithBufferButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                testBleGetFileWithBuffer();
            }
        });

        testPutButton = (Button)this.findViewById(R.id.testPut);
        testPutButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                testBlePutFile();
            }
        });


        testDeleteButton = (Button)this.findViewById(R.id.testDelete);
        testDeleteButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                testBleDeleteFile();
                //testBleDeleteFileWJNI();
            }
        });
        
        testRenameButton = (Button)this.findViewById(R.id.testRename);
        testRenameButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                testBleRenameFile();
                //testBleDeleteFileWJNI();
            }
        });
	    
        StartDiscoveryService();
        //TestFile();
        //TestBle();
        //TestMd5();        
	}
	
	@Override
	protected void onPause()
	{
		super.onPause();
	}
	
	@Override
	protected void onStop()
	{
		super.onStop();
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		//getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

    public void enableButtons(boolean enable)
    {
        testBleALButton.setEnabled(enable);
        testMd5Button.setEnabled(enable);
        testListButton.setEnabled(enable);
        testGetButton.setEnabled(enable);
        testGetWithBufferButton.setEnabled(enable);
        testPutButton.setEnabled(enable);
        testDeleteButton.setEnabled(enable);
        testRenameButton.setEnabled(enable);
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
	
	/*private void closeServices()
    {
        if (ardiscoveryServiceBound == true)
        {
            getApplicationContext().unbindService(ardiscoveryServiceConnection);
            ardiscoveryServiceBound = false;
            discoveryServiceBinder = null;
            ardiscoveryService = null;
        }
    }*/
	
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
	
	private void initBroadcastReceiver()
    {
        ardiscoveryServicesDevicesListUpdatedReceiver = new ARDiscoveryServicesDevicesListUpdatedReceiver(this);
    }
	
	private void registerReceivers()
    {
        LocalBroadcastManager localBroadcastMgr = LocalBroadcastManager.getInstance(getApplicationContext());
        localBroadcastMgr.registerReceiver(ardiscoveryServicesDevicesListUpdatedReceiver, new IntentFilter(ARDiscoveryService.kARDiscoveryServiceNotificationServicesDevicesListUpdated));
    }

    private void unregisterReceivers()
    {
        LocalBroadcastManager localBroadcastMgr = LocalBroadcastManager.getInstance(getApplicationContext());
        localBroadcastMgr.unregisterReceiver(ardiscoveryServicesDevicesListUpdatedReceiver);
    }
	
	public void onServicesDevicesListUpdated()
	{
		Log.d("DBG", APP_TAG + "onServicesDevicesListUpdated");
		
		ArrayList<ARDiscoveryDeviceService> list = ardiscoveryService.getDeviceServicesArray();
		Iterator<ARDiscoveryDeviceService> iterator = list.iterator();
		ARDISCOVERY_PRODUCT_ENUM ble = ARDISCOVERY_PRODUCT_ENUM.ARDISCOVERY_PRODUCT_MINIDRONE;
		while (iterator.hasNext())
		{
			ARDiscoveryDeviceService serviceIndex = iterator.next();
			Log.d("DBG", APP_TAG + " " + serviceIndex.getName());
			int productID = serviceIndex.getProductID();
			int value = ble.getValue();
			Log.d("DBG", APP_TAG + " " + productID + ", " + value);
			//if ( == )
			{
				//String name = "RS_R000387";
				String name = "RS_B000272";
			//String name = "RS_W000444";
			//String name = "RS_B000497";
			//String name = "RS_B000443";
				//String name = "Maurice";
				//07-02 17:49:58.933: D/DBG(8280): TestBLEFtp  Flower power 3337
				//String name = "Flower power 2FB7";

				if (serviceIndex.getName().contentEquals(name))
				{
					//ardiscoveryService.stop();
					
					ARDiscoveryDeviceBLEService serviceBle = (ARDiscoveryDeviceBLEService)serviceIndex.getDevice();
					
					if (mDevice == null)
					{
						mDevice = serviceBle.getBluetoothDevice();
                        mDeviceGatt = connectToBleDevice(mDevice);
                        
                        //getARUtilsManager();
                        
                        TestBleAL();
                        
                        enableButtons(true);
                        setProgressBarIndeterminateVisibility(false);
					}
				}
			}
		}
		
		Log.d("DBG", APP_TAG + "onServicesDevicesListUpdated exiting");
	}
	
	public void StartDiscoveryService()
	{
		Log.d("DBG", APP_TAG + "TestBle");
        setProgressBarIndeterminateVisibility(true);
        initServiceConnection();
        initServices();
        
        initBroadcastReceiver();
        registerReceivers();
	}
	
	private BluetoothGatt connectToBleDevice(BluetoothDevice device)
	{
		//ARSALBLEManager bleManager = new ARSALBLEManager(getApplicationContext());
	    ARSALBLEManager bleManager = ARSALBLEManager.getInstance(getApplicationContext());
        Log.d("DBG", APP_TAG + "will connect");
        bleManager.connect(device);
        Log.d("DBG", APP_TAG + "did connect");
        
        Log.d("DBG", APP_TAG + "will discover services");
        ARSAL_ERROR_ENUM resultSat = bleManager.discoverBLENetworkServices();
        Log.d("DBG", APP_TAG + "did discover services " + resultSat);
        
        BluetoothGatt gattDevice = bleManager.getGatt();
        
        List<BluetoothGattService> serviceList = gattDevice.getServices();
        Log.d("DBG", APP_TAG + "services count " + serviceList.size());
        ARSAL_ERROR_ENUM error = ARSAL_ERROR_ENUM.ARSAL_OK;
        
        //07-03 10:11:49.494: D/DBG(5070): BLEFtp service 9a66fd21-0800-9191-11e4-012d1540cb8e

        Iterator<BluetoothGattService> iterator = serviceList.iterator();
        while (iterator.hasNext())
        {
            BluetoothGattService service = iterator.next();
            
            String name = ARUUID.getShortUuid(service.getUuid());
            //String name = service.getUuid().toString();
            Log.d("DBG", APP_TAG + "service " + name);
            
            String serviceUuid = ARUUID.getShortUuid(service.getUuid());
            //String serviceUuid = service.getUuid().toString();
            
            if (serviceUuid.startsWith(/*"0000"+*/"fd21") 
                || serviceUuid.startsWith(/*"0000"+*/"fd51"))
            {
                List<BluetoothGattCharacteristic> characteristics = service.getCharacteristics();
                Iterator<BluetoothGattCharacteristic> it = characteristics.iterator();
                
                while (it.hasNext())
                {
                    BluetoothGattCharacteristic characteristic = it.next();
                    String characteristicUuid = ARUUID.getShortUuid(characteristic.getUuid());
                    //String characteristicUuid = characteristic.getUuid().toString();
                    Log.d("DBG", APP_TAG + "characteristic " + characteristicUuid);
                    
                    if (characteristicUuid.startsWith(/*"0000"+*/"fd23")
                        || characteristicUuid.startsWith(/*"0000"+*/"fd53"))
                    {
                        error = bleManager.setCharacteristicNotification(service, characteristic);
                        if (error != ARSAL_ERROR_ENUM.ARSAL_OK)
                        {
                            Log.d("DBG", APP_TAG + "set " + error.toString());
                        }
                        Log.d("DBG", APP_TAG + "set " + error.toString());
                    }
                }
            }
        }
        return gattDevice;
	}
	
	/*private void startTestBleAL(BluetoothDevice device)
	{
		Log.d("DBG", APP_TAG + "startTestBle");
        ARSALBLEManager bleManager = ARSALBLEManager.getInstance(getApplicationContext());
		BluetoothGatt gattDevice = connectToBleDevice(device);
		boolean ret = true;

        if (gattDevice != null)
        {
            ARUtilsBLEFtp bleFtp = new ARUtilsBLEFtp();

            //bleFtp.initWithDevice(bleManager, gattDevice, 21);
            bleFtp.initWithDevice(bleManager, gattDevice, 51);

            ret = bleFtp.registerCharacteristics();
         }

		bleManager.disconnect();
	}
	
	/*private String getShortUuid(UUID uuid)
	{
		String shortUuid = uuid.toString().substring(4, 8);		
		return shortUuid;
	}*/
	
	public void TestBleAL()
	{
	    if (mDeviceGatt != null)
	    {
            boolean ret = true;
            String[] list = new String[1];
            double[] totalSize = new double[1];
            Semaphore cancelSem = new Semaphore(0);
            ARSALBLEManager bleManager = ARSALBLEManager.getInstance(getApplicationContext());
            //ARUtilsBLEFtp bleFtp = new ARUtilsBLEFtp();
            ARUtilsBLEFtp bleFtp = ARUtilsBLEFtp.getInstance(getApplicationContext());
            
            File sysHome = this.getFilesDir();// /data/data/com.example.tstdata/files
            String tmp = sysHome.getAbsolutePath();
            String filePath = null;
            
            
            //bleFtp.initWithDevice(bleManager, mDeviceGatt, 21);
            //bleFtp.initWithDevice(bleManager, mDeviceGatt, 51);
            bleFtp.initWithBLEManager(bleManager);

            ret = bleFtp.registerDevice( mDeviceGatt, 51);
            ret = bleFtp.registerCharacteristics();


	        ret = bleFtp.listFilesAL("/", list);
            //ret = bleFtp.listFilesAL("/internal_000/Rolling_Spider/media", list);
            //ret = bleFtp.listFilesAL("/internal_000/Rolling_Spider/thumb", list);

            Log.d("DBG", APP_TAG + "LIST: " + list[0]);


            //Rolling_Spider_2014-07-01T161736+0000_33539649FC464D41FAD7FB4F0F5508F6.jpg
            //Rolling_Spider_2014-07-02T085718+0000_0A73A4E2A21B56DBDDFB3CEB1BA0FDAF.jpg


            //ret = bleFtp.sizeFileAL("/internal_000/Rolling_Spider/media/Rolling_Spider_2014-07-03T112436+0000_8858E359787B671FE7408A955294F048.jpg", totalSize);
            //ret = bleFtp.sizeFileAL("/internal_000/Rolling_Spider/thumb/Rolling_Spider_2014-07-01T161736+0000_33539649FC464D41FAD7FB4F0F5508F6.jpg", totalSize);
            //ret = bleFtp.sizeFileAL("/internal_000/Rolling_Spider/thumb/Rolling_Spider_2014-07-02T085718+0000_0A73A4E2A21B56DBDDFB3CEB1BA0FDAF.jpg", totalSize);
            //ret = bleFtp.sizeFileAL("/U_nbpckt.txt", totalSize);

            Log.d("DBG", APP_TAG + "SIZE: " + totalSize[0]);

            byte[][] data = new byte[1][];
            data[0] = null;

            //ret = bleFtp.getFileWithBufferAL("/internal_000/Rolling_Spider/thumb/Rolling_Spider_2014-07-01T161736+0000_33539649FC464D41FAD7FB4F0F5508F6.jpg", data, 0, null);
            //ret = bleFtp.getFileWithBufferAL("/internal_000/Rolling_Spider/thumb/Rolling_Spider_2014-07-02T085718+0000_0A73A4E2A21B56DBDDFB3CEB1BA0FDAF.jpg", data, 0, null);
            //ret = bleFtp.getFileWithBufferAL("/U_nbpckt.txt", data, null, null);
            //ret = bleFtp.getFileWithBufferAL("/rollingspider_update.plf.tmp", data, 0, null);
            //ret = bleFtp.getFileWithBufferAL("/update.plf.tmp", data, 0, null);
            //ret = bleFtp.getFileWithBufferAL("/u.plf.tmp", data, 0, null);

            Log.d("DBG", APP_TAG + "GET with buffer : " + ((data[0] != null) ? data[0].length : "null"));
            
            
            filePath = tmp + "/txt.tmp";
            
            //ret = bleFtp.getFileAL("/a.txt", filePath, 0, null);
            //ret = bleFtp.getFileAL("/u.plf.tmp", filePath, 0, null);
            
            Log.d("DBG", APP_TAG + "GET " + ret);
                            
            try
            {
                filePath = tmp + "/txt.plf.tmp";

                FileOutputStream dst = new FileOutputStream(filePath);

                //byte[] buffer = new String("123\n").getBytes("UTF-8");
                byte[] buffer = new byte[132];

                for (int i=0; i<500; i++)
                {
                    dst.write(buffer, 0, buffer.length);
                }
                dst.flush();
                dst.close();

                ret = bleFtp.putFileAL("/a.plf.tmp", filePath, 0, false, cancelSem);

                Log.d("DBG", APP_TAG + "PUT : " + ret);
            }
            catch (FileNotFoundException e)
            {
                Log.d("DBG", APP_TAG + e.toString());
            }
            catch (IOException e)
            {
                Log.d("DBG", APP_TAG + e.toString());
            }
            
            
            ret = bleFtp.renameFileAL("/a.txt", "/b.txt");
            Log.d("DBG", APP_TAG + "rename " + ret);
            
            ret = bleFtp.deleteFileAL("/b.txt");
            Log.d("DBG", APP_TAG + "delete " + ret);
            
            bleManager.disconnect();
	    }
	}
	
	public void TestFile()
	{
		//-rw-r--r--    1 root     root       1210512 Jan  1 02:46 ckcm.bin
		String line = "-rw-r--r--    1 root     root       1210512 Jan  1 02:46 ckcm.bin";
		//ARUtilsBLEFtp ftp = new ARUtilsBLEFtp();
		ARUtilsBLEFtp ftp = ARUtilsBLEFtp.getInstance(getApplicationContext());
		double[] size = new double[1];
		boolean ret = true;
		
		//String item = ftp.getListItemSize(line, line.length(), size);
		
		//Log.d("DBG", APP_TAG + " " + size[0]);
		
		
		String list = "dr--------    3 root     root           232 Jan  1  1970 internal_000\n" +
				"-rw-r--r--    1 root     root             0 Jul  1 14:12 list.txt\n";
		
		String[] nextItem = new String[1];
		String prefix = null;
		boolean isDirectory = false;
		int[] indexItem = null;
		int [] itemLen = null;
		
		//String file = ftp.getListNextItem(list, nextItem, prefix, isDirectory, indexItem, itemLen);

		//Log.d("DBG", APP_TAG + " " + file);
		double[] totalSize = new double[1];
		
		ret = ftp.sizeFileAL("/file.txt", totalSize);
	}
	
	public void TestMd5()
	{
		try
		{
			ARSALMd5Manager manager = null;
			ARSAL_ERROR_ENUM result = null;
			String filePath = "";
			byte[] md5 = null;
			
        	File sysHome = this.getFilesDir();// /data/data/com.example.tstdata/files
        	String tmp = sysHome.getAbsolutePath();
        	filePath = tmp + "/txt.txt";
			
			FileOutputStream dst = new FileOutputStream(filePath);
			
			byte[] buffer = new String("123\n").getBytes("UTF-8");
			
			dst.write(buffer, 0, 4);
			dst.flush();
			dst.close();
			
			FileInputStream src = new FileInputStream(filePath);
			byte[] block = new byte[1024];
			int count;
			
			/*java.security.MessageDigest digest = java.security.MessageDigest.getInstance("MD5");
            while ((count = src.read(block, 0, block.length)) > 0)
            {
                digest.update(block, 0, count);
            }
            //ba 1f 25 11 fc 30 42 3b db b1 83 fe 33 f3 dd 0f
            md5 = digest.digest();
            src.close();*/
			
			ARSALMd5 md5Ctx = new ARSALMd5();
			md5 = md5Ctx.compute(filePath);
			
			Log.d("DBG", APP_TAG + "block " + md5Ctx.getTextDigest(block, 0, 3));
			Log.d("DBG", APP_TAG + "md5 " +  md5Ctx.getTextDigest(md5, 0, md5.length));
		
			manager = new ARSALMd5Manager();
			
			manager.init();
			
			md5 = manager.compute(filePath);
			Log.d("DBG", APP_TAG + "md5 " +  md5Ctx.getTextDigest(md5, 0, md5.length));
			
			result = manager.check(filePath, "ba1f2511fc30423bdbb183fe33f3dd0f");
			Log.d("DBG", APP_TAG + "check " + result.toString());
			
			result = manager.close();
			
			manager.dispose();
		}
		
		catch (FileNotFoundException e)
		{
			Log.d("DBG", APP_TAG + e.toString());
		}
		catch (IOException e)
		{
			Log.d("DBG", APP_TAG + e.toString());
		}
		catch (ARSALException e)
		{
			Log.d("DBG", APP_TAG + e.toString());
		}
		catch (Throwable e)
		{
			Log.d("DBG", APP_TAG + e.toString());
		}
		
		Log.d("DBG", APP_TAG + "End");
	}

    private String createTestFile(String filename)
    {
        String filePath = null;
        try
        {
            File sysHome = this.getFilesDir();// /data/data/com.example.tstdata/files
            String tmp = sysHome.getAbsolutePath();
            filePath = tmp + filename/*"/txt.plf.tmp"*/;

            FileOutputStream dst = new FileOutputStream(filePath);

            byte[] buffer = new byte[132];

            for (int i=0; i<10; i++)
            {
                dst.write(buffer, 0, buffer.length);
            }
            dst.flush();
            dst.close();

        }
        catch (FileNotFoundException e)
        {
            Log.d("DBG", APP_TAG + e.toString());
        }
        catch (IOException e)
        {
            Log.d("DBG", APP_TAG + e.toString());
        }
        return filePath;
    }

    private ARUtilsBLEFtp connectToFtp()
    {
        Log.d(APP_TAG, "connect");
        ARUtilsBLEFtp bleFtp = null;
        ARSALBLEManager bleManager = ARSALBLEManager.getInstance(getApplicationContext());
        boolean connected = bleManager.isDeviceConnected();
        if (!connected)
        {
            BluetoothGatt gattDevice = connectToBleDevice(mDevice);
            if (gattDevice != null)
            {
                //bleFtp = new ARUtilsBLEFtp();
                bleFtp = ARUtilsBLEFtp.getInstance(getApplicationContext());
                //bleFtp.initWithDevice(bleManager, gattDevice, 51);
                bleFtp.initWithBLEManager(bleManager);
                bleFtp.registerDevice(gattDevice, 51);
                bleFtp.registerCharacteristics();
            }
        }
        return bleFtp;
    }

    private void disconnectFromFtp()
    {
        Log.d(APP_TAG, "connect");
        ARSALBLEManager bleManager = ARSALBLEManager.getInstance(getApplicationContext());
        if (bleManager.isDeviceConnected())
        {
            bleManager.disconnect();
        }
    }
    
    private boolean getARUtilsManager()
    {
        if (mUtilsManager == null)
        {
            try
            {
                mUtilsManager = new ARUtilsManager();
                mUtilsManager.initBLEFtp(MainActivity.this, mDevice, 51);
            }
            catch (ARUtilsException e) {
                e.printStackTrace();
                Log.e(APP_TAG, "Failed to init utilsManager");
            }
        }
        
        return true;
    }

    public void testBleListFile()
    {
        Log.d(APP_TAG, "testBleListFile");
        
        if (mUtilsManager != null)
        {
            Log.d(APP_TAG, "send LIST command:");

            String listFile = mUtilsManager.BLEFtpListFile("/");
            Log.d(APP_TAG, "\tLIST: [" + listFile + "]");
        }
        else
        {
            Log.e(APP_TAG, "testBleListFile failed because mUtilsManager is null");
        }
    }

    public void testBleGetFile()
    {
        Log.d(APP_TAG, "testBleGetFile");

        if (mUtilsManager != null)
        {
            Log.d(APP_TAG, "send GET command:");

            mUtilsManager.BLEFtpGet("/version.txt", "/sdcard/version.txt", new ARUtilsFtpProgressListener() {
                @Override
                public void didFtpProgress(Object arg, float percent) {
                    Log.i(APP_TAG, "Get file progress: [" + percent + "]");
                }
            }, null, true);
        }
        else
        {
            Log.e(APP_TAG, "testBleGetFile failed because mUtilsManager is null");
        }
    }

    public void testBleGetFileWithBuffer()
    {
        Log.d(APP_TAG, "testBleGetFileWithBuffer");

        if (mUtilsManager != null)
        {
            Log.d(APP_TAG, "send GET_WITH_BUFFER command:");

            byte[] data = mUtilsManager.BLEFtpGetWithBuffer("/version.txt", new ARUtilsFtpProgressListener() {
                @Override
                public void didFtpProgress(Object arg, float percent) {
                    Log.i(APP_TAG, "Get file progress: [" + percent + "]");
                }
            }, null);
            Log.d(APP_TAG, "data GET_WITH_BUFFER: [" + new String(data) + "], datalen = " + data.length);
        }
        else
        {
            Log.e(APP_TAG, "testBleGetFileWithBuffer failed because mUtilsManager is null");
        }

    }

    public void testBlePutFile()
    {
        Log.d(APP_TAG, "testBlePutFile");

        String filePath = createTestFile("toto");

        if (mUtilsManager != null)
        {
            Log.d(APP_TAG, "send PUT command:");

            mUtilsManager.BLEFtpPut("/toto.tmp", filePath, new ARUtilsFtpProgressListener() {
                @Override
                public void didFtpProgress(Object arg, float percent) {
                    Log.i(APP_TAG, "Put file progress: [" + percent + "]");
                }
            }, null, true);
        }
        else
        {
            Log.e(APP_TAG, "testBlePutFile failed because mUtilsManager is null");
        }
    }


    public void testBleDeleteFile()
    {
        Log.d(APP_TAG, "testBleDeleteFile");

        if (mUtilsManager != null)
        {
            Log.d(APP_TAG, "send DELETE command:");

            mUtilsManager.BLEFtpDelete("/toto.tmp");
        }
        else
        {
            Log.e(APP_TAG, "testBleDeleteFile failed because mUtilsManager is null");
        }
    }
    
    public void testBleRenameFile()
    {
        Log.d(APP_TAG, "testBleRenameFile");

        if (mUtilsManager != null)
        {
            Log.d(APP_TAG, "send RENAME command:");

            //mUtilsManager.BLEFtpRename("/toto.tmp", "/toto1.tmp");
        }
        else
        {
            Log.e(APP_TAG, "testBleDeleteFile failed because mUtilsManager is null");
        }
    }

}
