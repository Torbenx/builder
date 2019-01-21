package com.tt.engine;

import android.graphics.SurfaceTexture;
import android.graphics.Canvas;
import android.graphics.PorterDuff;
import android.view.Surface;
import android.util.Log;
import android.text.TextPaint;
import android.text.StaticLayout;
import android.text.Layout;

public class Text{
    private SurfaceTexture surfaceTexture;
    private Surface surface;
    private TextPaint paint;
    private float[] matrix = new float[16];
    
    public Text(int glId, int width, int height){
        surfaceTexture = new SurfaceTexture(glId, true);
        surfaceTexture.setDefaultBufferSize(width, height);
        surface = new Surface(surfaceTexture);
        paint = new TextPaint();
        paint.setAntiAlias(true);
        surfaceTexture.updateTexImage();
    }
    
    public float[] drawText(String text, int maxWidth, float textSize, float red, float green, float blue, float alpha){
        paint.setTextSize(textSize);
        paint.setARGB((int)(alpha * 255.0f), (int)(red * 255.0f), (int)(green * 255.0f), (int)(blue * 255.0f));
        StaticLayout layout = new StaticLayout(text, paint, maxWidth < 0 ? Integer.MAX_VALUE : maxWidth, Layout.Alignment.ALIGN_NORMAL, 1.0f, 0.0f, false);
        
        surfaceTexture.releaseTexImage();
        
        Canvas canvas = surface.lockCanvas(null);
        canvas.drawColor(0, PorterDuff.Mode.CLEAR);
        layout.draw(canvas);
        surface.unlockCanvasAndPost(canvas);
        
        surfaceTexture.updateTexImage();
        surfaceTexture.getTransformMatrix(matrix);
        return matrix;
    }
    
    public void release(){
        surface.release();
        surfaceTexture.release();
    }
}