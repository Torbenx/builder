package com.tt.engine;

import android.app.Activity;
import android.os.Bundle;
import android.widget.Toast;
import android.view.SurfaceHolder;
import android.os.Handler;
import android.opengl.EGLDisplay;
import android.opengl.EGLConfig;
import android.opengl.EGLSurface;
import android.opengl.EGLContext;
import android.graphics.PixelFormat;
import android.graphics.Canvas;
import android.view.View;
import static android.opengl.EGL14.*;

public class MainActivity extends Activity implements SurfaceHolder.Callback2,Runnable{
    private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
	
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
	
	@Override
	public void onCreate(Bundle saveInstaceState){
		super.onCreate(saveInstaceState);
		handler = new Handler();
		getWindow().takeSurface(this);
		getWindow().setFormat(PixelFormat.RGBA_8888);
	}
	
	@Override
	public void surfaceCreated(final SurfaceHolder holder){
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
			EGL_DEPTH_SIZE, 0,
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
		handler.post(this);
		nativeCreate();
	}
	
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height){
		nativeChange(width, height);
	}
	
	@Override
	public void run(){
		nativeDrawFrame();
		eglSwapBuffers(display, surface);
		handler.postDelayed(this, 1000 / 60);
	}
	
	@Override
	public void surfaceDestroyed(SurfaceHolder holder){
		
	}
	
	@Override
	public void surfaceRedrawNeeded(SurfaceHolder holder){
		
	}
	
	public void surfaceRedrawNeededAsync(SurfaceHolder holder, Runnable drawingFinished){
		
	}
	
	public native void nativeCreate();
	public native void nativeChange(int width, int height);
	public native void nativeDrawFrame();
}