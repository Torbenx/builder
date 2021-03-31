#include <jni.h>
#include "../Renderer.hpp"
#include "engine.hpp"
#include "../util/log.hpp"
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <EGL/egl.h>
#include <android/choreographer.h>
#include <android/input.h>

#define ENGINE_IDENT_CMD 1
#define ENGINE_IDENT_INPUT 2

#define CHECK_EGL(ret, fret, fun) if(ret == fret){LOGD(fun " failed: 0x%x", eglGetError());}
#define CHECK_EGL_BOOL(ret, fun) CHECK_EGL(ret, false, fun)

ALooper *looper;

Renderer* (*createRenderer)();
Renderer *renderer = nullptr;
int rendererState = RENDERER_STATE_INVISIBLE;
AChoreographer *choreographer;

void init_textureFormats();

void engine_setRenderer(Renderer* (*r)()){
    createRenderer = r;
}

void android_initThread(int cmdFd){
    looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    ALooper_acquire(looper);
    ALooper_addFd(looper, cmdFd, ENGINE_IDENT_CMD, ALOOPER_EVENT_INPUT, NULL, nullptr);
    choreographer = AChoreographer_getInstance();
}

struct FrameCallbackData{
    bool running;
    EGLDisplay *display;
    EGLSurface *surface;
};
void android_doFrame(long ignore, void *data){
    EGLDisplay *display = ((FrameCallbackData*)data)->display;
    EGLSurface *surface = ((FrameCallbackData*)data)->surface;
    if(rendererState != RENDERER_STATE_INVISIBLE && *surface != EGL_NO_SURFACE){
        AChoreographer_postFrameCallback(choreographer, android_doFrame, data);
        renderer->drawFrame();
        eglSwapBuffers(*display, *surface);
    }
    ((FrameCallbackData*)data)->running = false;
}

void android_runThread(){
    bool running = true;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig config;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;
    FrameCallbackData callbackData = {false, &display, &surface};
    ANativeWindow *window;

    init_textureFormats();

    while(running){
        int fd;
        int events;
        void *pollData;
        int ident = ALooper_pollOnce(-1, &fd, &events, &pollData);
        switch(ident){
            case ENGINE_IDENT_CMD:{
                cmd_t cmd;
                int readRet = read(fd, &cmd, sizeof(cmd_t));
                if(readRet == sizeof(cmd_t)){
                    switch(cmd){
                        case ENGINE_CMD_CREATE:{
                            display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
                            CHECK_EGL_BOOL(eglInitialize(display, nullptr, nullptr), "eglInitialize");
                            EGLint attrib_list[] = {
                                EGL_RED_SIZE, 8,
                                EGL_GREEN_SIZE, 8,
                                EGL_BLUE_SIZE, 8,
                                EGL_ALPHA_SIZE, 8,
                                EGL_DEPTH_SIZE, 16,
                                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                                EGL_NONE
                            };
                            int num_configs;
                            CHECK_EGL_BOOL(eglChooseConfig(display, attrib_list, &config, 1, &num_configs), "eglChooseConfig");
                            EGLint context_attrib_list[] = {
                                EGL_CONTEXT_CLIENT_VERSION, 2,
                                EGL_NONE
                            };
                            context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attrib_list);
                            CHECK_EGL(context, EGL_NO_CONTEXT, "eglCreateContext");
                            CHECK_EGL_BOOL(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context), "eglMakeCurrent (init)");
                            renderer = createRenderer();
                            break;
                        }
                        case ENGINE_CMD_VISIBLE:
                            rendererState = RENDERER_STATE_BACKGROUND;
                            renderer->visible();
                            if(!callbackData.running){
                                callbackData.running = true;
                                AChoreographer_postFrameCallback(choreographer, android_doFrame, &callbackData);
                            }
                            break;
                        case ENGINE_CMD_FOREGROUND:
                            rendererState = RENDERER_STATE_FOREGROUND;
                            renderer->foreground();
                            break;
                        case ENGINE_CMD_BACKGROUND:
                            rendererState = RENDERER_STATE_BACKGROUND;
                            renderer->background();
                            break;
                        case ENGINE_CMD_INVISIBLE:
                            rendererState = RENDERER_STATE_INVISIBLE;
                            renderer->invisible();
                            break;
                        case ENGINE_CMD_DESTROY:
                            delete renderer;
                            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                            eglDestroyContext(display, context);
                            if(surface != EGL_NO_SURFACE){
                                eglDestroySurface(display, surface);
                            }
                            eglTerminate(display);
                            running = false;
                            break;

                        case ENGINE_CMD_SURFACE_CREATE:{
                            struct SurfaceCmdData data;
                            read(fd, &data, sizeof(struct SurfaceCmdData));
                            if(surface != EGL_NO_SURFACE){
                                LOGI("surfaceCreate() while other surface still present");
                                CHECK_EGL_BOOL(eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context), "eglMakeCurrent (clear for new surface)");
                                eglDestroySurface(display, surface);
                                ANativeWindow_release(window);
                            }
                            window = data.window;
                            ANativeWindow_acquire(window);
                            EGLint none = EGL_NONE;
                            surface = eglCreateWindowSurface(display, config, window, &none);
                            CHECK_EGL(surface, EGL_NO_SURFACE, "eglCreateWindowSurface");
                            CHECK_EGL_BOOL(eglMakeCurrent(display, surface, surface, context), "eglMakeCurrent (set surface)");
                            if(!callbackData.running){
                                callbackData.running = true;
                                AChoreographer_postFrameCallback(choreographer, android_doFrame, &callbackData);
                            }
                            break;
                        }
                        case ENGINE_CMD_SURFACE_CHANGE:{
                            struct SurfaceChangeCmdData data;
                            read(fd, &data, sizeof(struct SurfaceChangeCmdData));
                            if(window != data.window && surface != EGL_NO_SURFACE){
                                LOGD("surface generation increased");
                                eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
                                eglDestroySurface(display, surface);
                                ANativeWindow_release(window);
                                window = data.window;
                                ANativeWindow_acquire(window);
                                EGLint none = EGL_NONE;
                                surface = eglCreateWindowSurface(display, config, window, &none);
                                eglMakeCurrent(display, surface, surface, context);
                            }
                            renderer->change(data.width, data.height);
                            break;
                        }
                        case ENGINE_CMD_SURFACE_DESTROY:{
                            struct SurfaceCmdData data;
                            read(fd, &data, sizeof(struct SurfaceCmdData));
                            if(surface != EGL_NO_SURFACE){
                                eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);
                                eglDestroySurface(display, surface);
                                surface = EGL_NO_SURFACE;
                                ANativeWindow_release(window);
                            }
                            break;
                        }
                    }
                }else if(readRet == 0){
                    LOGD("end of pipe?!");
                }else if(readRet == -1){
                    LOGW("error: %s", strerror(errno));
                }else{
                    LOGI("failed to read full cmd");
                }
                break;
            }
            case ENGINE_IDENT_INPUT:{
                AInputQueue *queue = (AInputQueue*)pollData;
                AInputEvent *event;
                while(AInputQueue_getEvent(queue, &event) >= 0){
                    if(AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION){
                        int32_t rawAction = AMotionEvent_getAction(event);
                        int32_t action = rawAction & AMOTION_EVENT_ACTION_MASK;
                        int32_t index = (rawAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                        PointerEvent pEvent;
                        pEvent.event = -1;
                        switch(action){
                            case AMOTION_EVENT_ACTION_DOWN:
                            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                                pEvent.event = POINTER_EVENT_DOWN;
                                pEvent.pointerId = AMotionEvent_getPointerId(event, index);
                                pEvent.x = AMotionEvent_getX(event, index);
                                pEvent.y = AMotionEvent_getY(event, index);
                                renderer->pointer(pEvent);
                                break;
                            case AMOTION_EVENT_ACTION_MOVE:
                                pEvent.event = POINTER_EVENT_MOVE;
                                for(int i=0;i<AMotionEvent_getPointerCount(event);i++){
                                    pEvent.pointerId = AMotionEvent_getPointerId(event, i);
                                    pEvent.x = AMotionEvent_getX(event, i);
                                    pEvent.y = AMotionEvent_getY(event, i);
                                    renderer->pointer(pEvent);
                                }
                                break;
                            case AMOTION_EVENT_ACTION_UP:
                            case AMOTION_EVENT_ACTION_POINTER_UP:
                            case AMOTION_EVENT_ACTION_CANCEL:
                                pEvent.event = POINTER_EVENT_UP;
                                pEvent.pointerId = AMotionEvent_getPointerId(event, index);
                                renderer->pointer(pEvent);
                                break;
                        }
                        AInputQueue_finishEvent(queue, event, pEvent.event != -1);
                    }else{
                        AInputQueue_finishEvent(queue, event, 0);
                    }
                }
                break;
            }
            case ALOOPER_POLL_CALLBACK:
            case ALOOPER_POLL_TIMEOUT:
            case ALOOPER_POLL_ERROR:
            case ALOOPER_POLL_WAKE:
                break;
        }
    }

    JNIEnv *env;
    jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
    env->DeleteGlobalRef(activity);
    ALooper_release(looper);
}

void Renderer::exit(){
    android_exit();
}

int Renderer::getState(){
    return rendererState;
}

void android_touch(int ev, int pointerId, int x, int y){
    PointerEvent event;
    event.event = ev;
    event.pointerId = pointerId;
    event.x = x;
    event.y = y;
    renderer->pointer(event);
}
