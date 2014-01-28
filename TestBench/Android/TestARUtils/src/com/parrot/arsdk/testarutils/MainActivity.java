package com.parrot.arsdk.testarutils;

import java.io.File;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import com.parrot.arsdk.arutils.ARUTILS_ERROR_ENUM;
import com.parrot.arsdk.arutils.ARUTILS_FTP_RESUME_ENUM;
import com.parrot.arsdk.arutils.ARUTILS_HTTPS_PROTOCOL_ENUM;
import com.parrot.arsdk.arutils.ARUtilsException;
import com.parrot.arsdk.arutils.ARUtilsFileSystem;
import com.parrot.arsdk.arutils.ARUtilsFtpConnection;
import com.parrot.arsdk.arutils.ARUtilsFtpProgressListener;
import com.parrot.arsdk.arutils.ARUtilsHttpConnection;
import com.parrot.arsdk.arutils.ARUtilsHttpProgressListener;

public class MainActivity extends Activity implements ARUtilsFtpProgressListener, ARUtilsHttpProgressListener
{
	public static String APP_TAG = "TestARUtils "; 
    
	public static String DRONE_IP = "172.20.5.117";
	//public static String DRONE_IP = "192.168.1.1";
	public static int DRONE_PORT = 21;
	
	@SuppressWarnings("serial")
	public class TestException extends Exception
    {
		public TestException()
		{
			super("\n=============== ASSERT TestException ==============");
		}
    }
        
    public void assertError(boolean status) throws TestException
	{
    	if (false == status)
    	{
    		throw new TestException();
    	}
	}  
    
	@Override
	protected void onCreate(Bundle savedInstanceState) 
	{
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		Log.d("DBG", APP_TAG + "onCreate");
        
        LoadModules(true);
        //LoadModules(false);
		
		Button testFileSystem = (Button)this.findViewById(R.id.testFileSystem);        
        testFileSystem.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				TestFileSystem();				
			}
		});
        
		Button testFtp = (Button)this.findViewById(R.id.testFtp);        
        testFtp.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				TestFtp();				
			}
		});
        
        Button testHttp = (Button)this.findViewById(R.id.testHttp);        
        testHttp.setOnClickListener(new OnClickListener() {
			@Override
			public void onClick(View v) {
				TestHttp();				
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) 
	{
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}
	
	private void LoadModules(boolean debug)
    {
    	try
        {
    		if (debug == false)
    		{
    		    System.loadLibrary("curl");
    			System.loadLibrary("arsal");
    			System.loadLibrary("arsal_android");
    			System.loadLibrary("arutils");
    			System.loadLibrary("arutils_android");    			
    		}
    		else
    		{
    		    System.loadLibrary("curl");
    		    //System.loadLibrary("curl_dbg");
    			System.loadLibrary("arsal_dbg");
    			System.loadLibrary("arsal_android_dbg");
    			System.loadLibrary("arutils_dbg");
    			System.loadLibrary("arutils_android_dbg");    			
    		}
        }
        catch (Exception e)
        {
        	Log.d("DBG", APP_TAG + e.toString());
        }
        catch (Throwable e)
        {
        	Log.d("DBG", APP_TAG + e.toString());
        }
    }
	
	public void TestFileSystem()
	{
		try
		{
			ARUtilsFileSystem fs = new ARUtilsFileSystem();
    		File sysHome = this.getFilesDir();// /data/data/com.example.tstdata/files
            String tmp = sysHome.getAbsolutePath();    		
    		
            /* FileSystem */
    		try { 
    			long size = fs.getFileSize("a.txt");
	        	Log.d("DBG", "getFileSize ERROR" + size); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "getFileSize " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FILE_NOT_FOUND ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FILE_NOT_FOUND);
	        }
    		
    		try { 
    			fs.rename("a.txt", "b.txt");
	        	Log.d("DBG", "rename ERROR"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "rename " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_SYSTEM ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_SYSTEM);
	        }
    		
    		try { 
    			fs.removeFile("a.txt");
	        	Log.d("DBG", "removeFile ERROR"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "removeFile " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_SYSTEM ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_SYSTEM);
	        }
    		
    		try { 
    			fs.removeDir(tmp + "/zzz");
	        	Log.d("DBG", "removeDir ERROR"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "removeDir " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FILE_NOT_FOUND ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FILE_NOT_FOUND);
	        }
    		
    		try { 
    			double space = fs.getFreeSpace(tmp);
	        	Log.d("DBG", "getFreeSpace OK " + space); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "getFreeSpace " + (e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }
		}
		catch (Exception e)
        {
        	Log.d("DBG", APP_TAG + e.toString());
        }
        catch (Throwable e)
        {
        	Log.d("DBG", APP_TAG + e.toString());
        }
	}
	
	public void TestFtp()
	{
		try
		{
			ARUtilsFtpConnection connection = new ARUtilsFtpConnection();
			File sysHome = this.getFilesDir();// /data/data/com.example.tstdata/files
            String tmp = sysHome.getAbsolutePath();
            
    		try {
    			connection.createFtpConnection(DRONE_IP, DRONE_PORT, ARUtilsFtpConnection.FTP_ANONYMOUS, "");
    			Log.d("DBG", "createFtpConnection OK");
    		} catch (ARUtilsException e) { 
	        	Log.d("DBG", "createFtpConnection " + (e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }
    		
    		try {
    			String list = connection.list("zzz");
    			Log.d("DBG", "list ERROR " + list); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "list " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE);
	        }
    		
    		try {
    			String list = connection.list("medias");
    			Log.d("DBG", "list OK " + list); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "list " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }
    		
    		try {
    			connection.rename("a.txt", "b.txt");
    			Log.d("DBG", "rename ERROR"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "rename " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE);
	        }
    		
    		try {
    			double size = connection.size("a.txt");
    			Log.d("DBG", "size ERROR" + size); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "size " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE);
	        }

    		try {
    			double size = connection.size("medias/thumbnail_video_20131001_235901.jpg");
    			Log.d("DBG", "size OK" + size); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "size " + (e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }

    		
    		try {
    			connection.delete("a.txt");
    			Log.d("DBG", "delete ERROR"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "delete " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE);
	        }

    		try {
    			connection.get("a.txt", tmp + "/a.txt", this, this, ARUTILS_FTP_RESUME_ENUM.FTP_RESUME_FALSE);
    			Log.d("DBG", "get ERROR"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "get " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE);
	        }
    		
    		try {
    			connection.get("medias/thumbnail_video_20131001_235901.jpg", tmp + "/thumbnail_video_20131001_235901.jpg", this, this, ARUTILS_FTP_RESUME_ENUM.FTP_RESUME_FALSE);
    			Log.d("DBG", "get OK"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "get " + (e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }    		
    		
    		try {
    			byte[] data = connection.getWithBuffer("a.txt", this, this);
    			Log.d("DBG", "delete ERROR" + (data != null ? data.length : "null")); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "delete " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CODE);
	        }
    		
    		try {
    			byte[] data = connection.getWithBuffer("medias/thumbnail_video_20131001_235901.jpg", this, this);
    			Log.d("DBG", "getWithBuffer OK " + (data != null ? data.length : "null")); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "getWithBuffer " + (e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }    		

    		try {
    			connection.put("a.txt", tmp + "/a.txt", this, this, ARUTILS_FTP_RESUME_ENUM.FTP_RESUME_FALSE);
    			Log.d("DBG", "put ERROR"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "put " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FILE_NOT_FOUND ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FILE_NOT_FOUND);
	        }
    		
    		ARUTILS_ERROR_ENUM result = connection.cancel();
    		Log.d("DBG", "cancel " + (result == ARUTILS_ERROR_ENUM.ARUTILS_OK ? "OK" : "ERROR"));
    		assertError(result == ARUTILS_ERROR_ENUM.ARUTILS_OK);
    		
    		result = connection.isCanceled();
    		Log.d("DBG", "isCanceled " + (result == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CANCELED ? "OK" : "ERROR"));
    		assertError(result == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_FTP_CANCELED);
    		
			connection.closeFtpConnection();
			Log.d("DBG", "closeFtpConnection OK"); 
		}
		catch (Exception e)
        {
        	Log.d("DBG", APP_TAG + e.toString());
        }
        catch (Throwable e)
        {
        	Log.d("DBG", APP_TAG + e.toString());
        }
	}

	public void TestHttp()
	{
		try
		{
			ARUtilsHttpConnection connection = new ARUtilsHttpConnection();
			File sysHome = this.getFilesDir();// /data/data/com.example.tstdata/files
            String tmp = sysHome.getAbsolutePath();
            
    		try {
    			connection.createHttpConnection(DRONE_IP, ARUtilsHttpConnection.HTTP_PORT, ARUTILS_HTTPS_PROTOCOL_ENUM.HTTPS_PROTOCOL_FALSE, null, null);
    			//connection.createHttpConnection(DRONE_IP, ARUtilsHttpConnection.HTTP_PORT, ARUTILS_HTTPS_PROTOCOL_ENUM.HTTPS_PROTOCOL_FALSE, "df", "df");
    			//connection.createHttpConnection(DRONE_IP, ARUtilsHttpConnection.HTTPS_PORT, ARUTILS_HTTPS_PROTOCOL_ENUM.HTTPS_PROTOCOL_TRUE, null, null);
    			//connection.createHttpConnection(DRONE_IP, ARUtilsHttpConnection.HTTPS_PORT, ARUTILS_HTTPS_PROTOCOL_ENUM.HTTPS_PROTOCOL_TRUE, "df", "df");
    			Log.d("DBG", "createHttpConnection OK");
    		} catch (ARUtilsException e) { 
	        	Log.d("DBG", "createFtpConnection " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }
    		
    		try {
    			connection.get("a.txt", tmp + "/a.txt", this, this);
    			Log.d("DBG", "get ERROR"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "get " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_HTTP_CODE ? "OK" : "ERROR"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_HTTP_CODE);
	        }
            
    		try {
    			connection.get("video_20131001_235901.mp4", tmp + "/video_20131001_235901.mp4", this, this);
    			//connection.get("private/video_20131001_235901.mp4", tmp + "/video_20131001_235901.mp4", this, this);
    			Log.d("DBG", "get OK"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "get " + (e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }    		

    		
    		ARUTILS_ERROR_ENUM result = connection.cancel();
    		Log.d("DBG", "cancel " + (result == ARUTILS_ERROR_ENUM.ARUTILS_OK ? "OK" : "ERROR"));
    		assertError(result == ARUTILS_ERROR_ENUM.ARUTILS_OK);
    		
    		result = connection.isCanceled();
    		Log.d("DBG", "isCanceled " + (result == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_HTTP_CANCELED ? "OK" : "ERROR"));
    		assertError(result == ARUTILS_ERROR_ENUM.ARUTILS_ERROR_HTTP_CANCELED);
    		
			connection.closeHttpConnection();
			Log.d("DBG", "closeHttpConnection OK");
			
    		try {
    			connection.createHttpConnection(DRONE_IP, ARUtilsHttpConnection.HTTP_PORT, ARUTILS_HTTPS_PROTOCOL_ENUM.HTTPS_PROTOCOL_FALSE, "parrot", "parrot");
    			Log.d("DBG", "createHttpConnection OK");
    		} catch (ARUtilsException e) { 
	        	Log.d("DBG", "createFtpConnection " + (e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }
    		
    		try {
    			connection.get("private/video_20131001_235901.mp4", tmp + "/video_20131001_235901.mp4", this, this);
    			Log.d("DBG", "get OK"); 
	        } catch (ARUtilsException e) { 
	        	Log.d("DBG", "get " + (e.getError() != ARUTILS_ERROR_ENUM.ARUTILS_OK ? "ERROR" : "OK"));
	        	assertError(e.getError() == ARUTILS_ERROR_ENUM.ARUTILS_OK);
	        }    		
    		
    		connection.closeHttpConnection();
			Log.d("DBG", "closeHttpConnection OK");
		}
        catch (Exception e)
        {
        	Log.d("DBG", APP_TAG + e.toString());
        }
        catch (Throwable e)
        {
        	Log.d("DBG", APP_TAG + e.toString());
        }
	}

	public void didFtpProgress(Object arg, int percent)
    {
    	Log.d("DBG", APP_TAG + "ARUtils Ftp/Http Connection, didProgress: " + percent + "%");
    }
	
	public void didHttpProgress(Object arg, int percent)
    {
    	Log.d("DBG", APP_TAG + "ARUtils Ftp/Http Connection, didProgress: " + percent + "%");
    }
}
