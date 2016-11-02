#pragma once
#ifndef _I_OPEN_CL_H_
#define _I_OPEN_CL_H_

#include <CL\cl.h>
#include <vector>


class IOpenCl
{
	struct DeviceInfo
	{
		DeviceInfo(cl_device_id id) : id(id), maxDimensions(3)
		{

		}
		cl_device_id id;
		cl_ulong totalGlobalMemory;
		size_t maxWorkGroupSize;
		cl_uint maxDimensions;
		size_t maxThreads[3];
	};
public:
	IOpenCl();
	~IOpenCl();

	const void Init();
	const void Shutdown();

	const size_t AddProgram(const char* path);

protected:
	cl_context _context;
	cl_command_queue _cmdQueue;
	std::vector<cl_kernel> _programs;
	std::vector<DeviceInfo> _deviceInfo;


	void _SetGlobalMemory(DeviceInfo& info);
	void _SetMaxBlocks(DeviceInfo& info);
	void _SetMaxDimensions(DeviceInfo& info);
	void _SetMaxThreads(DeviceInfo& info);


	cl_program CreateProgram(const char* path, cl_context context);

};

#endif