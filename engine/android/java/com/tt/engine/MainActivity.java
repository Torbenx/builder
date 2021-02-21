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

public class MainActivity extends Activity implements SurfaceHolder.Callback2, InputQueue.Callback, Runnable{
    
    static{
        //System.loadLibrary("c++_shared");
        System.loadLibrary("engine");
        System.loadLibrary("builder");
    }
    
    View view;
    AssetManager assetManager;
    boolean fullscreen = false;
    Thread thread;
    ParcelFileDescriptor read, write;
    int cmdFd;
    
    @Override
    public void onCreate(Bundle saveInstaceState){
        super.onCreate(saveInstaceState);
        Log.d("MainActivity", "onCreate");
        assetManager = getResources().getAssets();
        getWindow().takeSurface(this);
        getWindow().takeInputQueue(this);
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
        nativeCreate(cmdFd);
    }
    
    @Override
    public void run(){
        nativeGetRenderer();
        ParcelFileDescriptor[] fds;
        try{
            fds = ParcelFileDescriptor.createPipe();
        }catch(IOException e){
            throw new RuntimeException(e);
        }
        read = fds[0];
        write = fds[1];
        cmdFd = write.getFd();
        nativeThreadInit(assetManager, read.getFd());
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
    public void onStart(){
        super.onStart();
        Log.d("MainActivity", "onStart");
        nativeStart(cmdFd);
    }
    
    @Override
    public void onResume(){
        super.onResume();
        Log.d("MainActivity", "onResume");
        applyWindowMode();
        nativeResume(cmdFd);
    }
    
    @Override
    public void onConfigurationChanged(Configuration config){
        super.onConfigurationChanged(config);
        applyWindowMode();
    }
    
    @Override
    public void onPause(){
        super.onPause();
        Log.d("MainActivity", "onPause");
        nativePause(cmdFd);
    }
    
    @Override
    public void onStop(){
        super.onStop();
        Log.d("MainActivity", "onStop");
        nativeStop(cmdFd);
    }
    
    @Override
    public void onDestroy(){
        super.onDestroy();
        Log.d("MainActivity", "onDestroy");
        nativeDestroy(cmdFd);
    }
    
    @Override
    public void surfaceCreated(SurfaceHolder holder){
        Log.d("MainActivity","surfaceCreated");
        nativeSurfaceCreate(cmdFd, holder.getSurface());
    }
    
    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height){
        Log.d("MainActivity", "surfaceChanged format: "+format+", width: "+width+", height: "+height);
        nativeSurfaceChange(cmdFd, holder.getSurface(), width, height);
    }
    
    @Override
    public void surfaceDestroyed(SurfaceHolder holder){
        Log.d("MainActivity", "surfaceDestroyed");
        nativeSurfaceDestroy(cmdFd, holder.getSurface());
    }
    
    @Override
    public void surfaceRedrawNeeded(SurfaceHolder holder){
    }
    
    @Override
    public void surfaceRedrawNeededAsync(SurfaceHolder holder, Runnable drawingFinished){
        drawingFinished.run();
    }
    
    @Override
    public void onInputQueueCreated(InputQueue inputQueue){
        nativeCreateInputQueue(inputQueue);
    }
    
    @Override
    public void onInputQueueDestroyed(InputQueue inputQueue){
        nativeDestroyInputQueue(inputQueue);
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
    
    native void nativeGetRenderer();
    
    native void nativeThreadInit(AssetManager assetManager, int cmdFd);
    native void nativeThreadLoop();
    
    native void nativeCreate(int cmdFd);
    native void nativeStart(int cmdFd);
    native void nativeResume(int cmdFd);
    native void nativePause(int cmdFd);
    native void nativeStop(int cmdFd);
    native void nativeDestroy(int cmdFd);
    
    native void nativeSurfaceCreate(int cmdFd, Surface surface);
    native void nativeSurfaceChange(int cmdFd, Surface surface, int width, int height);
    native void nativeSurfaceDestroy(int cmdFd, Surface surface);
    
    native void nativeCreateInputQueue(InputQueue inputQueue);
    native void nativeDestroyInputQueue(InputQueue inputQueue);
}
