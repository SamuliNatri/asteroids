
// OS, graphics API & utility

#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES
#define COBJMACROS

#define MEGABYTE (1024 * 1024)
#define DEFAULT_MEMORY 10 * MEGABYTE

#define MAX_SHADERS 10
#define MAX_TEXTURES 10
#define MAX_MESHES 10
#define MAX_CONSTANT_BUFFERS 10
#define MAX_INPUT_LAYOUTS 10
#define MAX_BLEND_STATES 10

#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <windowsx.h>
#include <hidusage.h>
#include <d3d11_1.h>
#include <assert.h>
#include <time.h>
#include <float.h>
#include <math.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Types

enum {
    DEFAULT_INPUT_LAYOUT_NONE,
    DEFAULT_INPUT_LAYOUT_POSITION, 
    DEFAULT_INPUT_LAYOUT_POSITION_UV, 
    DEFAULT_INPUT_LAYOUT_COUNT,
};
enum {
    DEFAULT_SHADER_NONE,
    DEFAULT_SHADER_POSITION, 
    DEFAULT_SHADER_POSITION_UV, 
    DEFAULT_SHADER_POSITION_UV_ATLAS, 
    DEFAULT_SHADER_COUNT,
};
enum {
    DEFAULT_MESH_NONE,
    DEFAULT_MESH_TRIANGLE, 
    DEFAULT_MESH_RECTANGLE, 
    DEFAULT_MESH_RECTANGLE_UV, 
    DEFAULT_MESH_RECTANGLE_LINES, 
    DEFAULT_MESH_COUNT,
};
enum {
    DEFAULT_BLEND_STATE_NONE,
    DEFAULT_BLEND_STATE, 
    DEFAULT_BLEND_STATE_COUNT,
};

enum {
    DEFAULT_TEXTURE_NONE,
    DEFAULT_TEXTURE_FONT, 
    DEFAULT_TEXTURE_COUNT,
};

enum {
    UP, LEFT, DOWN, RIGHT, SPACE, 
    W, A, S, D, Q, E, P, M, N, R, B, C, T, KEYSAMOUNT
};

typedef uint32_t u32;
typedef struct { float X, Y, Z; } v3;
typedef struct { float X, Y, Z, W; } v4;
typedef struct { float M[4][4]; } matrix;
typedef struct { float R, G, B, A; } color;
typedef struct { v3 A, B, C; } triangle;

typedef struct {
    unsigned char* Data;
    size_t Length;
    size_t Offset;
} memory;

typedef struct {
    ID3D11ShaderResourceView* ShaderResourceView;
    ID3D11SamplerState* SamplerState;
    float UOffset;
    float VOffset;
    float USize;
    float VSize;
} texture;

typedef struct {
    float UOffset;
    float VOffset;
    float USize;
    float VSize;
} textureInfo;

typedef struct {
    ID3D11Buffer* Buffer;
    float* Vertices;
    int NumVertices;
    int Stride;
    int Offset;
} mesh;

typedef struct {
    float Left;
    float Right;
    float Top;
    float Bottom;
} rectangle;

typedef struct {
    matrix Model;        
    matrix View;          
    matrix Projection;   
    matrix Rotation;
    matrix Scale;
    color Color;         
    float UOffset;       
    float VOffset;       
    float USize;        
    float VSize;        
} constants;

typedef struct {
    UINT ByteWidth;
} constantBufferInfo;

typedef struct {
    LARGE_INTEGER StartingCount;
    LARGE_INTEGER EndingCount;
    LARGE_INTEGER TotalCount;
    LARGE_INTEGER CountsPerSecond;
    double ElapsedMilliSeconds;
} timer;

typedef struct {
    int X;
    int Y;
    int LeftButtonPressed;
    int RightButtonPressed;
    int MiddleButtonDown;
    int WheelUp;
    int WheelDown;
} mouse;

typedef struct {
    v3 Position;
    float DragSensitivity;
    float Speed;
} camera;

typedef struct {
    v3 Position;
    color Color;
    int Mesh;
    int Size;
    int Width;
    int Height;
    ID3D11VertexShader* VertexShader;
    ID3D11PixelShader* PixelShader;
    ID3D11InputLayout* InputLayout;
    ID3D10Blob* VSBlob;
    ID3D10Blob* PSBlob;
} grid;

typedef struct {
    ID3D11VertexShader* VertexShader;
    ID3D11PixelShader* PixelShader;
    ID3D10Blob* VSBlob;
    ID3D10Blob* PSBlob;
} shader;

typedef struct {
    const D3D_SHADER_MACRO* VSMacros;
    const D3D_SHADER_MACRO* PSMacros;
    // TODO: add more fields https://learn.microsoft.com/en-us/windows/win32/api/d3dcompiler/nf-d3dcompiler-d3dcompilefromfile
} shaderInfo;

// Globals

void* MemoryBackend;
memory Memory;
timer Timer;

// Zero index is not used in these arrays:
shader              Shaders[MAX_SHADERS];
mesh                Meshes[MAX_MESHES];
texture             Textures[MAX_TEXTURES];
ID3D11InputLayout*  InputLayouts[MAX_INPUT_LAYOUTS];
ID3D11BlendState*   BlendStates[MAX_BLEND_STATES];
ID3D11Buffer*       ConstantBuffers[MAX_CONSTANT_BUFFERS];

int ShaderCount          = DEFAULT_SHADER_COUNT;
int MeshCount            = DEFAULT_MESH_COUNT;
int TextureCount         = DEFAULT_TEXTURE_COUNT;
int InputLayoutCount     = DEFAULT_INPUT_LAYOUT_COUNT;
int BlendStateCount      = DEFAULT_BLEND_STATE_COUNT;
int ConstantBufferCount;

// For DrawRectangle(), testing function
int TestRectangleMesh;

float DeltaTime = 1.0f / 60.0f;

camera Camera = {
    .Position = {0.0f, 0.0f, -14.5f},
    .DragSensitivity = 0.1f,
    .Speed = 20.0f,
};

mouse Mouse;

// colors

color ColorRed =           {1.0f, 0.0f, 0.0f, 1.0f};
color ColorOrange =        {1.0f, 0.6f, 0.0f, 1.0f};
color ColorOrangeDark =    {1.0f, 0.4f, 0.0f, 1.0f};
color ColorOrangeRed =     {1.0f, 0.2f, 0.0f, 1.0f};
color ColorOrangeTomato =  {1.0f, 0.3f, 0.2f, 1.0f};
color ColorGreen =         {0.0f, 1.0f, 0.0f, 1.0f};
color ColorBlue =          {0.0f, 0.0f, 1.0f, 1.0f};
color ColorYellow =        {1.0f, 1.0f, 0.0f, 1.0f};
color ColorFuchsia =       {1.0f, 0.0f, 1.0f, 1.0f};
color ColorWhite =         {1.0f, 1.0f, 1.0f, 1.0f};
color ColorLightBlue =     {0.2f, 0.4f, 0.8f, 1.0f};
color ColorGray10 =        {0.1f, 0.1f, 0.1f, 1.0f};
color ColorGray20 =        {0.2f, 0.2f, 0.2f, 1.0f};
color ColorGray30 =        {0.3f, 0.3f, 0.3f, 1.0f};
color ColorGray40 =        {0.4f, 0.4f, 0.4f, 1.0f};
color ColorGray50 =        {0.5f, 0.5f, 0.5f, 1.0f};
color ColorGray60 =        {0.6f, 0.6f, 0.6f, 1.0f};
color ColorGray70 =        {0.7f, 0.7f, 0.7f, 1.0f};
color ColorGray80 =        {0.8f, 0.8f, 0.8f, 1.0f};
color ColorGray90 =        {0.9f, 0.9f, 0.9f, 1.0f};

color EngineColorBackground = {0.05f, 0.05f, 0.05f, 1.0f};
color EngineColorGrid =       {0.05f, 0.05f, 0.05f, 1.0f};
color EngineColorGridRed =    {1.0f, 0.05f, 0.05f, 1.0f};

int MeshTriangle;
int MeshRectangle;

int KeyDown[KEYSAMOUNT];
int KeyPressed[KEYSAMOUNT];

int Running = 1;

int WindowWidth = 640;
int WindowHeight = 640;
int ClientWidth;
int ClientHeight;
int ScreenWidth;
int ScreenHeight;

D3D11_VIEWPORT Viewport;

ID3D11Device1* Device;
ID3D11DeviceContext1* Context;
ID3D11Buffer* Buffer;

matrix ProjectionMatrix;
matrix ViewMatrix;

// Declarations

void Init();
void Input();
void Update();
void Draw();

void HandleCamera();

void DrawObject(v3 Position,
                v3 Scale,
                float Rotation,
                color Color,
                int Mesh, 
                int Texture,
                int Shader,
                int ConstantBuffer,
                int InputLayout,
                int PrimitiveTopology);

MemoryInit(size_t Size);
void* MemoryAlloc(size_t Size);

int ColorIsZero(color Color);

void DrawString(v3 Position, char* String, color Color, v3 Scale);

void GridInit(grid* Grid);
void GridDraw(grid* Grid);

int CreateMesh(float* Vertices, size_t Size, int Stride, int Offset, int MeshIndex);
void CreateDefaultMeshes();

int CreateBlendState();

void CreatetInputLayout(shader* Shader, D3D11_INPUT_ELEMENT_DESC* Desc, size_t Size, 
                        int InputLayoutIndex);

void CreateDefaultInputLayouts();
void CreateDefaultShaders();
void CreateDefaultBlendStates();
void CreateDefaultTextures();

int PickMeshRectangle(int MouseX, int MouseY, v3 Position, mesh* Mesh);
int RayTriangleIntersect(v3 RayOrigin, v3 RayDirection, triangle* Triangle);
int RectanglesIntersect(rectangle A, rectangle B);

int IsRepeat(LPARAM LParam);

DWORD InitTimer(timer* Timer);
void StartTimer(timer* Timer);
void UpdateTimer(timer* Timer);

float GetRandomZeroToOne();
color GetRandomColor();
color GetRandomShadeOfGray();

void Debug(char* Format, ...);
void DebugV3(char* Message, v3* V);
void DebugMatrix(char* Message, matrix* M);

v3 V3Add(v3 A, v3 B);
v3 V3Subtract(v3 A, v3 B);
v3 V3CrossProduct(v3 A, v3 B);
v3 V3AddScalar(v3 A, float B);
v3 V3MultiplyScalar(v3 A, float B);
v3 V3Inverse(v3 V);

float V3DotProduct(v3 A, v3 B);
float V3Length(v3* V);
float DegreesToRadians(float Degrees);
float RadiansToDegrees(float Radians);

int V3IsZero(v3 Vector);
int v3Compare(v3 A, v3 B);

void V3Normalize(v3* V);

v3 V3TransformCoord(v3* V, matrix* M);
v3 V3TransformNormal(v3* V, matrix* M);
v3 MatrixV3Multiply(matrix M, v3 V);

matrix MatrixTranslation(v3 V);
matrix MatrixMultiply(matrix* A, matrix* B);

void MatrixInverse(matrix* Source, matrix* Target);

// Win32

LRESULT CALLBACK 
WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam) {
    switch(Message) {
        
        case WM_INPUT: {
            if(Mouse.MiddleButtonDown) {
                UINT DataSize = sizeof(RAWINPUT);
                static BYTE Data[sizeof(RAWINPUT)];
                
                GetRawInputData((HRAWINPUT)LParam, RID_INPUT, Data,  &DataSize, sizeof(RAWINPUTHEADER));
                
                RAWINPUT* Raw = (RAWINPUT*)Data;
                
                if(Raw->header.dwType == RIM_TYPEMOUSE) {
                    
                    if(Raw->data.mouse.lLastX != 0) {
                        Camera.Position.X -= (float)Raw->data.mouse.lLastX * Camera.DragSensitivity;
                    }
                    
                    if(Raw->data.mouse.lLastY != 0) {
                        Camera.Position.Y += (float)Raw->data.mouse.lLastY * Camera.DragSensitivity;
                    }
                }
            }
            
        } break;
        
        case WM_MOUSEWHEEL: {
            int Delta = GET_WHEEL_DELTA_WPARAM(WParam);
            if(Delta > 0) {
                Mouse.WheelUp = 1;
            } else {
                Mouse.WheelDown = 1;
            }
        } break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP: {
            Mouse.MiddleButtonDown = (Message == WM_MBUTTONDOWN ? 1 : 0);
        } break;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN: {
            if(!IsRepeat(LParam)) {
                if(Message == WM_LBUTTONDOWN) {
                    Mouse.LeftButtonPressed = 1;
                } else {
                    Mouse.RightButtonPressed = 1;
                }
                Mouse.X = GET_X_LPARAM(LParam);
                Mouse.Y = GET_Y_LPARAM(LParam);
            }
            
        } break;
        case WM_KEYUP:
        case WM_KEYDOWN: {
            int IsKeyDown = (Message == WM_KEYDOWN ? 1 : 0);
            switch(WParam) {
                case VK_UP: {
                    KeyDown[UP] = IsKeyDown;
                } break;
                case VK_DOWN: {
                    KeyDown[DOWN] = IsKeyDown;
                } break;
                case VK_RIGHT: {
                    KeyDown[RIGHT] = IsKeyDown;
                } break;
                case VK_LEFT: {
                    KeyDown[LEFT] = IsKeyDown;
                } break;
                case 'W': {
                    KeyDown[W] = IsKeyDown;
                } break;
                case 'A': {
                    KeyDown[A] = IsKeyDown;
                } break;
                case 'C': {
                    KeyDown[C] = IsKeyDown;
                } break;
                case 'S': {
                    KeyDown[S] = IsKeyDown;
                } break;
                case 'D': {
                    KeyDown[D] = IsKeyDown;
                } break;
                case 'Q': {
                    KeyDown[Q] = IsKeyDown;
                } break;
                case 'E': {
                    KeyDown[E] = IsKeyDown;
                } break;
                case VK_SPACE: {
                    if(IsKeyDown && !IsRepeat(LParam)) {
                        KeyPressed[SPACE] = 1;
                    }
                } break;
                case 'P': {
                    if(IsKeyDown && !IsRepeat(LParam)) {
                        KeyPressed[P] = 1;
                    } 
                } break;
                case 'B': {
                    if(IsKeyDown && !IsRepeat(LParam)) {
                        KeyPressed[B] = 1;
                    } 
                } break;
                case 'M': {
                    if(IsKeyDown && !IsRepeat(LParam)) {
                        KeyPressed[M] = 1;
                    }
                } break;
                case 'N': {
                    if(IsKeyDown && !IsRepeat(LParam)) {
                        KeyPressed[N] = 1;
                    }
                } break;
                case 'R': {
                    if(IsKeyDown && !IsRepeat(LParam)) {
                        KeyPressed[R] = 1;
                    }
                } break;
                case 'T': {
                    if(IsKeyDown && !IsRepeat(LParam)) {
                        KeyPressed[T] = 1;
                    }
                } break;
                case 'O': { 
                    DestroyWindow(Window); 
                } break;
            }
        } break;
        case WM_DESTROY: { PostQuitMessage(0); } break;
        
        default: {
            return DefWindowProc(Window, Message, WParam,  LParam);
        }
    }
    
    return 0;
}

int WINAPI 
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CmdLine, int CmdShow) {
    
    MemoryInit(DEFAULT_MEMORY);
    InitTimer(&Timer);
    srand((unsigned int)time(NULL));
    
    WNDCLASS WindowClass = {0};
    const char ClassName[] = "Window";
    WindowClass.lpfnWndProc = WindowProc;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = ClassName;
    WindowClass.hCursor = LoadCursor(NULL, IDC_CROSS);
    
    if(!RegisterClass(&WindowClass)) {
        MessageBox(0, "RegisterClass failed", 0, 0);
        return GetLastError();
    }
    
    ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    HWND Window = CreateWindowEx(0, ClassName, ClassName,
                                 WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                 ScreenWidth / 2 - WindowWidth / 2,
                                 ScreenHeight / 2 - WindowHeight / 2,
                                 WindowWidth,
                                 WindowHeight,
                                 0, 0, Instance, 0);
    
    
    if(!Window) {
        MessageBox(0, "CreateWindowEx failed", 0, 0);
        return GetLastError();
    }
    
    // Get client width and height
    
    RECT ClientRect = {0};
    if(GetClientRect(Window, &ClientRect)) {
        ClientWidth = ClientRect.right;
        ClientHeight = ClientRect.bottom;
    } else {
        MessageBox(0, "GetClientRect() failed", 0, 0);
        return GetLastError();
    }
    
    // Device & Context
    
    ID3D11Device* BaseDevice;
    ID3D11DeviceContext* BaseContext;
    
    UINT CreationFlags = 0;
#ifdef _DEBUG
    CreationFlags = D3D11_CREATE_DEVICE_DEBUG;
#endif
    
    D3D_FEATURE_LEVEL FeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_0
    };
    
    HRESULT Result = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0,
                                       CreationFlags, FeatureLevels,
                                       ARRAYSIZE(FeatureLevels),
                                       D3D11_SDK_VERSION, &BaseDevice, 0,
                                       &BaseContext);    
    
    if(FAILED(Result)) {
        MessageBox(0, "D3D11CreateDevice failed", 0, 0);
        return GetLastError();
    }
    
    
    Result = ID3D11Device1_QueryInterface(BaseDevice, &IID_ID3D11Device1, (void**)&Device);
    assert(SUCCEEDED(Result));
    ID3D11Device1_Release(BaseDevice);
    
    Result = ID3D11DeviceContext1_QueryInterface(BaseContext, &IID_ID3D11DeviceContext1, (void**)&Context);
    assert(SUCCEEDED(Result));
    ID3D11Device1_Release(BaseContext);
    
    // Swap chain
    
    IDXGIDevice2* DxgiDevice;
    Result = ID3D11Device1_QueryInterface(Device, &IID_IDXGIDevice2, (void**)&DxgiDevice); 
    assert(SUCCEEDED(Result));
    
    IDXGIAdapter* DxgiAdapter;
    Result = IDXGIDevice2_GetAdapter(DxgiDevice, &DxgiAdapter); 
    assert(SUCCEEDED(Result));
    ID3D11Device1_Release(DxgiDevice);
    
    IDXGIFactory2* DxgiFactory;
    Result = IDXGIDevice2_GetParent(DxgiAdapter, &IID_IDXGIFactory2, (void**)&DxgiFactory); 
    assert(SUCCEEDED(Result));
    IDXGIAdapter_Release(DxgiAdapter);
    
    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {0};
    SwapChainDesc.Width = 0;
    SwapChainDesc.Height = 0;
    SwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = 1;
    SwapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    SwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    SwapChainDesc.Flags = 0;
    
    IDXGISwapChain1* SwapChain;
    Result = IDXGIFactory2_CreateSwapChainForHwnd(DxgiFactory, (IUnknown*)Device, Window,
                                                  &SwapChainDesc, 0, 0, &SwapChain);
    assert(SUCCEEDED(Result));
    IDXGIFactory2_Release(DxgiFactory);
    
    // Render target view
    
    ID3D11Texture2D* FrameBuffer;
    Result = IDXGISwapChain1_GetBuffer(SwapChain, 0, &IID_ID3D11Texture2D, (void**)&FrameBuffer);
    assert(SUCCEEDED(Result));
    
    ID3D11RenderTargetView* RenderTargetView;
    Result = ID3D11Device1_CreateRenderTargetView(Device, (ID3D11Resource*)FrameBuffer, 0, &RenderTargetView);
    assert(SUCCEEDED(Result));
    ID3D11Texture2D_Release(FrameBuffer);
    
    // Constant buffer
    
    CreateConstantBuffer(sizeof(constants), NULL);
    
    // Viewport
    
    Viewport = (D3D11_VIEWPORT){
        .Width = (float)ClientWidth, 
        .Height = (float)ClientHeight, 
        .MaxDepth = 1.0f,
    };
    
    // Projection matrix
    
    float AspectRatio = (float)ClientWidth / (float)ClientHeight;
    float Height = 1.0f;
    float Near = 1.0f;
    float Far = 100.0f;
    
    ProjectionMatrix = (matrix){
        2.0f * Near / AspectRatio, 0.0f, 0.0f, 0.0f, 
        0.0f, 2.0f * Near / Height, 0.0f, 0.0f, 
        0.0f, 0.0f, Far / (Far - Near), 1.0f, 
        0.0f, 0.0f, Near * Far / (Near - Far), 0.0f 
    };
    
    // View matrix
    
    ViewMatrix = (matrix){
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -Camera.Position.X, -Camera.Position.Y, -Camera.Position.Z, 1.0f,
    };
    
    // So we can get raw input data from mouse in WinProc
    
    RAWINPUTDEVICE Rid[1];
    Rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; 
    Rid[0].usUsage = HID_USAGE_GENERIC_MOUSE; 
    Rid[0].dwFlags = RIDEV_INPUTSINK;   
    Rid[0].hwndTarget = Window;
    RegisterRawInputDevices(Rid, 1, sizeof(Rid[0]));
    
    // Defaults
    
    CreateDefaultMeshes();
    CreateDefaultShaders();
    CreateDefaultInputLayouts();
    CreateDefaultBlendStates();
    CreateDefaultTextures();
    
    Init();
    
    while(Running) {
        
        UpdateTimer(&Timer);
        
        MSG Message;
        while(PeekMessage(&Message, NULL, 0, 0, PM_REMOVE)) {
            if(Message.message == WM_QUIT) Running = 0;
            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }
        
        Input();
        HandleCamera();
        Update();
        
        float ClearColor[] = {EngineColorBackground.R, EngineColorBackground.G, EngineColorBackground.B};
        
        ID3D11DeviceContext1_ClearRenderTargetView(Context, RenderTargetView, ClearColor);
        ID3D11DeviceContext1_RSSetViewports(Context, 1, &Viewport);
        
        float BlendFactor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        UINT BlendSampleMask = 0xffffffff;
        ID3D11DeviceContext1_OMSetBlendState(Context, 
                                             BlendStates[DEFAULT_BLEND_STATE], 
                                             BlendFactor, BlendSampleMask);
        
        ID3D11DeviceContext1_OMSetRenderTargets(Context, 1, &RenderTargetView, 0);
        
        ID3D11DeviceContext1_VSSetConstantBuffers(Context, 0, 1, &ConstantBuffers[0]);
        
        Draw();
        
        IDXGISwapChain1_Present(SwapChain, 1, 0);
        
    }
    
    return 0;
}

// dx11

void CreateDefaultTextures() {
    
    // Font
    
    textureInfo FontTextureInfo = {
        .USize = 1.0f / 16.0f,
        .VSize = 1.0f / 16.0f
    };
    
    CreateTexture("font_64_64.png", 
                  &FontTextureInfo, 
                  DEFAULT_TEXTURE_FONT);
}

void CreateDefaultShaders() {
    CreateShader(L"default_shaders_position.hlsl", NULL, DEFAULT_SHADER_POSITION);
    CreateShader(L"default_shaders_position_uv.hlsl", NULL, DEFAULT_SHADER_POSITION_UV);
    CreateShader(L"default_shaders_position_uv_atlas.hlsl", NULL, DEFAULT_SHADER_POSITION_UV_ATLAS);
}

void CreateDefaultInputLayouts() {
    
    {
        // INPUT_LAYOUT_POSITION
        
        D3D11_INPUT_ELEMENT_DESC 
            InputElementDesc[] = {
            {
                "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 
                0, 0, 
                D3D11_INPUT_PER_VERTEX_DATA, 0
            },
        };
        
        shader TempShader = {0};
        CreateShaderDiscard(L"dummy_shaders_position.hlsl", NULL, &TempShader);
        
        CreateInputLayout(&TempShader, InputElementDesc, ARRAYSIZE(InputElementDesc),
                          DEFAULT_INPUT_LAYOUT_POSITION);
    }
    
    {
        // INPUT_LAYOUT_POSITION_UV
        
        D3D11_INPUT_ELEMENT_DESC 
            InputElementDesc[] = {
            {
                "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 
                0, 0, 
                D3D11_INPUT_PER_VERTEX_DATA, 0
            },
            {
                "UV", 0, 
                DXGI_FORMAT_R32G32_FLOAT, 
                0, D3D11_APPEND_ALIGNED_ELEMENT, 
                D3D11_INPUT_PER_VERTEX_DATA, 0
            },
        };
        
        shader TempShader = {0};
        CreateShaderDiscard(L"dummy_shaders_position_uv.hlsl", NULL, &TempShader);
        
        CreateInputLayout(&TempShader, InputElementDesc, ARRAYSIZE(InputElementDesc),
                          DEFAULT_INPUT_LAYOUT_POSITION_UV);
    }
}


// TODO: handle info
int CreateConstantBuffer(size_t Size, constantBufferInfo* Info) {
    
    D3D11_BUFFER_DESC ConstantBufferDesc = {
        .ByteWidth = Size,
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    };
    
    HRESULT Result = ID3D11Device1_CreateBuffer(Device, &ConstantBufferDesc, NULL, &ConstantBuffers[ConstantBufferCount++]);
    assert(SUCCEEDED(Result));
    
    return ConstantBufferCount-1;
}


// Returns index to Textures array
int CreateTexture(const char* File, textureInfo* Info, int TextureIndex) {
    
    int Index = TextureIndex;
    if(Index == 0) Index = TextureCount++;
    texture* Texture = &Textures[Index];
    
    Texture->USize = 1.0f;
    Texture->VSize = 1.0f;
    
    if(Info != NULL) {
        Texture->UOffset = Info->UOffset;
        Texture->VOffset = Info->VOffset;
        Texture->USize = Info->USize;
        Texture->VSize = Info->VSize;
    }
    
    // Load image
    
    int ImageWidth;
    int ImageHeight;
    int ImageChannels;
    int ImageDesiredChannels = 4;
    
    unsigned char* ImageData = stbi_load(File,
                                         &ImageWidth, 
                                         &ImageHeight, 
                                         &ImageChannels, ImageDesiredChannels);
    assert(ImageData);
    
    int ImagePitch = ImageWidth * 4;
    
    // Texture
    
    D3D11_TEXTURE2D_DESC ImageTextureDesc = {0};
    
    ImageTextureDesc.Width = ImageWidth;
    ImageTextureDesc.Height = ImageHeight;
    ImageTextureDesc.MipLevels = 1;
    ImageTextureDesc.ArraySize = 1;
    ImageTextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    ImageTextureDesc.SampleDesc.Count = 1;
    ImageTextureDesc.SampleDesc.Quality = 0;
    ImageTextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ImageTextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    
    D3D11_SUBRESOURCE_DATA ImageSubresourceData = {0};
    
    ImageSubresourceData.pSysMem = ImageData; 
    ImageSubresourceData.SysMemPitch = ImagePitch; 
    
    ID3D11Texture2D* ImageTexture;
    
    HRESULT Result = ID3D11Device1_CreateTexture2D(Device, &ImageTextureDesc,
                                                   &ImageSubresourceData,
                                                   &ImageTexture
                                                   );
    assert(SUCCEEDED(Result));
    
    free(ImageData);
    
    // Shader resource view
    
    Result = ID3D11Device1_CreateShaderResourceView(Device,
                                                    (ID3D11Resource *)ImageTexture,
                                                    NULL,
                                                    &Texture->ShaderResourceView
                                                    );
    assert(SUCCEEDED(Result));
    
    // Sampler
    
    D3D11_SAMPLER_DESC ImageSamplerDesc = {0};
    
    ImageSamplerDesc.Filter   = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    ImageSamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    ImageSamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    ImageSamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    ImageSamplerDesc.MipLODBias = 0.0f;
    ImageSamplerDesc.MaxAnisotropy = 1;
    ImageSamplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    ImageSamplerDesc.BorderColor[0] = 1.0f;
    ImageSamplerDesc.BorderColor[1] = 1.0f;
    ImageSamplerDesc.BorderColor[2] = 1.0f;
    ImageSamplerDesc.BorderColor[3] = 1.0f;
    ImageSamplerDesc.MinLOD = -FLT_MAX;
    ImageSamplerDesc.MaxLOD = FLT_MAX;
    
    Result = ID3D11Device1_CreateSamplerState(Device, 
                                              &ImageSamplerDesc,
                                              &Texture->SamplerState);
    assert(SUCCEEDED(Result));
    
    return Index;
}

int CreateMesh(float* Vertices, size_t Size, int StrideInt, int Offset, int MeshIndex) {
    
    int Index = MeshIndex;
    if(Index == 0) Index = MeshCount++;
    mesh* Mesh = &Meshes[Index];
    
    Mesh->Stride = StrideInt * sizeof(float);
    Mesh->NumVertices = Size / Mesh->Stride;
    Mesh->Offset = Offset;
    Mesh->Vertices = MemoryAlloc(Size);
    memcpy(Mesh->Vertices, Vertices, Size);
    
    D3D11_BUFFER_DESC BufferDesc = {
        Size,
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0, 0, 0
    };
    
    D3D11_SUBRESOURCE_DATA InitialData = { Mesh->Vertices };
    
    ID3D11Device1_CreateBuffer(Device,
                               &BufferDesc,
                               &InitialData,
                               &Mesh->Buffer);
    return Index;
}

void CreateDefaultMeshes() {
    
    // Triangle 
    
    float TriangleVertexData[] = {
        -0.5f, -0.5f, 0.0f,
        0.0f,  0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
    };
    
    CreateMesh(TriangleVertexData, sizeof(TriangleVertexData),
               3, 0, DEFAULT_MESH_TRIANGLE);
    
    // Rectangle
    
    float RectangleVertexData[] = {
        -0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f, 
        -0.5f, -0.5f, 0.0f,
        0.5f, 0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
    };
    
    CreateMesh(RectangleVertexData, sizeof(RectangleVertexData),
               3, 0, DEFAULT_MESH_RECTANGLE);
    
    // Rectangle, with uv
    
    float RectangleUVVertexData[] = {
        // xyz              // uv
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.0f,  0.0f, 0.0f,
        0.5f, 0.5f, 0.0f,   1.0f, 0.0f, 
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.0f,   1.0f, 0.0f,
        0.5f, -0.5f, 0.0f,  1.0f, 1.0f,
    };
    
    CreateMesh(RectangleUVVertexData, sizeof(RectangleUVVertexData),
               5, 0, DEFAULT_MESH_RECTANGLE_UV);
    
    // Rectangle, with lines
    
    float RectangleLinesVertexData[] = {
        -0.5f, -0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        -0.5f, 0.5f, 0.0f,
        0.5f, 0.5f, 0.0f, 
        0.5f, 0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        0.5f, -0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
    };
    
    CreateMesh(RectangleLinesVertexData, sizeof(RectangleLinesVertexData),
               3, 0, DEFAULT_MESH_RECTANGLE_LINES);
}



void DrawObject(v3 Position,
                v3 Scale,
                float Rotation,
                color Color,
                int Mesh, 
                int Texture,
                int Shader,
                int ConstantBuffer,
                int InputLayout,
                int PrimitiveTopology) {
    
    if(Texture) {
        ID3D11DeviceContext1_PSSetShaderResources(Context, 0, 1, &Textures[Texture].ShaderResourceView);
        ID3D11DeviceContext1_PSSetSamplers(Context, 0, 1, &Textures[Texture].SamplerState);
    }
    
    ID3D11DeviceContext1_IASetInputLayout(Context, InputLayouts[InputLayout]);
    ID3D11DeviceContext1_VSSetShader(Context, Shaders[Shader].VertexShader, 0, 0);
    ID3D11DeviceContext1_PSSetShader(Context, Shaders[Shader].PixelShader, 0, 0);
    ID3D11DeviceContext1_IASetPrimitiveTopology(Context, PrimitiveTopology);
    ID3D11DeviceContext1_IASetVertexBuffers(Context, 0, 1, &Meshes[Mesh].Buffer, &Meshes[Mesh].Stride, &Meshes[Mesh].Offset);
    
    D3D11_MAPPED_SUBRESOURCE MappedSubresource;
    
    ID3D11DeviceContext1_Map(Context, (ID3D11Resource*)ConstantBuffers[ConstantBuffer], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
    constants* Constants = (constants*)MappedSubresource.pData;
    
    Constants->Model = (matrix){
        1.0f, 0.0f, 0.0f, 0.0f, 
        0.0f, 1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 1.0f, 0.0f, 
        Position.X, 
        Position.Y, 
        Position.Z, 1.0f
    };
    
    float Theta = DegreesToRadians(Rotation);
    
    Constants->Rotation = (matrix){
        cos(Theta),  sin(Theta), 0.0f, 0.0f, 
        -sin(Theta), cos(Theta), 0.0f, 0.0f, 
        0.0f,        0.0f,       1.0f, 0.0f, 
        0.0f,        0.0f,       0.0f, 1.0f, 
    };
    
    Constants->Scale = (matrix){
        Scale.X, 0.0f, 0.0f, 0.0f, 
        0.0f, Scale.Y, 0.0f, 0.0f, 
        0.0f, 0.0f, Scale.Z, 0.0f, 
        0.0f, 0.0f, 0.0f,    1.0f, 
    };
    
    Constants->View = ViewMatrix;
    Constants->Projection = ProjectionMatrix;
    Constants->Color = Color;
    
    if(Texture) {
        Constants->UOffset = Textures[Texture].UOffset;
        Constants->VOffset = Textures[Texture].VOffset;
        Constants->USize  = Textures[Texture].USize;
        Constants->VSize  = Textures[Texture].VSize;
    }
    
    ID3D11DeviceContext1_Unmap(Context, (ID3D11Resource*)ConstantBuffers[ConstantBuffer], 0);
    ID3D11DeviceContext1_Draw(Context, Meshes[Mesh].NumVertices, 0);
}

void CreateDefaultBlendStates() {
    CreateBlendState();
}

// TODO: params
int CreateBlendState() {
    
    D3D11_BLEND_DESC BlendStateDesc = {
        .RenderTarget[0].BlendEnable    = TRUE,
        .RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
        
        .RenderTarget[0].SrcBlend       = D3D11_BLEND_ONE,
        .RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE,
        .RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA,
        .RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
        
        .RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD,
        .RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD,
    };
    
    ID3D11Device1_CreateBlendState(Device, 
                                   &BlendStateDesc, 
                                   &BlendStates[DEFAULT_BLEND_STATE]);
    return BlendStateCount-1;
}

// This is used to create temp shaders for input layouts
int CreateShaderDiscard(const wchar_t* Filename, shaderInfo* Info, shader* Shader) {
    
    HRESULT Result = E_FAIL;
    
    if(Info == NULL) {
        Result = D3DCompileFromFile(Filename, NULL, NULL, "vs_main", "vs_5_0", NULL, NULL, &Shader->VSBlob, NULL);
        assert(SUCCEEDED(Result));
        
        Result = D3DCompileFromFile(Filename, NULL, NULL, "ps_main", "ps_5_0", NULL, NULL, &Shader->PSBlob, NULL);
        assert(SUCCEEDED(Result));
    } else {
        // TODO: handle all Info fields
        Result = D3DCompileFromFile(Filename, Info->VSMacros, NULL, "vs_main", "vs_5_0", NULL, NULL, &Shader->VSBlob, NULL);
        assert(SUCCEEDED(Result));
        Result = D3DCompileFromFile(Filename, Info->PSMacros, NULL, "ps_main", "ps_5_0", NULL, NULL, &Shader->PSBlob, NULL);
        assert(SUCCEEDED(Result));
        
    }
    
    Result = ID3D11Device1_CreateVertexShader(Device,
                                              ID3D10Blob_GetBufferPointer(Shader->VSBlob),
                                              ID3D10Blob_GetBufferSize(Shader->VSBlob),
                                              0,
                                              &Shader->VertexShader);
    assert(SUCCEEDED(Result));
    
    Result = ID3D11Device1_CreatePixelShader(Device,
                                             ID3D10Blob_GetBufferPointer(Shader->PSBlob),
                                             ID3D10Blob_GetBufferSize(Shader->PSBlob),
                                             0,
                                             &Shader->PixelShader);
    assert(SUCCEEDED(Result));
    
    return Result;
}

int CreateShader(const wchar_t* Filename, shaderInfo* Info, int ShaderIndex) {
    
    int Index = ShaderIndex;
    if(ShaderIndex == 0) Index = ShaderCount++;
    shader* Shader = &Shaders[Index];
    
    HRESULT Result = E_FAIL;
    
    if(Info == NULL) {
        Result = D3DCompileFromFile(Filename, NULL, NULL, "vs_main", "vs_5_0", NULL, NULL, &Shader->VSBlob, NULL);
        assert(SUCCEEDED(Result));
        
        Result = D3DCompileFromFile(Filename, NULL, NULL, "ps_main", "ps_5_0", NULL, NULL, &Shader->PSBlob, NULL);
        assert(SUCCEEDED(Result));
    } else {
        // TODO: handle all Info fields
        Result = D3DCompileFromFile(Filename, Info->VSMacros, NULL, "vs_main", "vs_5_0", NULL, NULL, &Shader->VSBlob, NULL);
        assert(SUCCEEDED(Result));
        Result = D3DCompileFromFile(Filename, Info->PSMacros, NULL, "ps_main", "ps_5_0", NULL, NULL, &Shader->PSBlob, NULL);
        assert(SUCCEEDED(Result));
        
    }
    
    Result = ID3D11Device1_CreateVertexShader(Device,
                                              ID3D10Blob_GetBufferPointer(Shader->VSBlob),
                                              ID3D10Blob_GetBufferSize(Shader->VSBlob),
                                              0,
                                              &Shader->VertexShader);
    assert(SUCCEEDED(Result));
    
    Result = ID3D11Device1_CreatePixelShader(Device,
                                             ID3D10Blob_GetBufferPointer(Shader->PSBlob),
                                             ID3D10Blob_GetBufferSize(Shader->PSBlob),
                                             0,
                                             &Shader->PixelShader);
    assert(SUCCEEDED(Result));
    
    return Index;
}

int CreateInputLayout(shader* Shader, 
                      D3D11_INPUT_ELEMENT_DESC* Desc, 
                      size_t Size, 
                      int InputLayoutIndex) {
    
    int Index = InputLayoutIndex;
    
    if(Index == 0) {
        int Index = InputLayoutCount++;
    }
    
    HRESULT Result = 
        ID3D11Device1_CreateInputLayout(Device, 
                                        Desc,
                                        Size,
                                        ID3D10Blob_GetBufferPointer(Shader->VSBlob),
                                        ID3D10Blob_GetBufferSize(Shader->VSBlob),
                                        &InputLayouts[Index]
                                        );
    assert(SUCCEEDED(Result));
    
    return Index;
}

// TODO: Broken. Fix.
void GridInit(grid* Grid) {
    
    // Shaders
    
    HRESULT Result = D3DCompileFromFile(L"shaders_grid.hlsl", 0, 0, "vs_main", "vs_5_0", 0, 0, &Grid->VSBlob, 0);
    assert(SUCCEEDED(Result));
    
    Result = ID3D11Device1_CreateVertexShader(Device,
                                              ID3D10Blob_GetBufferPointer(Grid->VSBlob),
                                              ID3D10Blob_GetBufferSize(Grid->VSBlob),
                                              0,
                                              &Grid->VertexShader);
    assert(SUCCEEDED(Result));
    
    Result = D3DCompileFromFile(L"shaders_grid.hlsl", 0, 0, "ps_main", "ps_5_0", 0, 0, &Grid->PSBlob, 0);
    assert(SUCCEEDED(Result));
    
    Result = ID3D11Device1_CreatePixelShader(Device,
                                             ID3D10Blob_GetBufferPointer(Grid->PSBlob),
                                             ID3D10Blob_GetBufferSize(Grid->PSBlob),
                                             0,
                                             &Grid->PixelShader);
    assert(SUCCEEDED(Result));
    
    // Data layout
    
    D3D11_INPUT_ELEMENT_DESC GridInputElementDesc[] = {
        {
            "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 
            0, 0, 
            D3D11_INPUT_PER_VERTEX_DATA, 0
        },
    };
    
    Result = ID3D11Device1_CreateInputLayout(Device, 
                                             GridInputElementDesc,
                                             ARRAYSIZE(GridInputElementDesc),
                                             ID3D10Blob_GetBufferPointer(Grid->VSBlob),
                                             ID3D10Blob_GetBufferSize(Grid->VSBlob),
                                             &Grid->InputLayout
                                             );
    assert(SUCCEEDED(Result));
    
    // Mesh
    
    if(Grid->Size <= 0.0f) Grid->Size = 1.0f;
    
    int YLines = Grid->Height + 1;
    int XLines = Grid->Width + 1;
    float HalfSize = Grid->Size / 2.0f;
    float* Vertices = NULL;
    
    size_t Size = (YLines * 6 + XLines * 6) * sizeof(*Vertices);
    Vertices = MemoryAlloc(Size);
    
    int VerticesIndex = 0;
    
    for(int Line = 0; Line < YLines; ++Line) {
        Vertices[0 + Line * 6] = -HalfSize;
        Vertices[1 + Line * 6] = -HalfSize + (float)Line*Grid->Size;
        Vertices[2 + Line * 6] = 0.0f;
        Vertices[3 + Line * 6] = -HalfSize + (float)Grid->Width;
        Vertices[4 + Line * 6] = -HalfSize + (float)Line*Grid->Size;
        Vertices[5 + Line * 6] = 0.0f;
        VerticesIndex = 5 + Line * 6;
    }
    
    for(int Line = 0; Line < XLines; ++Line) {
        Vertices[VerticesIndex + 1 + 0 + Line * 6] = -HalfSize + (float)Line*Grid->Size;
        Vertices[VerticesIndex + 1 + 1 + Line * 6] = -HalfSize;
        Vertices[VerticesIndex + 1 + 2 + Line * 6] = 0.0f;
        Vertices[VerticesIndex + 1 + 3 + Line * 6] = -HalfSize + (float)Line*Grid->Size;
        Vertices[VerticesIndex + 1 + 4 + Line * 6] = -HalfSize + (float)Grid->Width;
        Vertices[VerticesIndex + 1 + 5 + Line * 6] = 0.0f;
    }
    
    Grid->Mesh = CreateMesh(Vertices, Size, 3, 0, 0);
}


void GridDraw(grid* Grid) {
    
    ID3D11DeviceContext1_IASetInputLayout(Context, Grid->InputLayout);
    ID3D11DeviceContext1_VSSetShader(Context, Grid->VertexShader, 0, 0);
    ID3D11DeviceContext1_PSSetShader(Context, Grid->PixelShader, 0, 0);
    
    
    ID3D11DeviceContext1_IASetPrimitiveTopology(Context, D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    ID3D11DeviceContext1_IASetVertexBuffers(Context, 0, 1, &Meshes[Grid->Mesh].Buffer, &Meshes[Grid->Mesh].Stride, &Meshes[Grid->Mesh].Offset);
    
    D3D11_MAPPED_SUBRESOURCE MappedSubresource;
    ID3D11DeviceContext1_Map(Context, (ID3D11Resource*)ConstantBuffers[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedSubresource);
    constants* Constants = (constants*)MappedSubresource.pData;
    Constants->Model = (matrix){
        1.0f, 0.0f, 0.0f, 0.0f, 
        0.0f, 1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 1.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 1.0f, 
    };
    
    Constants->View = ViewMatrix;
    Constants->Projection = ProjectionMatrix;
    Constants->Color = Grid->Color;
    ID3D11DeviceContext1_Unmap(Context, (ID3D11Resource*)ConstantBuffers[0], 0);
    ID3D11DeviceContext1_Draw(Context, Meshes[Grid->Mesh].NumVertices, 0);
    
}

// Camera

void CameraUpdateByAcceleration(v3 Acceleration) {
    v3 CameraVelocity = {0};
    
    CameraVelocity = V3Add(CameraVelocity, 
                           V3MultiplyScalar(Acceleration, DeltaTime * Camera.Speed));
    Camera.Position = V3Add(Camera.Position, V3MultiplyScalar(CameraVelocity, DeltaTime * Camera.Speed));
    
    ViewMatrix = (matrix){
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -Camera.Position.X, -Camera.Position.Y, -Camera.Position.Z, 1.0f,
    };
}


void HandleCamera() {
    
    v3 CameraAcceleration = {0};
    
    if(KeyDown[W]) {
        CameraAcceleration.Y = 1.0f; 
    }
    if(KeyDown[A]) {
        CameraAcceleration.X = -1.0f; 
    }
    if(KeyDown[S]) {
        CameraAcceleration.Y = -1.0f; 
    }
    if(KeyDown[D]) {
        CameraAcceleration.X = 1.0f; 
    }
    if(KeyDown[Q]) {
        CameraAcceleration.Z = -1.0f;
    }
    if(KeyDown[E]) {
        CameraAcceleration.Z = 1.0f; 
    }
    
    if(Mouse.WheelDown) {
        CameraAcceleration.Z = -10.0f;
        Mouse.WheelDown = 0;
    }
    if(Mouse.WheelUp) {
        CameraAcceleration.Z = 10.0f;
        Mouse.WheelUp = 0;
    }
    
    CameraUpdateByAcceleration(CameraAcceleration);
}

// Misc

// Note: creates a mesh every iteration, for testing
/*
void DrawRectangle(rectangle* R, color Color) {
    
    float RectangleLinesVertexData[] = {
        R->Left, R->Bottom, 0.0f,
        R->Left, R->Top, 0.0f,
        R->Left, R->Top, 0.0f,
        R->Right, R->Top, 0.0f, 
        R->Right, R->Top, 0.0f,
        R->Right, R->Bottom, 0.0f,
        R->Right, R->Bottom, 0.0f,
        R->Left, R->Bottom, 0.0f,
    };
    
    TestRectangleMesh = CreateMesh(RectangleLinesVertexData, sizeof(RectangleLinesVertexData),
                                   3, 0, 0);
    
    DrawObject((v3){0.0f, 0.0f, 0.0f},
               (v3){1.0f, 1.0f, 1.0f},
               0.0f,
               Color,
               TestRectangleMesh,
               0,
               DEFAULT_SHADER_POSITION,
               1,
               DEFAULT_INPUT_LAYOUT_POSITION,
               D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
}
*/

int PickMeshRectangle(int MouseX, int MouseY, v3 Position, mesh* Mesh) {
    
    int Result = 0;
    float X = ((2.0f * (float)MouseX) / (float)ClientWidth) - 1.0f;
    float Y = (((2.0f * (float)MouseY) / (float)ClientHeight) - 1.0f) * -1.0f;
    
    X = X / ProjectionMatrix.M[0][0];
    Y = Y / ProjectionMatrix.M[1][1];
    
    matrix InverseViewMatrix = {0};
    MatrixInverse(&ViewMatrix, &InverseViewMatrix);
    
    v3 Direction = {0};
    Direction.X = X * InverseViewMatrix.M[0][0] + Y * InverseViewMatrix.M[1][0] + InverseViewMatrix.M[2][0];
    Direction.Y = X * InverseViewMatrix.M[0][1] + Y * InverseViewMatrix.M[1][1] + InverseViewMatrix.M[2][1];
    Direction.Z = X * InverseViewMatrix.M[0][2] + Y * InverseViewMatrix.M[1][2] + InverseViewMatrix.M[2][2];
    
    v3 Origin = Camera.Position;
    matrix TranslatedModel = MatrixTranslation(Position);
    
    matrix InverseModel = {0};
    MatrixInverse(&TranslatedModel, &InverseModel);
    
    v3 RayOrigin = V3TransformCoord(&Origin, &InverseModel);
    v3 RayDirection = V3TransformNormal(&Direction, &InverseModel);
    
    V3Normalize(&RayDirection);
    
    triangle Triangle1 = {
        .A = {
            Mesh->Vertices[0], 
            Mesh->Vertices[1], 
            Mesh->Vertices[2]
        },
        .B = {
            Mesh->Vertices[5], 
            Mesh->Vertices[6], 
            Mesh->Vertices[7]
        },
        .C = {
            Mesh->Vertices[10], 
            Mesh->Vertices[11], 
            Mesh->Vertices[12]
        }
    };
    
    triangle Triangle2 = {
        .A = {
            Mesh->Vertices[15], 
            Mesh->Vertices[16], 
            Mesh->Vertices[17]
        },
        .B = {
            Mesh->Vertices[20], 
            Mesh->Vertices[21], 
            Mesh->Vertices[22]
        },
        .C = {
            Mesh->Vertices[25], 
            Mesh->Vertices[26], 
            Mesh->Vertices[27]
        }
    };
    
    if(RayTriangleIntersect(RayOrigin, RayDirection, &Triangle1) || RayTriangleIntersect(RayOrigin, RayDirection, &Triangle2)) {
        Result = 1;
    }
    
    return Result;
}


void DrawString(v3 Position, char* String, color Color, v3 Scale) {
    
    v3 NewPosition = Position;
    
    while(*String) {
        
        Textures[DEFAULT_TEXTURE_FONT].UOffset = *String % 16;
        Textures[DEFAULT_TEXTURE_FONT].VOffset = *String / 16;
        
        DrawObject(NewPosition, 
                   Scale,
                   0.0f,
                   Color,
                   DEFAULT_MESH_RECTANGLE_UV,
                   DEFAULT_TEXTURE_FONT,
                   DEFAULT_SHADER_POSITION_UV_ATLAS,
                   0,
                   DEFAULT_INPUT_LAYOUT_POSITION_UV,
                   D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ++String;
        NewPosition.X += Scale.X;
    }
}

int IsRepeat(LPARAM LParam) {
    return (HIWORD(LParam) & KF_REPEAT);
}

void StartTimer(timer* Timer) {
    QueryPerformanceCounter(&Timer->StartingCount);
}

void UpdateTimer(timer* Timer) {
    // get current tick
    QueryPerformanceCounter(&Timer->EndingCount);
    // get tick count
    Timer->TotalCount.QuadPart = Timer->EndingCount.QuadPart - Timer->StartingCount.QuadPart;
    // for precision
    Timer->TotalCount.QuadPart *= 1000000;
    // calculate elapsed milliseconds
    Timer->ElapsedMilliSeconds = (double)(Timer->TotalCount.QuadPart / Timer->CountsPerSecond.QuadPart) / 1000;
}

DWORD InitTimer(timer* Timer) {
    if(!QueryPerformanceFrequency(&Timer->CountsPerSecond)) {
        return GetLastError();
    }
    StartTimer(Timer);
    return 0;
}

int TimeElapsed(double* Time, double Elapsed) {
    if(Timer.ElapsedMilliSeconds - *Time > Elapsed) {
        *Time = Timer.ElapsedMilliSeconds;
        return 1;
    }
    return 0;
}


/*
Möller–Trumbore intersection algorithm
https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
*/
int RayTriangleIntersect(v3 RayOrigin, v3 RayDirection, triangle* Triangle) {
    const float EPSILON = 0.0000001f;
    v3 Vertex0 = Triangle->A;
    v3 Vertex1 = Triangle->B;
    v3 Vertex2 = Triangle->C;
    v3 Edge1, Edge2, H, S, Q;
    float A, F, U, V;
    Edge1 = V3Subtract(Vertex1, Vertex0);
    Edge2 = V3Subtract(Vertex2, Vertex0);
    H = V3CrossProduct(RayDirection, Edge2);
    A = V3DotProduct(Edge1, H);
    if(A > -EPSILON && A < EPSILON) return 0;    
    F = 1.0f / A;
    S = V3Subtract(RayOrigin, Vertex0);
    U = F * V3DotProduct(S, H);
    if(U < 0.0f || U > 1.0f) return 0;
    Q = V3CrossProduct(S, Edge1);
    V = F * V3DotProduct(RayDirection, Q);
    if(V < 0.0f || ((U + V) > 1.0f)) return 0;
    float T = F * V3DotProduct(Edge2, Q);
    if(T > EPSILON) return 1;
    return 0;
}


int RectanglesIntersect(rectangle A, rectangle B) {
    if(A.Left > B.Right) return 0;
    if(B.Left > A.Right) return 0;
    if(A.Bottom > B.Top) return 0;
    if(B.Bottom > A.Top) return 0;
    return 1;
}

int ColorIsZero(color Color) {
    if(Color.R == 0.0f &&
       Color.G == 0.0f &&
       Color.B == 0.0f &&
       Color.A == 0.0f) {
        return 1; 
    }
    return 0;
}

float GetRandomZeroToOne() {
    return (float)rand() / (float)RAND_MAX;
}

color GetRandomColor() {
    return (color){
        GetRandomZeroToOne(),
        GetRandomZeroToOne(),
        GetRandomZeroToOne(),
    };
}

color GetRandomShadeOfGray() {
    float Shade = GetRandomZeroToOne();
    return (color){
        Shade,
        Shade,
        Shade,
    };
}

color GetColorByRGB(int R, int G, int B) {
    return (color){
        (float)R / 255.0f,
        (float)G / 255.0f,
        (float)B / 255.0f,
        1.0f,
    };
}

void Debug(char* Format, ...) {
    va_list Arguments;
    va_start(Arguments, Format);
    char String[1024] = {0};
    vsprintf(String, Format, Arguments);
    OutputDebugString(String);
    va_end(Arguments);
}

void DebugV3(char* Message, v3* V) {
    OutputDebugString(Message);
    char String[24] = {0};
    sprintf(String, 
            "\n%+06.2f %+06.2f %+06.2f\n",
            V->X, V->Y, V->Z);
    OutputDebugString(String);
}

void DebugMatrix(char* Message, matrix* M) {
    OutputDebugString(Message);
    char String[1024] = {0};
    sprintf(String, 
            "\n%+06.2f %+06.2f %+06.2f %+06.2f\n"
            "%+06.2f %+06.2f %+06.2f %+06.2f\n"
            "%+06.2f %+06.2f %+06.2f %+06.2f\n"
            "%+06.2f %+06.2f %+06.2f %+06.2f\n",
            M->M[0][0], M->M[0][1], M->M[0][2], M->M[0][3],
            M->M[1][0], M->M[1][1], M->M[1][2], M->M[1][3],
            M->M[2][0], M->M[2][1], M->M[2][2], M->M[2][3],
            M->M[3][0], M->M[3][1], M->M[3][2], M->M[3][3]
            );
    OutputDebugString(String);
}

// Memory

MemoryInit(size_t Size) {
    MemoryBackend = malloc(Size);
    Memory.Data = (unsigned char*)MemoryBackend;
    Memory.Length = Size;
    Memory.Offset = 0;
}

void* MemoryAlloc(size_t Size) {
    void* Pointer = NULL;
    if(Memory.Offset+Size <= Memory.Length) {
        Pointer = &Memory.Data[Memory.Offset];
        Memory.Offset += Size;
        memset(Pointer, 0, Size);
    }
    
    if(Pointer == NULL) {
        Debug("Game over, man. Game over!");
    }
    
    return Pointer;
}

// Maths

float DegreesToRadians(float Degrees) {
    return Degrees * M_PI / 180.0f;
}

float RadiansToDegrees(float Radians) {
    return Radians * 180.0f / M_PI;
}

float V3GetDistance(v3 A, v3 B) {
    return(sqrt((A.X - B.X) * (A.X - B.X) + 
                (A.Y - B.Y) * (A.Y - B.Y)
                ));
}

v3 V3GetDirection(v3 A, v3 B) {
    v3 Direction = V3Subtract(B, A);
    V3Normalize(&Direction);
    return Direction;
}

v3 V3GetRandomV2Direction() {
    return (v3){
        cos((float) (rand() % 361)), 
        sin((float) (rand() % 361)), 
        0.0f
    };
}

v3 V3Inverse(v3 V) {
    return (v3){-V.X, -V.Y, -V.Z};
}

v3 V3Add(v3 A, v3 B) {
    v3 Result = {0};
    Result.X += A.X + B.X;
    Result.Y += A.Y + B.Y;
    Result.Z += A.Z + B.Z;
    return Result;
}

v3 V3Subtract(v3 A, v3 B) {
    v3 Result = {0};
    Result.X += A.X - B.X;
    Result.Y += A.Y - B.Y;
    Result.Z += A.Z - B.Z;
    return Result;
}

v3 V3CrossProduct(v3 A, v3 B) {
    return (v3) {
        .X = A.Y * B.Z - A.Z * B.Y,
        .Y = A.Z * B.X - A.X * B.Z,
        .Z = A.X * B.Y - A.Y * B.X
    };
}

float V3DotProduct(v3 A, v3 B) {
    return (A.X * B.X + A.Y * B.Y + A.Z * B.Z);
}

v3 V3AddScalar(v3 A, float B) {
    v3 Result = {0};
    Result.X += A.X + B;
    Result.Y += A.Y + B;
    Result.Z += A.Z + B;
    return Result;
}

v3 V3MultiplyScalar(v3 A, float B) {
    v3 Result = {0};
    Result.X = A.X * B;
    Result.Y = A.Y * B;
    Result.Z = A.Z * B;
    return Result;
}

int V3IsZero(v3 Vector) {
    if(Vector.X == 0.0f &&
       Vector.Y == 0.0f &&
       Vector.Z == 0.0f) {
        return 1; 
    }
    return 0;
}

int V3Compare(v3 A, v3 B) {
    if(A.X == B.X &&
       A.Y == B.Y &&
       A.Z == B.Z) {
        return 1; 
    }
    return 0;
}

float V3Length(v3* V) {
    return sqrt(V->X * V->X + V->Y * V->Y + V->Z * V->Z);
}

void V3Normalize(v3* V) {
    float Length = V3Length(V);
    
    if (!Length) {
        V->X = 0.0f;
        V->Y = 0.0f;
        V->Z = 0.0f;
    } else {
        V->X = V->X / Length;
        V->Y = V->Y / Length;
        V->Z = V->Z / Length;
    }
}

v3 V3TransformCoord(v3* V, matrix* M) {
    
    float Norm = M->M[0][3] * V->X + M->M[1][3] * V->Y + M->M[2][3] * V->Z + M->M[3][3];
    
    return (v3) {
        (M->M[0][0] * V->X + M->M[1][0] * V->Y + M->M[2][0] * V->Z + M->M[3][0]) / Norm,
        (M->M[0][1] * V->X + M->M[1][1] * V->Y + M->M[2][1] * V->Z + M->M[3][1]) / Norm,
        (M->M[0][2] * V->X + M->M[1][2] * V->Y + M->M[2][2] * V->Z + M->M[3][2]) / Norm,
    };
}

v3 V3TransformNormal(v3* V, matrix* M) {
    
    return (v3) {
        M->M[0][0] * V->X + M->M[1][0] * V->Y + M->M[2][0] * V->Z,
        M->M[0][1] * V->X + M->M[1][1] * V->Y + M->M[2][1] * V->Z,
        M->M[0][2] * V->X + M->M[1][2] * V->Y + M->M[2][2] * V->Z
    };
}

matrix MatrixIdentity() {
    return (matrix){
        1.0f, 0.0f, 0.0f, 0.0f, 
        0.0f, 1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 1.0f, 0.0f, 
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

matrix MatrixTranslation(v3 V) {
    return (matrix){
        1.0f, 0.0f, 0.0f, 0.0f, 
        0.0f, 1.0f, 0.0f, 0.0f, 
        0.0f, 0.0f, 1.0f, 0.0f, 
        V.X,  V.Y,  V.Z,  1.0f
    };
}

matrix MatrixRotationZ(float AngleDegrees) {
    float Theta = DegreesToRadians(AngleDegrees);
    return (matrix){
        cos(Theta),  sin(Theta), 0.0f, 0.0f, 
        -sin(Theta), cos(Theta), 0.0f, 0.0f, 
        0.0f,        0.0f,       1.0f, 0.0f, 
        0.0f,        0.0f,       0.0f, 1.0f,
    };
}

matrix MatrixScale(v3 V) {
    return (matrix){
        V.X, 0.0f, 0.0f,  0.0f, 
        0.0f, V.Y, 0.0f,  0.0f, 
        0.0f, 0.0f, V.Z,  0.0f, 
        0.0f, 0.0f, 0.0f, 1.0f, 
    };
}

v3 MatrixV3Multiply(matrix M, v3 V) {
    
    float Temp[4] = {0};
    float VFloat[4] = {V.X, V.Y, V.Z};
    
    int i, j;
    for(j = 0; j < 4; ++j) {
        Temp[j] = 0.0f;
        for(i = 0; i < 4; ++i)
            Temp[j] += M.M[i][j] * VFloat[i];
    }
    
    return (v3){Temp[0], Temp[1], Temp[2]};
}

matrix MatrixMultiply(matrix* A, matrix* B) {
    return (matrix) {
        A->M[0][0] * B->M[0][0] + A->M[0][1] * B->M[1][0] + A->M[0][2] * B->M[2][0] + A->M[0][3] * B->M[3][0],
        A->M[0][0] * B->M[0][1] + A->M[0][1] * B->M[1][1] + A->M[0][2] * B->M[2][1] + A->M[0][3] * B->M[3][1],
        A->M[0][0] * B->M[0][2] + A->M[0][1] * B->M[1][2] + A->M[0][2] * B->M[2][2] + A->M[0][3] * B->M[3][2],
        A->M[0][0] * B->M[0][3] + A->M[0][1] * B->M[1][3] + A->M[0][2] * B->M[2][3] + A->M[0][3] * B->M[3][3],
        A->M[1][0] * B->M[0][0] + A->M[1][1] * B->M[1][0] + A->M[1][2] * B->M[2][0] + A->M[1][3] * B->M[3][0],
        A->M[1][0] * B->M[0][1] + A->M[1][1] * B->M[1][1] + A->M[1][2] * B->M[2][1] + A->M[1][3] * B->M[3][1],
        A->M[1][0] * B->M[0][2] + A->M[1][1] * B->M[1][2] + A->M[1][2] * B->M[2][2] + A->M[1][3] * B->M[3][2],
        A->M[1][0] * B->M[0][3] + A->M[1][1] * B->M[1][3] + A->M[1][2] * B->M[2][3] + A->M[1][3] * B->M[3][3],
        A->M[2][0] * B->M[0][0] + A->M[2][1] * B->M[1][0] + A->M[2][2] * B->M[2][0] + A->M[2][3] * B->M[3][0],
        A->M[2][0] * B->M[0][1] + A->M[2][1] * B->M[1][1] + A->M[2][2] * B->M[2][1] + A->M[2][3] * B->M[3][1],
        A->M[2][0] * B->M[0][2] + A->M[2][1] * B->M[1][2] + A->M[2][2] * B->M[2][2] + A->M[2][3] * B->M[3][2],
        A->M[2][0] * B->M[0][3] + A->M[2][1] * B->M[1][3] + A->M[2][2] * B->M[2][3] + A->M[2][3] * B->M[3][3],
        A->M[3][0] * B->M[0][0] + A->M[3][1] * B->M[1][0] + A->M[3][2] * B->M[2][0] + A->M[3][3] * B->M[3][0],
        A->M[3][0] * B->M[0][1] + A->M[3][1] * B->M[1][1] + A->M[3][2] * B->M[2][1] + A->M[3][3] * B->M[3][1],
        A->M[3][0] * B->M[0][2] + A->M[3][1] * B->M[1][2] + A->M[3][2] * B->M[2][2] + A->M[3][3] * B->M[3][2],
        A->M[3][0] * B->M[0][3] + A->M[3][1] * B->M[1][3] + A->M[3][2] * B->M[2][3] + A->M[3][3] * B->M[3][3],
    };
}

void MatrixInverse(matrix* Source, matrix* Target) {
    
    float Determinant;
    
    Target->M[0][0] =
        + Source->M[1][1] * Source->M[2][2] * Source->M[3][3]
        - Source->M[1][1] * Source->M[2][3] * Source->M[3][2]
        - Source->M[2][1] * Source->M[1][2] * Source->M[3][3]
        + Source->M[2][1] * Source->M[1][3] * Source->M[3][2]
        + Source->M[3][1] * Source->M[1][2] * Source->M[2][3]
        - Source->M[3][1] * Source->M[1][3] * Source->M[2][2];
    
    Target->M[0][1] =
        - Source->M[0][1] * Source->M[2][2] * Source->M[3][3]
        + Source->M[0][1] * Source->M[2][3] * Source->M[3][2]
        + Source->M[2][1] * Source->M[0][2] * Source->M[3][3]
        - Source->M[2][1] * Source->M[0][3] * Source->M[3][2]
        - Source->M[3][1] * Source->M[0][2] * Source->M[2][3]
        + Source->M[3][1] * Source->M[0][3] * Source->M[2][2];
    
    Target->M[0][2] =
        + Source->M[0][1] * Source->M[1][2] * Source->M[3][3]
        - Source->M[0][1] * Source->M[1][3] * Source->M[3][2]
        - Source->M[1][1] * Source->M[0][2] * Source->M[3][3]
        + Source->M[1][1] * Source->M[0][3] * Source->M[3][2]
        + Source->M[3][1] * Source->M[0][2] * Source->M[1][3]
        - Source->M[3][1] * Source->M[0][3] * Source->M[1][2];
    
    Target->M[0][3] =
        - Source->M[0][1] * Source->M[1][2] * Source->M[2][3]
        + Source->M[0][1] * Source->M[1][3] * Source->M[2][2]
        + Source->M[1][1] * Source->M[0][2] * Source->M[2][3]
        - Source->M[1][1] * Source->M[0][3] * Source->M[2][2]
        - Source->M[2][1] * Source->M[0][2] * Source->M[1][3]
        + Source->M[2][1] * Source->M[0][3] * Source->M[1][2];
    
    Target->M[1][0] =
        - Source->M[1][0] * Source->M[2][2] * Source->M[3][3]
        + Source->M[1][0] * Source->M[2][3] * Source->M[3][2]
        + Source->M[2][0] * Source->M[1][2] * Source->M[3][3]
        - Source->M[2][0] * Source->M[1][3] * Source->M[3][2]
        - Source->M[3][0] * Source->M[1][2] * Source->M[2][3]
        + Source->M[3][0] * Source->M[1][3] * Source->M[2][2];
    
    Target->M[1][1] =
        + Source->M[0][0] * Source->M[2][2] * Source->M[3][3]
        - Source->M[0][0] * Source->M[2][3] * Source->M[3][2]
        - Source->M[2][0] * Source->M[0][2] * Source->M[3][3]
        + Source->M[2][0] * Source->M[0][3] * Source->M[3][2]
        + Source->M[3][0] * Source->M[0][2] * Source->M[2][3]
        - Source->M[3][0] * Source->M[0][3] * Source->M[2][2];
    
    Target->M[1][2] =
        - Source->M[0][0] * Source->M[1][2] * Source->M[3][3]
        + Source->M[0][0] * Source->M[1][3] * Source->M[3][2]
        + Source->M[1][0] * Source->M[0][2] * Source->M[3][3]
        - Source->M[1][0] * Source->M[0][3] * Source->M[3][2]
        - Source->M[3][0] * Source->M[0][2] * Source->M[1][3]
        + Source->M[3][0] * Source->M[0][3] * Source->M[1][2];
    
    Target->M[1][3] =
        + Source->M[0][0] * Source->M[1][2] * Source->M[2][3]
        - Source->M[0][0] * Source->M[1][3] * Source->M[2][2]
        - Source->M[1][0] * Source->M[0][2] * Source->M[2][3]
        + Source->M[1][0] * Source->M[0][3] * Source->M[2][2]
        + Source->M[2][0] * Source->M[0][2] * Source->M[1][3]
        - Source->M[2][0] * Source->M[0][3] * Source->M[1][2];
    
    Target->M[2][0] =
        + Source->M[1][0] * Source->M[2][1] * Source->M[3][3]
        - Source->M[1][0] * Source->M[2][3] * Source->M[3][1]
        - Source->M[2][0] * Source->M[1][1] * Source->M[3][3]
        + Source->M[2][0] * Source->M[1][3] * Source->M[3][1]
        + Source->M[3][0] * Source->M[1][1] * Source->M[2][3]
        - Source->M[3][0] * Source->M[1][3] * Source->M[2][1];
    
    Target->M[2][1] =
        - Source->M[0][0] * Source->M[2][1] * Source->M[3][3]
        + Source->M[0][0] * Source->M[2][3] * Source->M[3][1]
        + Source->M[2][0] * Source->M[0][1] * Source->M[3][3]
        - Source->M[2][0] * Source->M[0][3] * Source->M[3][1]
        - Source->M[3][0] * Source->M[0][1] * Source->M[2][3]
        + Source->M[3][0] * Source->M[0][3] * Source->M[2][1];
    
    Target->M[2][2] =
        + Source->M[0][0] * Source->M[1][1] * Source->M[3][3]
        - Source->M[0][0] * Source->M[1][3] * Source->M[3][1]
        - Source->M[1][0] * Source->M[0][1] * Source->M[3][3]
        + Source->M[1][0] * Source->M[0][3] * Source->M[3][1]
        + Source->M[3][0] * Source->M[0][1] * Source->M[1][3]
        - Source->M[3][0] * Source->M[0][3] * Source->M[1][1];
    
    Target->M[2][3] =
        - Source->M[0][0] * Source->M[1][1] * Source->M[2][3]
        + Source->M[0][0] * Source->M[1][3] * Source->M[2][1]
        + Source->M[1][0] * Source->M[0][1] * Source->M[2][3]
        - Source->M[1][0] * Source->M[0][3] * Source->M[2][1]
        - Source->M[2][0] * Source->M[0][1] * Source->M[1][3]
        + Source->M[2][0] * Source->M[0][3] * Source->M[1][1];
    
    Target->M[3][0] =
        - Source->M[1][0] * Source->M[2][1] * Source->M[3][2]
        + Source->M[1][0] * Source->M[2][2] * Source->M[3][1]
        + Source->M[2][0] * Source->M[1][1] * Source->M[3][2]
        - Source->M[2][0] * Source->M[1][2] * Source->M[3][1]
        - Source->M[3][0] * Source->M[1][1] * Source->M[2][2]
        + Source->M[3][0] * Source->M[1][2] * Source->M[2][1];
    
    Target->M[3][1] =
        + Source->M[0][0] * Source->M[2][1] * Source->M[3][2]
        - Source->M[0][0] * Source->M[2][2] * Source->M[3][1]
        - Source->M[2][0] * Source->M[0][1] * Source->M[3][2]
        + Source->M[2][0] * Source->M[0][2] * Source->M[3][1]
        + Source->M[3][0] * Source->M[0][1] * Source->M[2][2]
        - Source->M[3][0] * Source->M[0][2] * Source->M[2][1];
    
    Target->M[3][2] =
        - Source->M[0][0] * Source->M[1][1] * Source->M[3][2]
        + Source->M[0][0] * Source->M[1][2] * Source->M[3][1]
        + Source->M[1][0] * Source->M[0][1] * Source->M[3][2]
        - Source->M[1][0] * Source->M[0][2] * Source->M[3][1]
        - Source->M[3][0] * Source->M[0][1] * Source->M[1][2]
        + Source->M[3][0] * Source->M[0][2] * Source->M[1][1];
    
    Target->M[3][3] =
        + Source->M[0][0] * Source->M[1][1] * Source->M[2][2]
        - Source->M[0][0] * Source->M[1][2] * Source->M[2][1]
        - Source->M[1][0] * Source->M[0][1] * Source->M[2][2]
        + Source->M[1][0] * Source->M[0][2] * Source->M[2][1]
        + Source->M[2][0] * Source->M[0][1] * Source->M[1][2]
        - Source->M[2][0] * Source->M[0][2] * Source->M[1][1];
    
    Determinant = + Source->M[0][0] * Target->M[0][0] + Source->M[0][1] * Target->M[1][0] + Source->M[0][2] * Target->M[2][0] + Source->M[0][3] * Target->M[3][0];
    
    Determinant = 1.0f / Determinant;
    
    Target->M[0][0] *= Determinant;
    Target->M[0][1] *= Determinant;
    Target->M[0][2] *= Determinant;
    Target->M[0][3] *= Determinant;
    Target->M[1][0] *= Determinant;
    Target->M[1][1] *= Determinant;
    Target->M[1][2] *= Determinant;
    Target->M[1][3] *= Determinant;
    Target->M[2][0] *= Determinant;
    Target->M[2][1] *= Determinant;
    Target->M[2][2] *= Determinant;
    Target->M[2][3] *= Determinant;
    Target->M[3][0] *= Determinant;
    Target->M[3][1] *= Determinant;
    Target->M[3][2] *= Determinant;
    Target->M[3][3] *= Determinant;
}

