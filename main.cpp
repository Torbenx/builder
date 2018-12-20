#include <windows.h>
#include "GL/glwrapper.hpp"
#include <stdio.h>
#include <ctime>
#include "renderer.hpp"
#include "graphic/Shader.hpp"
#include "util/log.hpp"

const char g_szClassName[] = "myWindowClass";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
    switch(msg) {
		case WM_CREATE:
			break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        case WM_PAINT:
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;

    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = CS_OWNDC;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

	int width = 1024;
	int height = 512;
    HWND hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "The title of my window",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        NULL, NULL, hInstance, NULL);

    if(hwnd == NULL){
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    HDC hdc = GetDC(hwnd);
    
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
		PFD_TYPE_RGBA,            //The kind of framebuffer. RGBA or palette.
		32,                        //Colordepth of the framebuffer.
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		24,                        //Number of bits for the depthbuffer
		0,                        //Number of bits for the stencilbuffer
		0,                        //Number of Aux buffers in the framebuffer.
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

    SetPixelFormat(hdc,ChoosePixelFormat(hdc,&pfd),&pfd);
	
	HGLRC hglrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc,hglrc);
	
	
	glwrapper_init();
	
	/*float coords[] = {
	   1.0f, 1.0f,
	   -1.0f, 1.0f,
	  -1.0f, -1.0f
	};
	
	int prog = glCreateProgram();
	int vert = glCreateShader(GL_VERTEX_SHADER);
	const char* vertSource = 
		"attribute vec2 a_position;"
		"void main(){"
		"	gl_Position = vec4(a_position, 0.0, 1.0);"
		"}";
	glShaderSource(vert, 1, &vertSource, NULL);
	glCompileShader(vert);
	glAttachShader(prog, vert);
	int frag = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fragSource = 
		"void main(){"
		"	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);"
		"}";
	glShaderSource(frag, 1, &fragSource, NULL);
	glCompileShader(frag);
	glAttachShader(prog, frag);
	glLinkProgram(prog);
	
	int maxInfoLogLength = 500;
	char infoLogBuffer[maxInfoLogLength];
	int infoLogLength;
	glGetShaderInfoLog(vert, maxInfoLogLength, &infoLogLength, infoLogBuffer);
	printf("Vertex Shader: %s\n", infoLogBuffer);
	glGetShaderInfoLog(frag, maxInfoLogLength, &infoLogLength, infoLogBuffer);
	printf("Fragment Shader: %s\n", infoLogBuffer);
	glValidateProgram(prog);
	glGetProgramInfoLog(prog, maxInfoLogLength, &infoLogLength, infoLogBuffer);
	printf("Program: %s\n", infoLogBuffer);
	
	int a_position = glGetAttribLocation(prog, "a_position");
	
	glViewport(0, 0, width, height);
	
	printf("Version: %s\n",glGetString(GL_VERSION));*/
	
	clock_t start = clock();
	onSurfaceCreated();
	onSurfaceChanged(width, height);
	
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
		
		clock_t now = clock();
		//glClearColor(0,(now - start) / (float)CLOCKS_PER_SEC,0,1);
		
		//glClear(GL_COLOR_BUFFER_BIT);
		//shader->use();
		//shader->attribPointer("a_position", 2, 0, coords);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
		
		//glUseProgram(prog);
		//glEnableVertexAttribArray(a_position);
		//glVertexAttribPointer(a_position, 2, GL_FLOAT, GL_FALSE, 0, coords);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
		
		onDrawFrame();

        SwapBuffers(hdc);
    }

    return msg.wParam;
}