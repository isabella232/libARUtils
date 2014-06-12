package com.parrot.arsdk.arutils;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import com.parrot.arsdk.arsal.ARSALPrint;

public class ARUtilsMD5 
{

	private static final String TAG = "ARUtilsMD5";

	public final static int MD5_LENGTH = 16;
	
	MessageDigest digest = null;
	
	public ARUtilsMD5()
	{
		initialize();
	}
	
	public boolean initialize()
	{
		boolean ret = true;
		try
		{
			digest = java.security.MessageDigest.getInstance("MD5");
		}
		catch (NoSuchAlgorithmException e)
		{
			ARSALPrint.d(TAG, e.toString());
			ret = false;
		}
		
		return ret;
	}
	
	public void update(byte[] buffer, int index, int len)
	{
		digest.update(buffer, index, len);
	}
	
	static public String getTextDigest(byte[] hash, int index, int len)
	{
		StringBuffer txt = new StringBuffer();
		
		for (int i=0; i<len; i++)
		{
			String val = String.format("%02x", hash[index + i] & 0x00FF);
			txt.append(val);
		}
		return txt.toString();
	}
	
	public String digest()
	{
		byte[] hash = digest.digest();
		return getTextDigest(hash, 0, hash.length);
	}
}
