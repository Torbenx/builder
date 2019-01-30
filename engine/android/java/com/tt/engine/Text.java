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
    
    public static StaticLayout createTextFormat(String text, int maxWidth, float textSize){
        TextPaint paint = new TextPaint();
        paint.setAntiAlias(true);
        paint.setTextSize(textSize);
        return new StaticLayout(text, paint, maxWidth < 0 ? Integer.MAX_VALUE : maxWidth, Layout.Alignment.ALIGN_NORMAL, 1.0f, 0.0f, false);
    }
    
    public static float getTextFormatWidth(StaticLayout layout){
        float width = 0;
        for(int i=0;i<layout.getLineCount();i++){
            float w = layout.getLineWidth(i);
            if(width < w){
                width = w;
            }
        }
        return width;
    }
    
    public static float getTextFormatHeight(StaticLayout layout){
        return layout.getHeight();
    }
    
    public static int getTextFormatLineCount(StaticLayout layout){
        return layout.getLineCount();
    }
    
    private SurfaceTexture surfaceTexture;
    private Surface surface;
    private float[] matrix = new float[16];
    
    public Text(int glId, int width, int height){
        surfaceTexture = new SurfaceTexture(glId, true);
        surfaceTexture.setDefaultBufferSize(width, height);
        surface = new Surface(surfaceTexture);
        surfaceTexture.updateTexImage();
    }
    
    public float[] drawText(StaticLayout format, float posX, float posY, float red, float green, float blue, float alpha, boolean clear){
        format.getPaint().setARGB((int)(alpha * 255.0f), (int)(red * 255.0f), (int)(green * 255.0f), (int)(blue * 255.0f));
        
        surfaceTexture.releaseTexImage();
        
        Canvas canvas = surface.lockCanvas(null);
        if(clear){
            canvas.drawColor(0, PorterDuff.Mode.CLEAR);
        }
        canvas.translate(posX, posY);
        format.draw(canvas);
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