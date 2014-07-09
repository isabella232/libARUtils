package com.parrot.arsdk.arutils;

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.InputMismatchException;
import java.util.Iterator;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Scanner;
import java.util.UUID;
import java.util.concurrent.Semaphore;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.util.Log;

import com.parrot.arsdk.arsal.ARSALBLEManager;
import com.parrot.arsdk.arsal.ARSALBLEManager.ARSALManagerNotificationData;
import com.parrot.arsdk.arsal.ARSALPrint;
import com.parrot.arsdk.arsal.ARSAL_ERROR_ENUM;
import com.parrot.arsdk.arsal.ARUUID;

public class ARUtilsBLEFtp 
{
	//private static final String TAG_ = "ARUtilsBLEFtp";
	private static final String APP_TAG = "BLEFtp ";
	
	public final static String BLE_GETTING_KEY = "kARUTILS_BLEFtp_Getting";
	
	public final static String BLE_PACKET_WRITTEN =        "FILE WRITTEN";
	public final static String BLE_PACKET_NOT_WRITTEN =    "FILE NOT WRITTEN";
	public final static String BLE_PACKET_EOF =            "End of Transfer";
	public final static String BLE_PACKET_RENAME_SUCCESS = "Rename successful";
	public final static String BLE_PACKET_DELETE_SUCCES =  "Delete successful";
	public final static int BLE_PACKET_MAX_SIZE = 132;
	public final static int BLE_PACKET_BLOCK_PUTTING_COUNT = 500;
	public final static int BLE_PACKET_BLOCK_GETTING_COUNT = 100;
	
	public final static long BLE_PACKET_WRITE_SLEEP = 20;
	public final static int BLE_MTU_SIZE = 20;
	
	/*
* Paquet de Start :      ID = 10 (en binaire) + data
* Paquet de "data" :     ID = 00 + data
* Paquet de Stop :       ID = 01 +data
* Paquet unique :        ID = 11 + data
	*/
	
	public final static byte BLE_BLOCK_HEADER_START = 0x02;
	public final static byte BLE_BLOCK_HEADER_CONTINUE = 0x00;
	public final static byte BLE_BLOCK_HEADER_STOP  = 0x01;
	public final static byte BLE_BLOCK_HEADER_SINGLE = 0x03;
	
	private byte[] notificationDataArray = null;
	
	private ARSALBLEManager bleManager = null;
	private BluetoothGatt gattDevice = null;
	private int port;
	
	private BluetoothGattCharacteristic transferring = null;
	private BluetoothGattCharacteristic getting = null;
	private BluetoothGattCharacteristic handling = null;
	private ArrayList<BluetoothGattCharacteristic> arrayGetting = null;
	private Semaphore cancelSem = null;
	
	private native void nativeProgressCallback(long nativeCallback, float percent);
	
	private native static void nativeJNIInit();

	static
    {
        nativeJNIInit();
    }
	
	public ARUtilsBLEFtp()
	{
	}
	
	public void initWithDevice(ARSALBLEManager bleManager, BluetoothGatt gattDevice, int port)
	{
		this.bleManager = bleManager;
		this.gattDevice = gattDevice;
		this.port = port;
		this.cancelSem = new Semaphore(0);
	}
	
	public boolean registerCharacteristics()
	{
		List<BluetoothGattService> services = gattDevice.getServices();
		ARSAL_ERROR_ENUM error = ARSAL_ERROR_ENUM.ARSAL_OK;
		boolean ret = true;
		
		ARSALPrint.w("DBG", APP_TAG + "registerCharacteristics");
		
		Iterator<BluetoothGattService> servicesIterator = services.iterator();
		while (servicesIterator.hasNext())
		{
			BluetoothGattService service = servicesIterator.next();
			String serviceUuid = ARUUID.getShortUuid(service.getUuid());	
			//String serviceUuid = service.getUuid().toString();
			String name = ARUUID.getShortUuid(service.getUuid());
			//String name = service.getUuid().toString();
			ARSALPrint.w("DBG", APP_TAG + "service " + name);
			
			if (serviceUuid.startsWith(String.format(/*"0000"+*/"fd%02d", this.port)))
			{
				List<BluetoothGattCharacteristic> characteristics = service.getCharacteristics();
				Iterator<BluetoothGattCharacteristic> characteristicsIterator = characteristics.iterator();
				
				while (characteristicsIterator.hasNext())
				{
					BluetoothGattCharacteristic characteristic = characteristicsIterator.next();
					String characteristicUuid = ARUUID.getShortUuid(characteristic.getUuid());
					//String characteristicUuid = characteristic.getUuid().toString();
					ARSALPrint.w("DBG", APP_TAG + "characteristic " + characteristicUuid);
					
					if (characteristicUuid.startsWith(String.format("fd%02d", this.port + 1)))
					{
						this.transferring = characteristic;
					}
					else if (characteristicUuid.startsWith(String.format("fd%02d", this.port + 2)))
					{
						this.arrayGetting = new ArrayList<BluetoothGattCharacteristic>();
						this.arrayGetting.add(characteristic);
						this.getting = characteristic;
						
						ARSALPrint.w("DBG", APP_TAG + "set " + error.toString());
						
					}
					else if (characteristicUuid.startsWith(String.format("fd%02d", this.port + 3)))
					{
						this.handling = characteristic;
					}
				}
			}
		}
		
		if ((transferring != null) && (getting != null) && (handling != null))
		{
			/*error = bleManager.setCharacteristicNotification(gettingService, this.getting);
			if (error != ARSAL_ERROR_ENUM.ARSAL_OK)
			{
				ARSALPrint.d("DBG", APP_TAG + "set " + error.toString());
				ret = false;
			}*/
			
			if (ret == true)
			{
				bleManager.registerNotificationCharacteristics(this.arrayGetting, BLE_GETTING_KEY);
			}
		}
		
		return ret;
	}
	
	public boolean unregisterCharacteristics()
	{
		boolean ret = true;
		
		ARSALPrint.w("DBG", APP_TAG + "unregisterCharacteristics");
		
		ret = bleManager.unregisterNotificationCharacteristics(BLE_GETTING_KEY);
		
		return ret;
	}
	
	public boolean cancelFile()
	{
		boolean ret = true;
		
		cancelSem.release();
		
		bleManager.cancelReadNotification(BLE_GETTING_KEY);
		
		return ret;
	}
	
	private String normalizeName(String name)
	{
		String newName = name;
		if (name.charAt(0) != '/')
		{
			newName = "/" + name;
		}
		
		return newName;
	}
	
	private boolean isConnectionCanceled()
	{
		boolean ret = false;
		
		ret = cancelSem.tryAcquire();
		if (ret  == true)
		{
			cancelSem.release();
		}
		
		return ret;
	}
	
	public boolean sizeFile(String remoteFile, double[] fileSize)
	{
		String[] resultList = new String[1];
		resultList[0] = null;
		String remotePath = null;
		String remoteFileName = null;
		int idx = 0;
		int endIdx = -1;
		boolean found = false;
		boolean ret = true;
		
		ARSALPrint.w("DBG", APP_TAG + "sizeFile " + remoteFile);
		
		remoteFile = normalizeName(remoteFile);
		
		fileSize[0] = 0;

		while ((idx = remoteFile.indexOf('/', idx)) != -1)
		{
			idx++;
			endIdx = idx;
		}
		
		if (endIdx != -1)
		{
			remotePath = remoteFile.substring(0, endIdx);
			remoteFileName = remoteFile.substring(endIdx, remoteFile.length());
		}
		
		ret = listFiles(remotePath, resultList);
		
		if ((ret == true) && (resultList[0] != null))
		{
			String[] nextItem = new String[1];
			nextItem[0] = null; 
			int[] indexItem = new int[1];
			indexItem[0] = 0;
			int[] itemLen = new int[1];
			itemLen[0] = 0;
			String fileName = null;
			
			while ((found == false) && (fileName = getListNextItem(resultList[0], nextItem, null, false, indexItem, itemLen)) != null)
			{
				Log.d("DBG", APP_TAG + "file " + fileName);
				
				if (remoteFileName.contentEquals(fileName))
				{
					if (getListItemSize(resultList[0], indexItem[0], itemLen[0], fileSize) == null)
					{
						ret = false;
					}
					else
					{
						found = true;
					}
				}
			}
		}
		
		return ret;
	}
	
	public boolean listFiles(String remotePath, String[] resultList)
	{
		boolean ret = true;
		
		ARSALPrint.w("DBG", APP_TAG + "listFiles " + remotePath);
		
		ret = sendCommand("LIS", remotePath, handling);
		
		if (ret == true)
		{
			byte[][] data = new byte[1][];
			
			ret = readGetData(0, null, data, 0);
			
			if (data[0] != null)
			{
				ARSALPrint.w("DBG", APP_TAG + "listFiles==" + new String(data[0]) + "==");
			}
			
			if ((ret == true) && (data[0] != null)) 
			{
				resultList[0] = new String(data[0]);
			}
		}
		
		return ret;
	}
	
	public boolean getFile(String remotePath, String localFile, long nativeCallback)
	{
		boolean ret = false;
		ret = getFileInternal(remotePath, localFile, null, nativeCallback);
		
		return ret;
	}
	
	public boolean getFileWithBuffer(String remotePath, byte[][] data, long nativeCallback)
	{
		boolean ret = false;
		
		ret = getFileInternal(remotePath, null, data, nativeCallback);
		
		return ret;
	}
	
	public boolean getFileInternal(String remoteFile, String localFile, byte[][] data, long nativeCallback)
	{
		FileOutputStream dst = null;
		boolean ret = true;
		double[] totalSize = new double[1];
		totalSize[0] = 0.f;
		
		ARSALPrint.w("DBG", APP_TAG + "getFile " + remoteFile);
		
		remoteFile = normalizeName(remoteFile);
		
		ret = sizeFile(remoteFile, totalSize);
		
		if (ret == true)
		{
			ret = sendCommand("GET", remoteFile, handling);
		}
		
		if ((ret == true) && (localFile != null))
		{
			try
			{
				dst = new FileOutputStream(localFile);
			}
			catch(FileNotFoundException e)
			{
				ARSALPrint.e("DBG", APP_TAG + e.toString());
				ret = false;
			}
		}
		
		if (ret == true)
		{
			ret = readGetData((int)totalSize[0], dst, data, nativeCallback);
		}
		
		if (dst != null)
		{
			try { dst.close(); } catch(IOException e) { }
		}
		
		return ret;
	}
	
	public boolean abortPutFile(String remotePath)
	{
		int[] resumeIndex = new int[1];
		resumeIndex[0] = 0;
		boolean resume = false;
		boolean ret = true;
		
		ret = readPutResumeIndex(remotePath, resumeIndex);
		if ((ret == true) && (resumeIndex[0] > 0))
		{
			resume = true;
		}
		else
		{
			resume = false;
		}
		
		if (resume == true)
		{
			ret = sendCommand("PUT", remotePath, handling); 
		
			if (ret == true)
			{
				ret = sendPutData(0, null, resumeIndex[0], false, true, 0);
			}
		}
		
		return ret;
	}
	
	public boolean putFile(String remotePath, String localFile, long nativeCallback, boolean resume)
	{
		FileInputStream src = null;
		int[] resumeIndex = new int[1];
		resumeIndex[0] = 0;
		boolean ret = true;
		int totalSize = 0;
		
		ARSALPrint.w("DBG", APP_TAG + "putFile " + remotePath);
		
		remotePath = normalizeName(remotePath);
		
		/*if (resume == true)
		{
			abortPutFile(remotePath);
		}
		else
		{
			if (ret == true)
			{
				ret = readPutResumeIndex(remotePath, resumeIndex);
				if (ret == false)
				{
					ret = true;
					resumeIndex[0] = 0;
					resume = false;
				}
			}
			
			if (resumeIndex[0] > 0)
			{
				resume = false;
			}
		}*/
	
		ARUtilsFileSystem fileSys = new ARUtilsFileSystem();
		try
		{
			totalSize = (int)fileSys.getFileSize(localFile);
		}
		catch (ARUtilsException e)
		{
			ret = false;
		}
		
		if (ret == true)
		{
			ret = sendCommand("PUT", remotePath, handling);
		}
		
		if (ret == true)
		{
			try
			{
				src = new FileInputStream(localFile);
			}
			catch(FileNotFoundException e)
			{
				ret = false;
			}
		}
		
		if (ret == true)
		{
			ret = sendPutData(totalSize, src, resumeIndex[0], resume, false, nativeCallback);
		}
		
		if (src != null)
		{
			try { src.close(); } catch(IOException e) { }
		}
		
		return ret;
	}
	
	public boolean deleteFile(String remoteFile)
	{
		boolean ret = true;
		
		ret = sendCommand("DEL", remoteFile, handling);
		
		if (ret == true)
		{
			ret = readDeleteData();
		}
		
		return ret;
	}
	
	public boolean renameFile(String oldNamePath, String newNamePath)
	{
		boolean ret = true;
		String param = oldNamePath + " " + newNamePath;
		
		ret = sendCommand("REN", param, handling);
		
		if (ret == true)
		{
			ret = readRenameData();
		}
		
		return ret;
	}
	
	private boolean sendCommand(String cmd, String param, BluetoothGattCharacteristic characteristic)
	{
		boolean ret = true;
		byte[] bufferParam = null;
		byte[] bufferCmd = null;
		byte[] buffer = null;
		int indexBuffer = 0;
		
		try
		{
			bufferCmd = cmd.getBytes("UTF8");
		}
		catch (UnsupportedEncodingException e)
		{
			ret = false;
		}
		
		if ((ret == true) && (param != null))
		{
			try
			{
				bufferParam = param.getBytes("UTF8");
			}
			catch (UnsupportedEncodingException e)
			{
				ret = false;
			}
		}

		if (ret == true)
		{
			if ((bufferParam != null) && ((cmd.length() + bufferParam.length + 1) > BLE_PACKET_MAX_SIZE))
			{
				ret = false;
			}
		}
		
		if (ret == true)
		{
			if (bufferParam == null)
			{
				buffer = new byte[bufferCmd.length + 1];
			}
			else
			{
				buffer = new byte[bufferCmd.length + bufferParam.length + 1];
			}
		}
		
		System.arraycopy(bufferCmd, 0, buffer, 0, bufferCmd.length);
		indexBuffer = bufferCmd.length;
		
		if (bufferParam != null)
		{
			System.arraycopy(bufferParam, 0, buffer, indexBuffer, bufferParam.length);
			indexBuffer += bufferParam.length;
		}
		buffer[indexBuffer] = 0;
		ret = sendBufferBlocks(buffer, characteristic);
		
		return ret;
	}
	
	private boolean sendResponse(String cmd, BluetoothGattCharacteristic characteristic)
	{
		boolean ret = true;
		byte[] bufferCmd = null;
		byte[] buffer = null;
		int indexBuffer = 0;
		
		try
		{
			try
			{
				bufferCmd = cmd.getBytes("UTF8");
			}
			catch (UnsupportedEncodingException e)
			{
				ret = false;
			}
	
			if (ret == true)
			{
				if (((cmd.length() + 1) > BLE_PACKET_MAX_SIZE))
				{
					ret = false;
				}
			}
			
			if (ret == true)
			{
				buffer = new byte[bufferCmd.length + 1];
			}
			
			System.arraycopy(bufferCmd, 0, buffer, 0, bufferCmd.length);
			indexBuffer = bufferCmd.length;
			
			buffer[indexBuffer] = 0;
			
			Thread.sleep(BLE_PACKET_WRITE_SLEEP, 0);
			ret = bleManager.writeData(buffer, characteristic);
		} 
		catch (InterruptedException e) 
		{
		}
		
		return ret;
	}
	
	boolean sendBufferBlocks(byte[] buffer, BluetoothGattCharacteristic characteristic)
	{
		boolean ret = true;
		
		try
		{
			int bufferIndex = 0;
			while ((ret == true) && (bufferIndex < buffer.length))
			{			
				int blockSize = BLE_MTU_SIZE;
				if ((buffer.length - bufferIndex) <= (BLE_MTU_SIZE -1))
				{
					blockSize = (buffer.length - bufferIndex) + 1;
				}
				byte[] block = new byte[blockSize];
	
				if (buffer.length < BLE_MTU_SIZE)
				{
					block[0] = BLE_BLOCK_HEADER_SINGLE;
				}
				else
				{
					if (bufferIndex == 0)
					{
						block[0] = BLE_BLOCK_HEADER_START;
					}
					else if (bufferIndex + (BLE_MTU_SIZE - 1) >= buffer.length) 
					{
						block[0] = BLE_BLOCK_HEADER_STOP;
					}
					else
					{
						block[0] = BLE_BLOCK_HEADER_CONTINUE;
					}
				}
				
				System.arraycopy(buffer, bufferIndex, block, 1, blockSize - 1);
				bufferIndex += blockSize - 1;
				Thread.sleep(BLE_PACKET_WRITE_SLEEP, 0);
				ret = bleManager.writeData(block, characteristic);
				
				ARSALPrint.w("DBG", APP_TAG + "block " + blockSize + ", " + bufferIndex);
				
				/*if (isConnectionCanceled())
				{
					ret = false;
				}*/
			}
		} 
		catch (InterruptedException e) 
		{
		}
		
		return ret;
	}
	
	boolean readBufferBlocks(byte[][] notificationArray)
	{
		ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>();
		ARSALManagerNotificationData notificationData = null;
		boolean ret = true;
		boolean end = false;
		int bufferIndex = 0;
		byte[] buffer = null;
		int blockCount = 0;
		
		do
		{
			if (notificationDataArray != null)
			{
				buffer = notificationDataArray;
				bufferIndex += notificationDataArray.length;
				notificationDataArray = null;
			}
			
			if (receivedNotifications.size() == 0)
			{
				ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
			}
			
			if ((ret == true) && (receivedNotifications.size() > 0))
			{
				notificationData = receivedNotifications.get(0);
				int blockLen = notificationData.value.length;
				byte[] block = notificationData.value;
				int blockIndex = 0;
				
				Log.d("DBG", APP_TAG + "Block length " + blockLen);
				if (blockLen > 0)
				{
					switch(block[0])
					{
					case BLE_BLOCK_HEADER_SINGLE:
					case BLE_BLOCK_HEADER_STOP:
						end = true;
						blockLen = blockLen -1;
						blockIndex = 1;
						break;
						
					case BLE_BLOCK_HEADER_CONTINUE:
					case BLE_BLOCK_HEADER_START:
						blockLen = blockLen -1;
						blockIndex = 1;
						break;
					default:
						/*end = true;
						blockIndex = 0;*/
						ret = false;
						break;
					}
					
					if (ret == true)
					{
						if ((bufferIndex + blockLen) > BLE_PACKET_MAX_SIZE)
						{
							Log.d("DBG", APP_TAG + "Packet length " + bufferIndex);
							
							int size = bufferIndex + blockLen - BLE_PACKET_MAX_SIZE;
							notificationDataArray = new byte[size];
							System.arraycopy(block, BLE_MTU_SIZE - size, notificationDataArray, 0, size);
							blockLen -= size;
						}
						
						byte[] oldBuffer = buffer;
						buffer = new byte[bufferIndex + blockLen];
						if (oldBuffer != null)
						{
							System.arraycopy(oldBuffer, 0, buffer, 0, oldBuffer.length);
						}
						
						/*if (blockLen < 0)
						{
							//blockLen = blockLen;
							blockLen++;
							blockLen--;
						}*/
						
						System.arraycopy(block, blockIndex, buffer, bufferIndex, blockLen);
						bufferIndex += blockLen;
						blockCount++;
						
						Log.d("DBG", APP_TAG + "block " + blockCount +", "+ blockLen +", "+ bufferIndex);
					}
				}
				else
				{
					//ret = false;
					Log.d("DBG", APP_TAG + "Empty block ");
				}
				
				receivedNotifications.remove(notificationData);
			}
		}
		while ((ret == true) && (end == false));
		
		if (buffer != null)
		{
			notificationArray[0] = buffer;
		}
		
		return ret;
	}
	
	private boolean sendPutData(int fileSize, FileInputStream src, int resumeIndex, boolean resume, boolean abort, long nativeCallback)
	{
		BufferedInputStream in = new BufferedInputStream(src);
		byte[] buffer = new byte [BLE_PACKET_MAX_SIZE];
		boolean ret = true;
		int packetLen = 0;
		int packetCount = 0;
		int totalPacket = 0;
		int totalSize = 0;
		boolean endFile = false;
		ARUtilsMD5 md5 = new ARUtilsMD5();
		ARUtilsMD5 md5End = new ARUtilsMD5();
		String[] md5Msg = new String[1];
		md5Msg[0] = "";
		String md5Txt = null;
		byte[] send = null;
		
		try
		{
			do
			{
				if (abort == false)
				{
					packetLen = in.read(buffer, 0, BLE_PACKET_MAX_SIZE);
				}
				
				if (packetLen > 0)
				{
					packetCount++;
					totalPacket++;
					totalSize += packetLen;
					
					md5End.update(buffer, 0, packetLen);
					
					if ((resume == false) || ((resume == true) && (totalPacket > resumeIndex)))
					{
						md5.update(buffer, 0, packetLen);
						
						send = buffer;
						if (packetLen != BLE_PACKET_MAX_SIZE)
						{
							send = new byte[packetLen];
							System.arraycopy(buffer, 0, send, 0, packetLen);
						}
						
						//Thread.sleep(BLE_PACKET_WRITE_SLEEP);
						//ret = bleManager.writeData(send, transferring);
						ret = sendBufferBlocks(send, transferring);
						
						ARSALPrint.w("DBG", APP_TAG + "packet " + packetCount + ", " + packetLen);
					}
					else
					{
						ARSALPrint.w("DBG", APP_TAG + "resume " + packetCount);
					}

					if (nativeCallback != 0)
					{
						nativeProgressCallback(nativeCallback, ((float)totalSize / (float)fileSize) * 100.f);
					}
					
					/*if (progressListener != null)
					{
						progressListener.didFtpProgress(progressArg, ((float)totalSize / (float)fileSize) * 100.f);
					}*/
					
					/*if (isConnectionCanceled())
					{
						ret = false;
					}*/
				}
				else 
				{
					if (packetLen == -1)
					{
						endFile = true;
					}
				}
				
				if ((ret == true) && ((packetCount >= BLE_PACKET_BLOCK_PUTTING_COUNT) || ((endFile == true) && (packetCount > 0))))
				{
					packetCount = 0;
					
					if ((resume == false) || ((resume ==  true) && (totalPacket > resumeIndex)))
					{
						md5Txt = md5.digest();
						md5.initialize();
				
						ARSALPrint.w("DBG", APP_TAG + "sending md5 " + md5Txt);
						
						md5Txt = "MD5" + md5Txt;
						send = md5Txt.getBytes("UTF8");
						//Thread.sleep(BLE_PACKET_WRITE_SLEEP);
						//ret = bleManager.writeData(send, transferring);
						ret = sendBufferBlocks(send, transferring);
						
						if (ret == true)
						{
							ret = readPudDataWritten();
						}
					}
				}
			}
			while ((ret == true) && (endFile == false));
			
			if ((ret == true) && (endFile == true))
			{
				send = new byte[0];
				//Thread.sleep(BLE_PACKET_WRITE_SLEEP);
				//ret = bleManager.writeData(send, transferring);
				ret = sendBufferBlocks(send, transferring);
				
				if (ret == true)
				{
					ret = readPutMd5(md5Msg);
				}
		
				if (ret == true)
				{
					md5Txt = md5End.digest();
					
					ARSALPrint.w("DBG", APP_TAG + "md5 end" + md5Txt);
					
					if (md5Msg[0].compareTo(md5Txt) != 0)
					{
						ARSALPrint.w("DBG", APP_TAG + "md5 end Failed");
						ret = false;
					}
					else
					{
						ARSALPrint.w("DBG", APP_TAG + "md5 end ok");
					}
				}
			}
		}
		/*catch (InterruptedException e)
		{
			ARSALPrint.e("DBG", APP_TAG + e.toString());
			ret = false;
		}*/
		catch (IOException e)
		{
			ARSALPrint.e("DBG", APP_TAG + e.toString());
			ret = false;
		}
		
		return ret;
	}
	
	/*private boolean readPutResumeIndex(int[] resumeIndex)
	{
		ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>(); 
		boolean ret = true;
		
		bleManager.readData(getting);
		
		ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
		if (ret = true)
		{
			if (receivedNotifications.size() > 0)
			{
				ARSALManagerNotificationData  notificationData = receivedNotifications.get(0);
				byte[] packet = notificationData.value;
				int packetLen = notificationData.value.length;
	
				if (packetLen == 3)
				{
					int size = (0xff & packet[0]) | (0xff00 & (packet[1] << 8)) | (0xff0000 & (packet[2] << 16));
					resumeIndex[0] = size;
					ARSALPrint.w("DBG", APP_TAG + "resume index " + size);
				}
				else
				{
					ret = false;
				}
			}
			else
			{
				ret = false;
			}
		}
		
		return ret;
	}*/
	private boolean readPutResumeIndex(String remoteFile, int[] resumeIndex)
	{
		double[] fileSize = new double[1];
		fileSize[0] = 0.f;
		boolean ret = true;
		
		resumeIndex[0] = 0;
		ret = sizeFile(remoteFile, fileSize);
		if (ret == true)
		{
			resumeIndex[0] = (int)fileSize[0] / BLE_PACKET_MAX_SIZE;
		}
	
		return ret;
	}
	
	private boolean readGetData(int fileSize, FileOutputStream dst, byte[][] data, long nativeCallback)
	{
		//ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>();
		//ArrayList<ARSALManagerNotificationData> removeNotifications = new ArrayList<ARSALManagerNotificationData>();
		byte[][] notificationArray = new byte[1][];
		boolean ret = true;
		int packetCount = 0;
		int totalSize = 0;
		int totalPacket = 0;
		boolean endFile = false;
		boolean endMD5 = false;
		String md5Msg = null;
		String md5Txt = null;
		ARUtilsMD5 md5 = new ARUtilsMD5();
		ARUtilsMD5 md5End = new ARUtilsMD5();
		
		while ((ret == true) && (endMD5 == false))
		{
			boolean blockMD5 = false;
			md5.initialize();
			
			do
			{
				/*if (receivedNotifications.size() == 0)
				{
					ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
				}*/
				ret = readBufferBlocks(notificationArray);
				if (ret == false)
				{
					ret = bleManager.isDeviceConnected();
				}
				else
				{
					//for (int i=0; (i < receivedNotifications.size()) && (ret == true) && (blockMD5 == false) && (endMD5 == false); i++)
					if ((ret == true) && (notificationArray[0] != null) && (blockMD5 == false) && (endMD5 == false))
					{
						/*ARSALManagerNotificationData notificationData = receivedNotifications.get(i);
						int packetLen = notificationData.value.length;
						byte[] packet = notificationData.value;*/
						int packetLen = notificationArray[0].length;
						byte[] packet = notificationArray[0];
						
						packetCount++;
						totalPacket++;
						Log.d("DBG", APP_TAG + "== packet " + packetLen +", "+ packetCount + ", " + totalPacket + ", " + totalSize);
						String s = new String(packet);
						Log.d("DBG", APP_TAG + "packet " + s);
						
						if (packetLen > 0)
						{
							if (endFile == true)
							{
								endMD5 = true;
								
								if (packetLen == (ARUtilsMD5.MD5_LENGTH * 2))
								{
									md5Msg = new String(packet, 0, packetLen);
									ARSALPrint.w("DBG", APP_TAG + "md5 end received " + md5Msg);
								}
								else
								{
									ret = false;
									ARSALPrint.w("DBG", APP_TAG + "md5 end failed size " + packetLen);
								}
							}
							else if (compareToString(packet, packetLen, BLE_PACKET_EOF))
							{
								endFile = true;
								
								if (packetLen == (BLE_PACKET_EOF.length() + 1))
								{
									ARSALPrint.w("DBG", APP_TAG + "End of file received ");
								}
								else
								{
									ARSALPrint.w("DBG", APP_TAG + "End of file failed size " + packetLen);
									ret = false;
								}
							}
							else if (compareToString(packet, packetLen, "MD5"))
							{
								if (packetCount > (BLE_PACKET_BLOCK_GETTING_COUNT + 1))
								{
									ARSALPrint.w("DBG", APP_TAG + "md5 failed packet count " + packetCount);
								}
								
								if (packetLen == ((ARUtilsMD5.MD5_LENGTH * 2) + 3))
								{
									blockMD5 = true;
									md5Msg = new String(packet, 3, packetLen - 3);
									ARSALPrint.w("DBG", APP_TAG + "md5 received " + md5Msg);
								}
								else
								{
									ret = false;
									ARSALPrint.w("DBG", APP_TAG + "md5 failed size " + packetLen);
								}
							}
							else
							{
								totalSize += packetLen;
								md5.update(packet, 0, packetLen);
								md5End.update(packet, 0, packetLen);
								
								if (dst != null)
								{
									try
									{
										dst.write(packet, 0, packetLen);
									}
									catch (IOException e)
									{
										ARSALPrint.e("DBG", APP_TAG + "failed writting file " + e.toString());
										ret = false;
									}
								}
								else
								{
									byte[] newData = new byte[totalSize];
									if (data[0] != null)
									{
										System.arraycopy(data[0], 0, newData, 0, totalSize - packetLen);
									}
									System.arraycopy(packet, 0, newData, totalSize - packetLen, packetLen);
									data[0] = newData;
								}
								if (nativeCallback != 0)
								{
									nativeProgressCallback(nativeCallback, ((float)totalSize / (float)fileSize) * 100.f);
								}
								
								/*if (isConnectionCanceled())
								{
									ret = false;
								}*/
							}
						}
						else
						{
							//empty âcket authorized
						}
					}
				}
				
				//receivedNotifications.clear();
				notificationArray[0] = null;
			}
			while ((ret == true) && (blockMD5 == false) && (endMD5 == false));
			
			if ((ret == true) && (blockMD5 == true))
			{
				blockMD5 = false;
				packetCount = 0;
				md5Txt = md5.digest();
				
				if (md5Msg.contentEquals(md5Txt) == false)
				{
					ARSALPrint.w("DBG", APP_TAG + "md5 block failed");
					//TOFIX some 1st md5 packet failed !!!!
					//ret = false;
				}
				else
				{
					ARSALPrint.w("DBG", APP_TAG + "md5 block ok");
				}
				
				ret = sendResponse("MD5 OK", getting);
			}
		}
		
		if (endMD5 == true)
		{
			md5Txt = md5End.digest();
			ARSALPrint.w("DBG", APP_TAG + "md5 end computed " + md5Txt);
			
			if (md5Msg.contentEquals(md5Txt) == false)
			{
				ARSALPrint.w("DBG", APP_TAG + "md5 end Failed");
				ret = false;
			}
			else
			{
				ARSALPrint.w("DBG", APP_TAG + "md5 end OK");
			}
		}
		else
		{
			ret = false;
		}
		
		return ret;
	}
	
	private boolean readPudDataWritten()
	{
		//ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>();
		byte[][] notificationArray = new byte[1][];
		boolean ret = false;
		
		/*try
		{*/
			//ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
			ret = readBufferBlocks(notificationArray);
			if (ret = true)
			{
				//if (receivedNotifications.size() > 0)
				if (notificationArray[0] != null)
				{
					/*ARSALManagerNotificationData  notificationData = receivedNotifications.get(0);
					byte[] packet = notificationData.value;
					int packetLen = notificationData.value.length;*/
					int packetLen = notificationArray[0].length;
					byte[] packet = notificationArray[0];
					
					if (packetLen > 0)
					{
						//String packetTxt = new String(packet, 0, packetLen, "UTF8");
						
						//if (packetTxt.compareTo(BLE_PACKET_WRITTEN) == 0)
						if (compareToString(packet, packetLen, BLE_PACKET_WRITTEN))
						{
							ARSALPrint.w("DBG", APP_TAG + "Written OK");
							ret = true;
						}
						//else if (packetTxt.compareTo(BLE_PACKET_NOT_WRITTEN) == 0)
						else if (compareToString(packet, packetLen, BLE_PACKET_NOT_WRITTEN))
						{
							ARSALPrint.w("DBG", APP_TAG + "NOT Written");
							ret = false;
						}
						else
						{
							ARSALPrint.w("DBG", APP_TAG + "UNKNOWN Written");
							ret = false;
						}
					}
				}
				else
				{
					ARSALPrint.w("DBG", APP_TAG + "UNKNOWN Written");
					ret = false;
				}
			}
		//}
		/*catch (UnsupportedEncodingException e)
		{
			ARSALPrint.e("DBG", APP_TAG + e.toString());
			ret = false;
		}*/
		
		return ret;
	}
	
	private boolean readPutMd5(String[] md5Txt)
	{
		//ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>();
		byte[][] notificationArray = new byte[1][];
		boolean ret = false;
		
		md5Txt[0] = "";
	
		try
		{
			//ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
			ret = readBufferBlocks(notificationArray);
			if (ret == true)
			{
				//if (receivedNotifications.size() > 0)
				if (notificationArray[0] != null)
				{
					/*ARSALManagerNotificationData  notificationData = receivedNotifications.get(0);
					byte[] packet = notificationData.value;
					int packetLen = notificationData.value.length;*/
					int packetLen = notificationArray[0].length;
					byte[] packet = notificationArray[0];
					
					if (packetLen > 0)
					{
						String packetTxt = new String(packet, 0, packetLen, "UTF8");
						md5Txt[0] = packetTxt.substring(3, packetTxt.length() - 3);
						
						ARSALPrint.w("DBG", APP_TAG + "md5 end received " + md5Txt[0]);
					}
					else 
					{
						ARSALPrint.w("DBG", APP_TAG + "md5 end failed");
						ret = false;
					}
				}
				else 
				{
					ret = false;
				}
			}
		}
		catch (UnsupportedEncodingException e)
		{
			ARSALPrint.e("DBG", APP_TAG + e.toString());
			ret = false;
		}
		
		return ret;
	}
	
	private boolean readRenameData()
	{
		//ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>();
		byte[][] notificationArray = new byte[1][];
		boolean ret = false;
	
		//ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
		ret = readBufferBlocks(notificationArray);
		if (ret == true)
		{
			//if (receivedNotifications.size() > 0)
			if (notificationArray[0] != null)
			{
				/*ARSALManagerNotificationData  notificationData = receivedNotifications.get(0);
				byte[] packet = notificationData.value;
				int packetLen = notificationData.value.length;*/
				int packetLen = notificationArray[0].length;
				byte[] packet = notificationArray[0];					
				
				if (packetLen > 0)
				{
					String packetString = new String(packet);
					if ((packetLen == BLE_PACKET_RENAME_SUCCESS.length()) && packetString.contentEquals(BLE_PACKET_RENAME_SUCCESS))
					{
						ret = true;
					}
					else
					{
						ret = false;
					}
				}
				else 
				{
					ret = false;
				}
			}
			else 
			{
				ret = false;
			}
		}
		return ret;
	}
	
	private boolean readDeleteData()
	{
		//ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>();
		byte[][] notificationArray = new byte[1][];
		boolean ret = false;
	
		//ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
		ret = readBufferBlocks(notificationArray);
		if (ret == true)
		{
			//if (receivedNotifications.size() > 0)
			if (notificationArray[0] != null)
			{
				/*ARSALManagerNotificationData  notificationData = receivedNotifications.get(0);
				byte[] packet = notificationData.value;
				int packetLen = notificationData.value.length;*/
				int packetLen = notificationArray[0].length;
				byte[] packet = notificationArray[0];					
				
				if (packetLen > 0)
				{
					String packetString = new String(packet);
					if ((packetLen == BLE_PACKET_DELETE_SUCCES.length()) && packetString.contentEquals(BLE_PACKET_DELETE_SUCCES))
					{
						ret = true;
					}
					else
					{
						ret = false;
					}
				}
				else 
				{
					ret = false;
				}
			}
			else 
			{
				ret = false;
			}
		}
		return ret;
	}
	
	private boolean compareToString(byte[] buffer, int len, String str)
	{
		boolean ret = false;
		byte[] strBytes = null;
		
		try
		{
			strBytes = str.getBytes("UTF8");
			if (len >= strBytes.length)
			{
				ret = true;
				for (int i=0; i<strBytes.length; i++)
				{
					if (buffer[i] != strBytes[i])
					{
						ret = false;
						break;
					}
				}
			}
			else
			{
				ret = false;
			}
		}
		catch (UnsupportedEncodingException e)
		{
			ARSALPrint.e("DBG", APP_TAG + e.toString());
			ret = false;
		}
		
		return ret;
	}
	
	public static String getListNextItem(String list, String[] nextItem, String prefix, boolean isDirectory, int[] indexItem, int[] itemLen)
	{
	    String lineData = null;
	    String item = null;
	    String line = null;
	    int fileIdx = 0;
	    int endLine = 0;
	    int ptr;
	    
	    if ((list != null) && (nextItem != null))
	    {
	        if (nextItem[0] == null)
	        {
	            nextItem[0] = list;
	            if (indexItem != null)
	            {
	            	indexItem[0] = 0;
	            }
	        }
	        else
	        {
	        	if (indexItem != null)
	            {
	            	indexItem[0] += itemLen[0];
	            }
	        }
	        
	        ptr = 0;
	        while ((item == null) && (ptr != -1))
	        {
	            line = nextItem[0];
	            endLine =  line.length();
	            ptr = line.indexOf('\n');
	            if (ptr == -1)
	            {
	                ptr = line.indexOf('\r');
	            }
	            
	            if (ptr != -1)
	            {
	                endLine = ptr;
	                if (line.charAt(endLine - 1) == '\r')
	                {
	                    endLine--;
	                }
	                
	                ptr++;
	                nextItem[0] = line.substring(ptr);
	                fileIdx = 0;
	                if (line.charAt(0) == ((isDirectory  == true) ? 'd' : '-'))
	                {
	                    int varSpace = 0;
	                    while (((ptr = line.indexOf('\u0020', fileIdx)) != -1) && (ptr < endLine) && (varSpace < 8))
	                    {
	                        if (line.charAt(ptr + 1) != '\u0020')
	                        {
	                            varSpace++;
	                        }
	                            
	                        fileIdx = ++ptr;
	                    }
	                    
	                    if ((prefix != null) && (prefix.length() != 0))
	                    {
	                        if (line.indexOf(prefix, fileIdx) != -1)
	                        {
	                            fileIdx = -1;
	                        }
	                    }
	                    
	                    if (fileIdx != -1)
	                    {
	                    	int len = endLine - fileIdx;
	                        lineData = line.substring(fileIdx, fileIdx + len);
	                        item = lineData;
	                    }
	                }
	            }
	        }
	        
	        if (itemLen != null)
	        {
	            itemLen[0] = endLine;
	            //Log.d("DBG", APP_TAG + "LINE " + list.substring(indexItem[0], indexItem[0] + itemLen[0])); 
	        }
	    }
	    
	    return item;
	}
	
	//-rw-r--r--    1 root     root       1210512 Jan  1 02:46 ckcm.bin
	public static String getListItemSize(String list, int lineIndex, int lineSize, double[] size)
	{
	    int fileIdx;
	    int sizeIdx;
	    int endLine;
	    int ptr;
	    String item = null;
	    int varSpace = 0;
	    
	    if ((list != null) && (size != null))
	    {
	        size[0] = 0.f;
	        endLine = lineIndex + lineSize;
	        sizeIdx = -1;
	        fileIdx = lineIndex;

	        while ((ptr = list.indexOf('\u0020', fileIdx)) != -1 && (ptr < endLine) && (varSpace < 3))
	        {
	            if ((list.charAt(ptr - 1) == '\u0020') && (list.charAt(ptr + 1) != '\u0020'))
	            {
	                varSpace++;
	                if ((list.charAt(0) == '-'))
	                {
	                    if ((varSpace == 3) && (sizeIdx == -1))
	                    {
	                        sizeIdx = ptr + 1;
	                        String subLine = list.substring(sizeIdx);
	                        Scanner scanner = new Scanner(subLine);
	                        try
	                        {
	                        	size[0] = scanner.nextDouble();
	                        }
	                        catch (InputMismatchException e)
	                        {
	                        	size[0] = 0.f;
	                        }
	                        catch (IllegalStateException e) 
	                        {
	                        	size[0] = 0.f;
	                        }
	                        catch (NoSuchElementException e)
	                        {
	                        	size[0] = 0.f;
	                        }
	                        scanner.close();
	                        item = subLine;
	                    }
	                }
	            }
	            fileIdx = ++ptr;
	        }
	    }
	    
	    return item;
	}

}
