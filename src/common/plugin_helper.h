
#pragma once

#define INIT_FAR_PSI(psi,fsf,info) \
	psi = (PluginStartupInfo*)calloc(sizeof(*psi),1); \
	fsf = (FarStandardFunctions*)calloc(sizeof(*fsf),1); \
	if (psi == NULL || fsf == NULL) \
	{ \
		_ASSERTE(psi && fsf); \
		return; \
	} \
	memmove(psi, info, min(sizeof(*psi),(size_t)(info)->StructSize)); \
	memmove(fsf, (info)->FSF, min(sizeof(*fsf),(size_t)(info)->FSF->StructSize)); \
	psi->FSF = fsf;
