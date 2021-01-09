
/*
Copyright (c) 2012-present Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#pragma once

#include "Common.h"
#include "MSectionSimple.h"
#include "MHandle.h"
#include "MEvent.h"
#include "MFileMapping.h"

#include <memory>
#include <unordered_map>

// Helper class to create CEDEFAULTTERMHOOK event and CEINSIDEMAPNAMEP mapping.
// Note! It's not thread-safe, if use in multithreading - protect it with section.
class CDefTermChildMap final
{
public:
	void CreateChildMapping(const DWORD childPid, const MHandle& childHandle, const DWORD conemuInsidePid)
	{
		auto data = std::make_unique<DefTermChildData>();

		data->defTermMark.InitName(CEDEFAULTTERMHOOK, childPid);
		if (data->defTermMark.Open(true))
		{
			data->defTermMark.Set();
		}

		data->hProcess = childHandle.Duplicate(SYNCHRONIZE);

		if (conemuInsidePid)
		{
			data->insideMapping.InitName(CEINSIDEMAPNAMEP, childPid);
			if (data->insideMapping.Create())
			{
				CONEMU_INSIDE_MAPPING info{};
				info.cbSize = sizeof(info);
				info.nProtocolVersion = CESERVER_REQ_VER;
				info.nGuiPID = conemuInsidePid;
				data->insideMapping.SetFrom(&info);
			}
		}

		for (auto iter = childData_.begin(); iter != childData_.end();)
		{
			const bool terminated = iter->second->hProcess.HasHandle()
				? (WaitForSingleObject(iter->second->hProcess.GetHandle(), 0) == WAIT_OBJECT_0)
				: true; // if we don't have a handle - let's clean the record on next invocation
			if (terminated)
				iter = childData_.erase(iter);
			else
				++iter;
		}

		childData_[childPid] = std::move(data);
	}

private:
	struct DefTermChildData
	{
		MEvent defTermMark; // CEDEFAULTTERMHOOK
		MFileMapping<CONEMU_INSIDE_MAPPING> insideMapping; // CEINSIDEMAPNAMEP
		MHandle hProcess; // handle with SYNCHRONIZE access right
		DWORD pid = 0; // started child process id (informational)
	};

	std::unordered_map<DWORD, std::unique_ptr<DefTermChildData>> childData_{};
};
