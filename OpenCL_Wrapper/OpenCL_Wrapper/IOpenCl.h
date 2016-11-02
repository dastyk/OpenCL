#pragma once
#ifndef _I_OPEN_CL_H_
#define _I_OPEN_CL_H_

#include <CL\cl.h>
#include <vector>
#include <map>

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
	struct Program
	{
		cl_kernel kernel;
		cl_program program;
		Program(cl_kernel kernel, cl_program program) : kernel(kernel), program(program)
		{

		}
	};
public:
	struct Param
	{
		size_t byteSize;
		void* data;
	};

	IOpenCl();
	~IOpenCl();

	const void Init();
	const void Shutdown();

	const size_t AddProgram(const char* path);
	const Param GetParamFromBuffer(uint32_t buffer);
	const void ExecuteProgram(size_t program, uint8_t numParam, const Param* param, cl_uint numDim, const size_t* globalWorkSize, const size_t* localWorkSize);
	const size_t CreateBuffer(size_t byteSize, cl_mem_flags flags = CL_MEM_ALLOC_HOST_PTR | CL_MEM_READ_ONLY, void* data = nullptr);
	const void CopyHostToDevice(uint32_t buffer, size_t byteSize, void* data);
	const void CopyDeviceToHost(uint32_t buffer, size_t byteSize, void* data);
protected:
	cl_context _context;
	cl_command_queue _cmdQueue;
	std::vector<Program> _programs;
	std::vector<DeviceInfo> _deviceInfo;
	std::map<uint32_t, cl_mem> _buffers;

	void _SetGlobalMemory(DeviceInfo& info);
	void _SetMaxBlocks(DeviceInfo& info);
	void _SetMaxDimensions(DeviceInfo& info);
	void _SetMaxThreads(DeviceInfo& info);


	cl_program _CreateProgram(const char* path, cl_context context);

};

#endif