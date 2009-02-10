#pragma once

#include "../../common/pluginA.hpp"

extern PluginStartupInfo psi;
extern FarStandardFunctions fsf;

typedef void (WINAPI* FClosePlugin)(HANDLE hPlugin);
void WINAPI ClosePlugin(HANDLE hPlugin);

typedef int (WINAPI* FConfigure)(int ItemNumber);
int WINAPI Configure(int ItemNumber);

typedef void (WINAPI* FExitFAR)(void);
void WINAPI ExitFAR(void);

typedef int (WINAPI* FGetMinFarVersion)(void);
int WINAPI GetMinFarVersion(void);

typedef void (WINAPI* FGetPluginInfo)(struct PluginInfo *Info);
void WINAPI GetPluginInfo(struct PluginInfo *Info);

typedef HANDLE (WINAPI* FOpenFilePlugin)(char *Name,const unsigned char *Data,int DataSize);
HANDLE WINAPI OpenFilePlugin(char *Name,const unsigned char *Data,int DataSize);

typedef HANDLE (WINAPI* FOpenPlugin)(int OpenFrom,INT_PTR Item);
HANDLE WINAPI OpenPlugin(int OpenFrom,INT_PTR Item);

typedef int (WINAPI* FProcessEditorEvent)(int Event,void *Param);
int WINAPI ProcessEditorEvent(int Event,void *Param);

typedef int (WINAPI* FProcessViewerEvent)(int Event,void *Param);
int WINAPI ProcessViewerEvent(int Event,void *Param);

typedef void (WINAPI* FSetStartupInfo)(const struct PluginStartupInfo *Info);
void WINAPI SetStartupInfo(const struct PluginStartupInfo *Info);

// Wrapper
typedef INT_PTR (WINAPI* FWrapperAdvControl)(int ModuleNumber,int Command,void *Param);
INT_PTR WINAPI WrapperAdvControl(int ModuleNumber,int Command,void *Param);

// Service
HWND AtoH(TCHAR *Str, int Len);
