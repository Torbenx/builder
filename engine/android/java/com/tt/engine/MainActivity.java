package com.tt.engine;

import android.app.Activity;
import android.os.Bundle;
import android.view.SurfaceHolder;
import android.os.Handler;
import android.opengl.EGLDisplay;
import android.opengl.EGLConfig;
import android.opengl.EGLSurface;
import android.opengl.EGLContext;
import android.graphics.PixelFormat;
import android.view.View;
import android.content.res.Configuration;
import android.util.Log;
import android.util.DisplayMetrics;
import static android.opengl.EGL14.*;

public class MainActivity extends Activity implements SurfaceHolder.Callback2,Runnable{
    static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
	
	static{
		System.loadLibrary("c++_shared");
		System.loadLibrary("engine");
		System.loadLibrary("main");
	}
	
	EGLDisplay display;
	EGLSurface surface;
	EGLContext context;
	EGLConfig config;
	Handler handler;
	View view;
	boolean running = false;
	
	@Override
	public void onCreate(Bundle saveInstaceState){
		super.onCreate(saveInstaceState);
		handler = new Handler();
		getWindow().takeSurface(this);
		getWindow().setFormat(PixelFormat.RGBA_8888);
		view = new View(this);
		setContentView(view);
		view.setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
	}
	
	@Override
	public void onConfigurationChanged(Configuration config){
		super.onConfigurationChanged(config);
		DisplayMetrics m = new DisplayMetrics();
		getWindowManager().getDefaultDisplay().getMetrics(m);
		int width = m.widthPixels;
		int height = m.heightPixels;
		Log.d("MainActivity","onConfigurationChanged, width: "+width+", height: "+height);
		nativeChange(width,height);
	}
	
	@Override
	public void surfaceCreated(final SurfaceHolder holder){
		Log.d("MainActivity","surfaceCreated");
		holder.setFormat(PixelFormat.RGBA_8888);
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		eglInitialize(display, null, 0, null, 0);
		EGLConfig[] configs = new EGLConfig[1];
		int[] num_configs = new int[1];
		eglChooseConfig(display, new int[]{
			EGL_RED_SIZE, 8,
			EGL_GREEN_SIZE, 8,
			EGL_BLUE_SIZE, 8,
			EGL_ALPHA_SIZE, 8,
			EGL_DEPTH_SIZE, 16,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE
		}, 0, configs, 0, configs.length, num_configs, 0);
		System.out.println("num: "+num_configs[0]);
		config = configs[0];
		surface = eglCreateWindowSurface(display, config, holder.getSurface(), new int[]{EGL_NONE}, 0);
		context = eglCreateContext(display, config, EGL_NO_CONTEXT, new int[]{EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE}, 0);
		eglMakeCurrent(display, surface, surface, context);
		if(display == EGL_NO_DISPLAY) System.out.println("bad display");
		if(surface == EGL_NO_SURFACE) System.out.println("bad surface");
		if(context == EGL_NO_CONTEXT) System.out.println("bad context");
		nativeCreate();
	}
	
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height){
		Log.d("MainActivity", "surfaceChanged, format: "+format+", width: "+width+", height: "+height);
		if(format == PixelFormat.RGBA_8888){
			if(surface == EGL_NO_SURFACE){
				surface = eglCreateWindowSurface(display, config, holder.getSurface(), new int[]{EGL_NONE}, 0);
				eglMakeCurrent(display, surface, surface, context);
			}
			nativeChange(width, height);
			if(!running){
				handler.post(this);
				running = true;
			}
		}else{
			handler.removeCallbacks(this);
			running = false;
			/*eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
			if(surface != EGL_NO_SURFACE){
				eglDestroySurface(display, surface);
				surface = EGL_NO_SURFACE;
			}
			getWindow().setFormat(PixelFormat.RGBA_8888);*/
		}
	}
	
	@Override
	public void run(){
		nativeDrawFrame();
		eglSwapBuffers(display, surface);
		handler.postDelayed(this, 1000 / 60);
	}
	
	@Override
	public void surfaceDestroyed(SurfaceHolder holder){
		Log.d("MainActivity", "surfaceDestroyed");
		handler.removeCallbacks(this);
		running = false;
		nativeDestroy();
		eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(display, context);
		if(surface != EGL_NO_SURFACE){
			eglDestroySurface(display, surface);
		}
		eglTerminate(display);
		context = EGL_NO_CONTEXT;
		surface = EGL_NO_SURFACE;
		display = EGL_NO_DISPLAY;
	}
	
	@Override
	public void surfaceRedrawNeeded(SurfaceHolder holder){
		
	}
	
	public void surfaceRedrawNeededAsync(SurfaceHolder holder, Runnable drawingFinished){
		
	}
	
	native void nativeCreate();
	native void nativeChange(int width, int height);
	native void nativeDrawFrame();
	native void nativeDestroy();
}