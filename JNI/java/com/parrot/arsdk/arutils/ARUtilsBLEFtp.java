package com.parrot.arsdk.arutils;

import java.io.BufferedInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.UUID;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;

import com.parrot.arsdk.arsal.ARSALBLEManager;
import com.parrot.arsdk.arsal.ARSALBLEManager.ARSALManagerNotificationData;
import com.parrot.arsdk.arsal.ARSALPrint;
import com.parrot.arsdk.arsal.ARSAL_ERROR_ENUM;

public class ARUtilsBLEFtp 
{

	private static final String TAG = "ARUtilsBLEFtp";
	
	public final static String BLE_GETTING_KEY = "kARUTILS_BLEFtp_Getting";
	
	public final static String BLE_PACKET_WRITTEN = "FILE WRITTEN";
	public final static String BLE_PACKET_NOT_WRITTEN = "FILE NOT WRITTEN";
	public final static String BLE_PACKET_EOF = "End of Transfer";
	public final static int BLE_PACKET_MAX_SIZE = 132;
	public final static int BLE_PACKET_BLOCK_PUTTING_COUNT = 500;
	public final static int BLE_PACKET_BLOCK_GETTING_COUNT = 100;
	
	public final static long BLE_PACKET_WRITE_SLEEP = 20;

	//private native static void nativeJNIInit();
	
	private ARSALBLEManager bleManager = null;
	private BluetoothGatt gattDevice = null;
	
	private BluetoothGattCharacteristic transferring = null;
	private BluetoothGattCharacteristic getting = null;
	private BluetoothGattCharacteristic handling = null;
	ArrayList<BluetoothGattCharacteristic> arrayGetting = null;
	
	private int port;

	/*static
    {
        nativeJNIInit();
    }*/
	
	public void initWithDevice(ARSALBLEManager bleManager, BluetoothGatt gattDevice, int port)
	{
		this.bleManager = bleManager;
		this.gattDevice = gattDevice;
		this.port = port;
	}
	
	public boolean registerCharacteristics()
	{
		List<BluetoothGattService> services = gattDevice.getServices();
		ARSAL_ERROR_ENUM error = ARSAL_ERROR_ENUM.ARSAL_OK;
		BluetoothGattService gettingService = null;
		boolean ret = true;
		
		ARSALPrint.d("DBG", TAG + "registerCharacteristics");
		
		Iterator<BluetoothGattService> servicesIterator = services.iterator();
		while (servicesIterator.hasNext())
		{
			BluetoothGattService service = servicesIterator.next();
			String serviceUuid = getShortUuid(service.getUuid());	
			String name = service.getUuid().toString();
			ARSALPrint.d("DBG", TAG + "service " + name);
			
			if (serviceUuid.compareTo(String.format("fd%02d", this.port)) == 0)
			{
				List<BluetoothGattCharacteristic> characteristics = service.getCharacteristics();
				Iterator<BluetoothGattCharacteristic> characteristicsIterator = characteristics.iterator();
				
				while (characteristicsIterator.hasNext())
				{
					BluetoothGattCharacteristic characteristic = characteristicsIterator.next();
					String characteristicUuid = getShortUuid(characteristic.getUuid());
					ARSALPrint.d("DBG", TAG + "characteristic " + characteristicUuid);
					
					if (characteristicUuid.compareTo(String.format("fd%02d", this.port + 1)) == 0)
					{
						this.transferring = characteristic;
					}
					else if (characteristicUuid.compareTo(String.format("fd%02d", this.port + 2)) == 0)
					{
						gettingService = service;
						this.arrayGetting = new ArrayList<BluetoothGattCharacteristic>();
						this.arrayGetting.add(characteristic);
						this.getting = characteristic;
						
						ARSALPrint.d("DBG", TAG + "set " + error.toString());
						
					}
					else if (characteristicUuid.compareTo(String.format("fd%02d", this.port + 3)) == 0)
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
				ARSALPrint.d("DBG", TAG + "set " + error.toString());
				ret = false;
			}*/
			
			if (ret == true)
			{
				bleManager.registerNotificationCharacteristics(this.arrayGetting, BLE_GETTING_KEY);
			}
		}
		
		return ret;
	}
	
	private String getShortUuid(UUID uuid)
	{
		String shortUuid = uuid.toString().substring(4, 8);		
		return shortUuid;
	}
	
	public boolean sizeFile(String remoteFile, int[] totalSize)
	{
		totalSize[0] = 0;
		return true;
	}
	
	public boolean listFiles(String remotePath)
	{
		StringBuffer[] list = new StringBuffer[1];
		list[0] = new StringBuffer();
		boolean ret = true;
		
		ARSALPrint.d("DBG", TAG + "listFiles");
		
		ret = sendCommand("LIS", remotePath, handling);
		
		if (ret == true)
		{
			ret = readDataList(list);
		}
		
		return ret;
	}
	
	public boolean getFileInternal(String remoteFile, String localFile, byte[][] data, int[] dataLen, ARUtilsFtpProgressListener progressListener, Object progressArg)
	{
		FileOutputStream dst = null;
		boolean ret = true;
		int[] totalSize = new int[1];
		totalSize[0] = 0;
		
		ARSALPrint.d("DBG", TAG + "getFile");
		
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
				ret = false;
			}
		}
		
		if (ret == true)
		{
			ret = readGetData(totalSize[0], dst, data, dataLen, progressListener, progressArg);
		}
		
		if (dst != null)
		{
			try
			{
				dst.close();
			}
			catch(IOException e)
			{
			}
		}
		
		return ret;
	}
	
	public boolean getFile(String remotePath, String localFile, ARUtilsFtpProgressListener progressListener, Object progressArg)
	{
		boolean ret = false;
		
		ret = getFileInternal(remotePath, localFile, null, null, progressListener, progressArg);
		
		return ret;
	}
	
	public boolean getFileWithBuffer(String remotePath, byte[][] data, int[] dataLen, ARUtilsFtpProgressListener progressListener, Object progressArg)
	{
		boolean ret = false;
		
		ret = getFileInternal(remotePath, null, data, dataLen, progressListener, progressArg);
		
		return ret;
	}
	
	public boolean abortPutFile(String remotePath)
	{
		int[] resumeIndex = new int[1];
		resumeIndex[0] = 0;
		boolean resume = false;
		boolean ret = true;
		
		ret = readPutResumeIndex(resumeIndex);
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
				ret = sendPutData(0, null, resumeIndex[0], false, true, null, null);
			}
		}
		
		return ret;
	}
	
	public boolean putFile(String remotePath, String localFile, ARUtilsFtpProgressListener progressListener, Object progressArg, boolean resume)
	{
		FileInputStream src = null;
		int[] resumeIndex = new int[1];
		resumeIndex[0] = 0;
		boolean ret = true;
		int totalSize = 0;
		
		ARSALPrint.d("DBG", TAG + "putFile");
		
		if (resume == true)
		{
			abortPutFile(remotePath);
		}
		else
		{
			if (ret == true)
			{
				ret = readPutResumeIndex(resumeIndex);
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
		}
	
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
			ret = sendPutData(totalSize, src, resumeIndex[0], resume, false, progressListener, progressArg);
		}
		
		if (src != null)
		{
			try
			{
				src.close();
			}
			catch(IOException e)
			{
			}
		}
		
		return ret;
	}
	
	public boolean deleteFile(String remoteFile)
	{
		boolean ret = true;
		
		ret = sendCommand("DEL", remoteFile, handling);
		
		return ret;
	}
	
	public boolean renameFile(String oldNamePath, String newNamePath)
	{
		boolean ret = true;
		String param = oldNamePath + " " + newNamePath;
		
		ret = sendCommand("REN", param, handling);
		
		return ret;
	}
	
	private boolean sendCommand(String cmd, String param, BluetoothGattCharacteristic characteristic)
	{
		boolean ret = true;
		byte[] paramBuff = null;
		byte[] buffer = null;//new byte[BLE_PACKET_MAX_SIZE];
		int len = 0;
		int index = 0;
		
		try
		{
			paramBuff = param.getBytes("UTF8");
		}
		catch (UnsupportedEncodingException e)
		{
			ret = false;
		}

		if (ret == true)
		{
			if ((cmd.length() + paramBuff.length + 1) > BLE_PACKET_MAX_SIZE)
			{
				ret = false;
			}
			else
			{
				buffer = new byte[cmd.length() + paramBuff.length + 1];
			}
		}
		
		for (int i=0; i<cmd.length(); i++)
		{
			buffer[i] = (byte)cmd.charAt(i);
			index++;
		}
		
		if (param != null)
		{
			System.arraycopy(paramBuff, 0, buffer, index, paramBuff.length);
			index += paramBuff.length;
			buffer[index] = 0;
		}
		
		if (ret == true)
		{
			bleManager.writeData(buffer, characteristic);
		}
		
		return ret;
	}
	
	private boolean sendPutData(int fileSize, FileInputStream src, int resumeIndex, boolean resume, boolean abort, ARUtilsFtpProgressListener progressListener, Object progressArg)
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
						
						Thread.sleep(BLE_PACKET_WRITE_SLEEP);
						send = buffer;
						if (packetLen != BLE_PACKET_MAX_SIZE)
						{
							send = new byte[packetLen];
							System.arraycopy(buffer, 0, send, 0, packetLen);
						}
						
						ret = bleManager.writeData(send, transferring);
						
						ARSALPrint.d("DBG", TAG + "packet " + packetCount);
					}
					else
					{
						ARSALPrint.d("DBG", TAG + "resume " + packetCount);
					}
					
					if (progressListener != null)
					{
						progressListener.didFtpProgress(progressListener, ((float)totalSize / (float)fileSize) * 100.f);
					}
					
					/*if (cancelSem != null)
					{
						ret = 
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
					
					if ((resume == false) && ((resume ==  true) && (packetCount > resumeIndex)))
					{
						md5Txt = md5.digest();
						md5.initialize();
				
						ARSALPrint.d("DBG", TAG + "sending md5 " + md5Txt);
						
						Thread.sleep(BLE_PACKET_WRITE_SLEEP);
						send = md5Txt.getBytes("UTF8");
						ret = bleManager.writeData(send, transferring);
						
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
				Thread.sleep(BLE_PACKET_WRITE_SLEEP);
				send = new byte[0];
				ret = bleManager.writeData(send, transferring);
				
				if (ret == true)
				{
					ret = readPutMd5(md5Msg);
				}
		
				if (ret == true)
				{
					md5Txt = md5End.digest();
					
					ARSALPrint.d("DBG", TAG + "md5 end" + md5Txt);
					
					if (md5Msg[0].compareTo(md5Txt) != 0)
					{
						ARSALPrint.d("DBG", TAG + "md5 end Failed");
						ret = false;
					}
					else
					{
						ARSALPrint.d("DBG", TAG + "md5 end ok");
					}
				}
			}
		}
		catch (InterruptedException e)
		{
			ARSALPrint.d("DBG", e.toString());
			ret = false;
		}
		catch (IOException e)
		{
			ARSALPrint.d("DBG", e.toString());
			ret = false;
		}
		
		return ret;
	}
	
	private boolean readPutResumeIndex(int[] resumeIndex)
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
					ARSALPrint.d("DBG", TAG + "resume index " + size);
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
	
	private boolean readPudDataWritten()
	{
		ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>(); 
		boolean ret = false;
		
		try
		{
			ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
			if (ret = true)
			{
				if (receivedNotifications.size() > 0)
				{
					ARSALManagerNotificationData  notificationData = receivedNotifications.get(0);
					byte[] packet = notificationData.value;
					int packetLen = notificationData.value.length;
					
					if (packetLen > 0)
					{
						String packetTxt = new String(packet, 0, packetLen, "UTF8");
						
						if (packetTxt.compareTo(BLE_PACKET_WRITTEN) == 0)
						{
							ARSALPrint.d("DBG", "Written OK");
							ret = true;
						}
						else if (packetTxt.compareTo(BLE_PACKET_NOT_WRITTEN) == 0)
						{
							ARSALPrint.d("DBG", "NOT Written");
							ret = false;
						}
						else
						{
							ARSALPrint.d("DBG", "UNKNOWN Written");
							ret = false;
						}
					}
				}
				else
				{
					ARSALPrint.d("DBG", "UNKNOWN Written");
					ret = false;
				}
			}
		}
		catch (UnsupportedEncodingException e)
		{
			ARSALPrint.d("DBG", e.toString());
			ret = false;
		}
		
		return ret;
	}
	
	private boolean readPutMd5(String[] md5Txt)
	{
		ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>(); 
		boolean ret = false;
		
		md5Txt[0] = "";
	
		try
		{
			ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
			if (ret == true)
			{
				if (receivedNotifications.size() > 0)
				{
					ARSALManagerNotificationData  notificationData = receivedNotifications.get(0);
					byte[] packet = notificationData.value;
					int packetLen = notificationData.value.length;
					
					if (packetLen > 0)
					{
						String packetTxt = new String(packet, 0, packetLen, "UTF8");
						md5Txt[0] = packetTxt.substring(3, packetTxt.length() - 3);
						
						ARSALPrint.d("DBG", "md5 end received " + md5Txt[0]);
					}
					else 
					{
						ARSALPrint.d("DBG", "md5 end failed");
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
			ARSALPrint.d("DBG", e.toString());
			ret = false;
		}
		
		return ret;
	}
	
	private boolean readGetData(int fileSize, FileOutputStream dst, byte[][] data, int[] dataLen, ARUtilsFtpProgressListener progressListener, Object progressArg)
	{
		ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>();
		//ArrayList<ARSALManagerNotificationData> removeNotifications = new ArrayList<ARSALManagerNotificationData>();
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
				if (receivedNotifications.size() == 0)
				{
					ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
				}
				if (ret == false)
				{
					ret = bleManager.isDeviceConnected();
				}
				else
				{
					for (int i=0; (i < receivedNotifications.size()) && (ret == true) && (blockMD5 == false) && (endMD5 == false); i++)
					{
						ARSALManagerNotificationData notificationData = receivedNotifications.get(i);
						int packetLen = notificationData.value.length;
						byte[] packet = notificationData.value;
						
						packetCount++;
						totalPacket++;
						
						if (packetLen > 0)
						{
							if (endFile == true)
							{
								endMD5 = true;
								
								if (packetLen == (ARUtilsMD5.MD5_LENGTH * 2))
								{
									md5Msg = ARUtilsMD5.getTextDigest(packet, 0, packetLen);
								}
								else
								{
									ret = false;
									ARSALPrint.d("DBG", "md5 end failed size " + packetLen);
								}
							}
							else if (compareToString(packet, packetLen, BLE_PACKET_EOF))
							{
								endFile = true;
								
								if (packetLen == BLE_PACKET_EOF.length())
								{
									ARSALPrint.d("DBG", "end of file received ");
								}
								else
								{
									ARSALPrint.d("DBG", "end of file failed size " + packetLen);
									ret = false;
								}
							}
							else if (compareToString(packet, packetLen, "MD5"))
							{
								if (packetCount > (BLE_PACKET_BLOCK_GETTING_COUNT + 1))
								{
									ARSALPrint.d("DBG", "md5 failed packet count " + packetCount);
								}
								
								if (packetLen == ((ARUtilsMD5.MD5_LENGTH * 2) + 3))
								{
									blockMD5 = true;
									md5Msg = ARUtilsMD5.getTextDigest(packet, 3, ARUtilsMD5.MD5_LENGTH);
									ARSALPrint.d("DBG", "md5 received " + packetCount);
								}
								else
								{
									ret = false;
									ARSALPrint.d("DBG", "md5 failed size " + packetLen);
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
										ARSALPrint.d("DBG", "failed writting file " + e.toString());
										ret = false;
									}
								}
								else
								{
									byte[] newData = new byte[totalSize];
									System.arraycopy(data[0], 0, newData, 0, totalSize - packetLen);
									System.arraycopy(packet, 0, newData, totalSize - packetLen, packetLen);
									data[0] = newData;
								}
								
								if (progressListener != null)
								{
									progressListener.didFtpProgress(progressArg, ((float)totalSize / (float)fileSize) * 100.f);
								}
								
								/*if (cancelSem != null)
								{
									ret = 
								}*/
							}
						}
						else
						{
							//empty âcket authorized
						}
					}
				}
				
				receivedNotifications.clear();
			}
			while ((ret == true) && (blockMD5 == false) && (endMD5 == false));
			
			if ((ret == true) && (blockMD5 == true))
			{
				blockMD5 = false;
				packetCount = 0;
				md5Txt = md5.digest();
				
				if (md5Msg.contentEquals(md5Txt) == false)
				{
					ARSALPrint.d("DBG", "md5 block failed ");
					//TOFIX some 1st md5 packet failed !!!!
					//ret = false;
				}
				else
				{
					ARSALPrint.d("DBG", "md5 block failed ");
				}
			}
			
			ret = sendCommand("MD5 OK", null, transferring);
		}
		
		if (endMD5 == true)
		{
			md5Txt = md5End.digest();
			ARSALPrint.d("DBG", "md5 end computed " + md5Txt);
			
			if (md5Msg.contentEquals(md5Txt) == false)
			{
				ARSALPrint.d("DBG", "md5 end failed ");
				ret = false;
			}
			else
			{
				ARSALPrint.d("DBG", "md5 end ok ");
			}
			
		}
		else
		{
			ret = false;
		}
		
		return ret;
	}
	
	private boolean readDataList(StringBuffer[] list)
	{
		ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>();
		//ArrayList<ARSALManagerNotificationData> removeNotifications = new ArrayList<ARSALManagerNotificationData>();
		boolean ret = true;
		int packetCount = 0;
		int totalSize = 0;
		int totalPacket = 0;
		boolean endFile = false;
		//boolean endMD5 = false;
		String md5Msg = null;
		String md5Txt = null;
		ARUtilsMD5 md5End = new ARUtilsMD5();
		
		try
		{
			while ((ret == true) && (endFile == false) /*&& (endMD5 == false)*/)
			{
				do
				{
					if (receivedNotifications.size() == 0)
					{
						ret = bleManager.readDataNotificationData(receivedNotifications, 1, BLE_GETTING_KEY);
					}
					if (ret == false)
					{
						ret = bleManager.isDeviceConnected();
					}
					else
					{
						for (int i=0; (i < receivedNotifications.size()) && (ret == true) && (endFile == false) /*&& (endMD5 == false)*/; i++)
						{
							ARSALManagerNotificationData notificationData = receivedNotifications.get(i);
							int packetLen = notificationData.value.length;
							byte[] packet = notificationData.value;
	
							packetCount++;
							
							if (packetLen > 0)
							{
								/*if (endFile == true)
								{
									endMD5 = true;
									
									if (packetLen == (ARUtilsMD5.MD5_LENGTH * 2))
									{
										md5Msg = ARUtilsMD5.getTextDigest(packet, 0, packetLen);
									}
									else
									{
										ret = false;
										ARSALPrint.d("DBG", "md5 end failed size " + packetLen);
									}
								}
								else*/ if (compareToString(packet, packetLen, BLE_PACKET_EOF))
								{
									endFile = true;
									
									if (packetLen == BLE_PACKET_EOF.length())
									{
										ARSALPrint.d("DBG", "end of file received ");
									}
									else
									{
										ARSALPrint.d("DBG", "end of file failed size " + packetLen);
										ret = false;
									}
								}
								else
								{
									totalSize += packetLen;
									md5End.update(packet, 0, packetLen);
	
									String str = new String(packet, 0, packetLen, "UTF8");
									list[0].append(str);
									ARSALPrint.d("DBG", "packet " + packetCount);
								}
							}
							else
							{
								endFile = true;
								ARSALPrint.d("DBG", "end received");
							}
						}
					}
					
					receivedNotifications.clear();
				}
				while ((ret == true) && (endFile == false) /*&& (endMD5 == false)*/);
			}
			
			if ((ret == true) && (endFile == false) /*&& (endMD5 == false)*/)
			{
				md5Txt = md5End.digest();
				
				if (md5Msg.contentEquals(md5Txt))
				{
					ARSALPrint.d("DBG", "md5 end failed");
					ret = false;
				}
				else
				{
					ARSALPrint.d("DBG", "md5 end ok");
				}
			
			}
			else
			{
				ret = false;
			}
		}
		catch (UnsupportedEncodingException e) 
		{
			ARSALPrint.d("DBG", e.toString());
			ret = false;
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
			ARSALPrint.d("DBG", e.toString());
			ret = false;
		}
		
		return ret;
	}
}
