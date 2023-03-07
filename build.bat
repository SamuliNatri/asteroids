@echo off
cl main.c ^
/Fea.exe /Zi /nologo ^
/link ^
user32.lib d3d11.lib d3dcompiler.lib dxguid.lib  