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
import android.view.Choreographer;
import android.view.MotionEvent;
import android.content.res.Configuration;
import android.content.res.AssetManager;
import android.util.Log;
import android.util.DisplayMetrics;
import static android.opengl.EGL14.*;

public class MainActivity extends Activity implements SurfaceHolder.Callback2,Choreographer.FrameCallback,View.OnTouchListener{
    static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
    static final int POINTER_EVENT_DOWN = 1;
    static final int POINTER_EVENT_MOVE = 2;
    static final int POINTER_EVENT_UP = 3;
	
	static{
		System.loadLibrary("c++_shared");
		System.loadLibrary("engine");
		System.loadLibrary("main");
	}
	
	EGLDisplay display = EGL_NO_DISPLAY;
	EGLSurface surface = EGL_NO_SURFACE;
	EGLContext context = EGL_NO_CONTEXT;
	EGLConfig config;
	Handler handler;
	View view;
    AssetManager assetManager;
    Choreographer choreographer;
    boolean callbackPosted = false;
    int format = PixelFormat.RGB_565;
	
	@Override
	public void onCreate(Bundle saveInstaceState){
		super.onCreate(saveInstaceState);
        Log.d("MainActivity", "onCreate");
		handler = new Handler();
        choreographer = Choreographer.getInstance();
        assetManager = getResources().getAssets();
		getWindow().takeSurface(this);
		view = new View(this);
        view.setOnTouchListener(this);
		setContentView(view);
		view.setSystemUiVisibility(View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
        
		getWindow().setFormat(format);
		display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
		eglInitialize(display, null, 0, null, 0);
		EGLConfig[] configs = new EGLConfig[1];
		int[] num_configs = new int[1];
        int red,green,blue,alpha;
        switch(format){
            case PixelFormat.RGBA_8888:
                red = green = blue = alpha = 8;
                break;
            case PixelFormat.RGB_565:
                red = 5; green = 6; blue = 5; alpha = 0;
                break;
            default:
                throw new UnsupportedOperationException("invalid format "+format);
        }
		eglChooseConfig(display, new int[]{
			EGL_RED_SIZE, red,
			EGL_GREEN_SIZE, green,
			EGL_BLUE_SIZE, blue,
			EGL_ALPHA_SIZE, alpha,
			EGL_DEPTH_SIZE, 16,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
			EGL_NONE
		}, 0, configs, 0, configs.length, num_configs, 0);
		System.out.println("num: "+num_configs[0]);
		config = configs[0];
		context = eglCreateContext(display, config, EGL_NO_CONTEXT, new int[]{EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE}, 0);
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
        nativeInit();
		nativeCreate(assetManager);
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
    public void onDestroy(){
        super.onDestroy();
        Log.d("MainActivity", "onDestroy");
		nativeDestroy();
        eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
		eglDestroyContext(display, context);
		eglTerminate(display);
		context = EGL_NO_CONTEXT;
		display = EGL_NO_DISPLAY;
    }
    
    @Override
    public boolean onTouch(View v, MotionEvent event){
        int action = event.getActionMasked();
        int ev;
        switch(action){
            case MotionEvent.ACTION_POINTER_DOWN:
            case MotionEvent.ACTION_DOWN:
                ev = POINTER_EVENT_DOWN;
                break;
            case MotionEvent.ACTION_MOVE:
                ev = POINTER_EVENT_MOVE;
                break;
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                ev = POINTER_EVENT_UP;
                break;
            default:
                Log.d("MainActivity", "unsupported touch action: "+action);
                return true;
        }
        int index = event.getActionIndex();
        int x = Math.round(event.getX(index));
        int y = Math.round(event.getY(index));
        int pointerId = event.getPointerId(index);
        nativeTouch(ev, pointerId, x, y);
        return true;
    }
	
	@Override
	public void surfaceCreated(final SurfaceHolder holder){
		Log.d("MainActivity","surfaceCreated");
		surface = eglCreateWindowSurface(display, config, holder.getSurface(), new int[]{EGL_NONE}, 0);
		eglMakeCurrent(display, surface, surface, context);
	}
	
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height){
		Log.d("MainActivity", "surfaceChanged, format: "+format+", width: "+width+", height: "+height);
		//if(format == this.format){
			nativeChange(width, height);
            if(!callbackPosted){
                choreographer.postFrameCallback(this);
                callbackPosted = true;
            }
		/*}else{
			getWindow().setFormat(format);
		}*/
	}
	
	@Override
	public void doFrame(long nanos){
        choreographer.postFrameCallback(this);
		nativeDrawFrame();
		eglSwapBuffers(display, surface);
	}
	
	@Override
	public void surfaceDestroyed(SurfaceHolder holder){
		Log.d("MainActivity", "surfaceDestroyed");
        choreographer.removeFrameCallback(this);
        callbackPosted = false;
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
		eglDestroySurface(display, surface);
        surface = EGL_NO_SURFACE;
	}
	
	@Override
	public void surfaceRedrawNeeded(SurfaceHolder holder){
		
	}
	
	public void surfaceRedrawNeededAsync(SurfaceHolder holder, Runnable drawingFinished){
		
	}
	
	native void nativeInit();
	native void nativeCreate(AssetManager assetManager);
	native void nativeChange(int width, int height);
	native void nativeDrawFrame();
    native void nativeTouch(int event, int pointerId, int x, int y);
	native void nativeDestroy();
}