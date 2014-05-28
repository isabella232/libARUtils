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

import com.parrot.arsdk.arsal.ARSAL_ERROR_ENUM;
import com.parrot.arsdk.arsal.ARSALBLEManager;
import com.parrot.arsdk.arsal.ARSALBLEManager.ARSALManagerNotificationData;

import com.parrot.arsdk.arsal.ARSALPrint;

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
	
	private ARSALBLEManager bleManager = null;
	private BluetoothGatt gattDevice = null;
	
	private BluetoothGattCharacteristic transferring = null;
	private BluetoothGattCharacteristic getting = null;
	private BluetoothGattCharacteristic handling = null;
	
	private int resumeIndex = 0;
	private String readMd5Txt = null;
	private StringBuffer readList = new StringBuffer();
	private int port;
	
	public void initWithDevice(ARSALBLEManager bleManager, BluetoothGatt gattDevice, int port)
	{
		this.bleManager = bleManager;
		this.gattDevice = gattDevice;
		this.port = port;
	}
	
	public boolean registerCharacteristics()
	{
		List<BluetoothGattService> list = gattDevice.getServices();
		ARSAL_ERROR_ENUM error = ARSAL_ERROR_ENUM.ARSAL_OK;
		boolean ret = true;
		
		ARSALPrint.d("DBG", TAG + "registerCharacteristics");
		
		Iterator<BluetoothGattService> iterator = list.iterator();
		while (iterator.hasNext())
		{
			BluetoothGattService service = iterator.next();
			String serviceUuid = getShortUuid(service.getUuid());	
			String name = service.getUuid().toString();
			ARSALPrint.d("DBG", TAG + "service " + name);
			
			if (serviceUuid.startsWith("fd"))
			{
				List<BluetoothGattCharacteristic> characteristics = service.getCharacteristics();
				Iterator<BluetoothGattCharacteristic> it = characteristics.iterator();
				
				while (it.hasNext())
				{
					BluetoothGattCharacteristic characteristic = it.next();
					String characteristicUuid = getShortUuid(characteristic.getUuid());
					ARSALPrint.d("DBG", TAG + "characteristic " + characteristicUuid);
					
					if (characteristicUuid.compareTo(String.format("fd%02d", this.port + 1)) == 0)
					{
						this.transferring = characteristic;
					}
					else if (characteristicUuid.compareTo(String.format("fd%02d", this.port + 2)) == 0)
					{
						error = bleManager.setCharacteristicNotification(service, characteristic);
						if (error != ARSAL_ERROR_ENUM.ARSAL_OK)
						{
							ARSALPrint.d("DBG", TAG + "set " + error.toString());
							ret = false;
						}
						else
						{
							ArrayList<BluetoothGattCharacteristic> characteristicsArray = new ArrayList<BluetoothGattCharacteristic>();
							characteristicsArray.add(characteristic);
							bleManager.registerNotificationCharacteristics(characteristicsArray, BLE_GETTING_KEY);
						}
						ARSALPrint.d("DBG", TAG + "set " + error.toString());
						this.getting = characteristic;
					}
					else if (characteristicUuid.compareTo(String.format("fd%02d", this.port + 3)) == 0)
					{
						this.handling = characteristic;
					}
				}
			}
		}
		
		return ret;
	}
	
	private String getShortUuid(UUID uuid)
	{
		String shortUuid = uuid.toString().substring(4, 8);		
		return shortUuid;
	}
	
	public boolean listFiles(String remotePath)
	{
		boolean ret = true;
		
		ARSALPrint.d("DBG", TAG + "listFiles");
		
		ret = sendCommand("LIS", remotePath, handling);
		
		if (ret == true)
		{
			ret = readDataList();
		}
		
		return ret;
	}
	
	public boolean getFile(String remotePath, String localFile)
	{
		FileOutputStream dst = null;
		boolean ret = true;
		
		ARSALPrint.d("DBG", TAG + "getFile");
		
		ret = sendCommand("GET", remotePath, handling);
		
		if (ret == true)
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
			ret = readGetData(dst);
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
	
	public boolean putFile(String remotePath, String localFile)
	{
		//BufferedOutputStream out = null;
		//FileOutputStream dst = null;
		FileInputStream src = null;
		boolean abort = false;
		boolean resume = true;
		boolean ret = true;
		
		ARSALPrint.d("DBG", TAG + "putFile");
		
		this.resumeIndex = 0;
		ret = readPutResumeIndex();
		if (ret == false)
		{
			ret = true;
			this.resumeIndex = 0;
			resume = false;
		}
		
		if (this.resumeIndex > 0)
		{
			resume = true;
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
			ret = sendPutData(src, resumeIndex, resume, abort);
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
	
	private boolean sendPutData(FileInputStream src, int resumeIndex, Boolean resume, boolean abort)
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
					this.readMd5Txt = null;
					ret = readPutMd5();
				}
		
				if (ret == true)
				{
					md5Txt = md5End.digest();
					
					ARSALPrint.d("DBG", TAG + "md5 end" + md5Txt);
					
					if (this.readMd5Txt.compareTo(md5Txt) != 0)
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
	
	private boolean readPutResumeIndex()
	{
		ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>(); 
		boolean ret = true;
		
		this.resumeIndex = 0;
		
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
					this.resumeIndex = size;
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
	
	private boolean readPutMd5()
	{
		ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>(); 
		boolean ret = false;
		
		this.readMd5Txt = null;
	
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
						this.readMd5Txt = packetTxt.substring(3, packetTxt.length() - 3);
						
						ARSALPrint.d("DBG", "md5 end received " + this.readMd5Txt);
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
	
	private boolean readGetData(FileOutputStream dst)
	{
		ArrayList<ARSALManagerNotificationData> receivedNotifications = new ArrayList<ARSALManagerNotificationData>();
		ArrayList<ARSALManagerNotificationData> removeNotifications = new ArrayList<ARSALManagerNotificationData>();
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
		
		try
		{
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
									dst.write(packet, 0, packetLen);
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
		}
		catch (IOException e)
		{
			ARSALPrint.d("DBG", e.toString());
			ret = false;
		}
		
		return ret;
	}
	
	private boolean readDataList()
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
		ARUtilsMD5 md5End = new ARUtilsMD5();
		
		readList = new StringBuffer();
		
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
									readList.append(str);
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
