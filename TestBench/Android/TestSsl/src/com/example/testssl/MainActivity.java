package com.example.testssl;


import java.io.File;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;

import com.parrot.arsdk.arsync.ARSyncException;
import com.parrot.arsdk.arsync.ARSyncMacgyverUploader;
import com.parrot.arsdk.arutils.ARUTILS_HTTPS_PROTOCOL_ENUM;
import com.parrot.arsdk.arutils.ARUtilsHttpConnection;
import com.parrot.arsdk.arutils.ARUtilsHttpProgressListener;

public class MainActivity extends Activity implements ARUtilsHttpProgressListener
{

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        
        Log.d("DBG", "onCreate");
        //LoadModules();
        
        Button testOpenssl = (Button)this.findViewById(R.id.buttonOpenssl);        
        testOpenssl.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                TestOpenssl();              
            }
        });
        
        Button buttonARSync = (Button)this.findViewById(R.id.buttonARSync);        
        buttonARSync.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                TestARSync();              
            }
        });

    }
    
    private void LoadModules()
    {
        Log.d("DBG", "LoadLibs");
        
        System.loadLibrary("z");
        //System.loadLibrary("openssl");
        System.loadLibrary("crypto");
        System.loadLibrary("ssl");
        
        System.loadLibrary("curl");
        System.loadLibrary("arsal");
        System.loadLibrary("arsal_android");
        System.loadLibrary("arutils");
        System.loadLibrary("arutils_android");
        
        System.loadLibrary("arsync");
        System.loadLibrary("arsync_android");
    }
    
    private void TestOpenssl()
    {
        Log.d("DBG", "TestOpenssl");
        
        try
        {
            LoadModules();
            
            //ARUtilsManager manager = new ARUtilsManager();
            //manager.
            
            File sysHome = this.getFilesDir();// /data/data/com.example.tstdata/files
            String tmp = sysHome.getAbsolutePath(); 
            Log.d("DBG", "TEMP " + tmp);
            
            ARUtilsHttpConnection connection = new ARUtilsHttpConnection();
            
            connection.createHttpConnection("www.google.com", 443, ARUTILS_HTTPS_PROTOCOL_ENUM.HTTPS_PROTOCOL_TRUE, null, null);
            String dstFile = tmp + "/get.html"; 
            connection.get("/?gws_rd=ssl", dstFile, this, this);
        }
        catch (Exception ex)
        {
            Log.d("DBG", ex.toString());
        }
        catch (Throwable ex)
        {
            Log.d("DBG", ex.toString());
        }
    }

    @Override
    public void didHttpProgress(Object arg, float percent)
    {
        Log.d("DBG", "didHttpProgress " + percent);
    }
    
    void TestARSync()
    {
        try
        {
            File sysHome = this.getFilesDir();// /data/data/com.example.tstdata/files
            String tmp = sysHome.getAbsolutePath(); 
            
            LoadModules();
            
            ARSyncMacgyverUploader manager = new ARSyncMacgyverUploader(tmp, "3.4.5", "android", "samsung", 500.f, 500.f);
            
            manager.pause();
            manager.resume();
            //manager.cancelThread();
            Runnable uploaderRunnable = manager.getUploaderRunnable();
            
            Thread thread = new Thread(uploaderRunnable);
            
            thread.start();
            //Thread.sleep(1000);
            Thread.sleep(10000);
            
            manager.cancelThread();
            thread.join();

            manager.dispose();
        }
        catch (ARSyncException ex)
        {
            Log.d("DBG", ex.toString());
        }
        catch (Exception ex)
        {
            Log.d("DBG", ex.toString());
        }
        catch (Throwable ex)
        {
            Log.d("DBG", ex.toString());
        }
    }
}
