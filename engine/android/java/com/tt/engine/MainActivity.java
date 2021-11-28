package com.tt.engine;

import android.app.Activity;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.view.View;
import android.view.SurfaceHolder;
import android.view.WindowManager;
import android.view.Surface;
import android.view.InputQueue;
import android.graphics.PixelFormat;
import android.graphics.Canvas;
import android.content.res.Configuration;
import android.content.res.AssetManager;
import android.util.Log;
import java.io.IOException;
import java.io.File;
import android.app.NativeActivity;

public class MainActivity extends NativeActivity implements Runnable{
    
    View view;
    boolean fullscreen = false;
    Thread thread;
    ParcelFileDescriptor read, write;
    
    @Override
    public void onCreate(Bundle saveInstaceState){
        super.onCreate(saveInstaceState);
        Log.d("MainActivity", "onCreate");

        String dir = getApplicationInfo().nativeLibraryDir;
        File dirFile = new File(dir);
        for(File libFile : dirFile.listFiles()){
            Log.d("MainActivity", "loading "+libFile.getName());
            System.load(libFile.getPath());
        }

        view = new View(this);
        setContentView(view);
        
        thread = new Thread(this, "engine");
        thread.start();
        try{
            synchronized(thread){
                thread.wait();
            }
        }catch(InterruptedException e){
            e.printStackTrace();
        }
        nativeCreate();
    }
    
    @Override
    public void run(){
        ParcelFileDescriptor[] fds;
        try{
            fds = ParcelFileDescriptor.createPipe();
        }catch(IOException e){
            throw new RuntimeException(e);
        }
        read = fds[0];
        write = fds[1];
        nativeThreadInit(read.getFd(), write.getFd());
        synchronized(thread){
            thread.notifyAll();
        }
        nativeThreadLoop();
    }
    
    public void applyWindowMode(){
        int flags = WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS;
        getWindow().setFlags(fullscreen ? flags : 0, flags);
        view.setSystemUiVisibility(fullscreen ? View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION : 0);
    }
    
    @Override
    public void onConfigurationChanged(Configuration config){
        super.onConfigurationChanged(config);
        applyWindowMode();
    }
    
    public void goFullscreen(){
        runOnUiThread(new Runnable(){
            @Override
            public void run(){
                fullscreen = true;
                applyWindowMode();
            }
        });
    }
    
    public void exitFullscreen(){
        runOnUiThread(new Runnable(){
            @Override
            public void run(){
                fullscreen = false;
                applyWindowMode();
            }
        });
    }
    
    native void nativeThreadInit(int readFd, int writeFd);
    native void nativeThreadLoop();
    native void nativeCreate();
}
