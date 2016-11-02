#include "IOpenCl.h"
#include <string>
#include <fstream>

static const void CreateException(const char* msg, cl_int err, int line)
{
	std::string out = "Error: ";
	out += msg;
	out += ", (" + std::to_string(err) + "), At line: " + std::to_string(line);
	throw out;
}

#define CL_ERR(err, msg) if (err != CL_SUCCESS) CreateException(msg, err, __LINE__)


IOpenCl::IOpenCl()
{
}


IOpenCl::~IOpenCl()
{
}

const void IOpenCl::Init()
{
	// Get various information
	cl_int err;
	cl_uint platformIdCount = 0;
	clGetPlatformIDs(0, nullptr, &platformIdCount);
	CL_ERR(platformIdCount != 0 ? CL_SUCCESS : -1, "No platforms found");

	std::vector<cl_platform_id> platformIds(platformIdCount);
	clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);

	//std::string name = GetPlatformName(platformIds[0]);

	cl_uint deviceIdCount = 0;

	clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, 0, nullptr,
		&deviceIdCount);

	CL_ERR(deviceIdCount != 0 ? CL_SUCCESS : -1, "No devices found");

	std::vector<cl_device_id> deviceIds(deviceIdCount);
	clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, deviceIdCount,
		deviceIds.data(), nullptr);

	// Fill the deviceinfo with data.
	for (auto& id : deviceIds)
	{
		DeviceInfo di(id);
		_SetGlobalMemory(di);
		_SetMaxBlocks(di);
		_SetMaxDimensions(di);
		_SetMaxThreads(di);
		_deviceInfo.push_back(di);		
	}



	// Create a context
	const cl_context_properties contextProperties[] =
	{
		CL_CONTEXT_PLATFORM,
		reinterpret_cast<cl_context_properties> (platformIds[0]),
		0, 0
	};

	_context = clCreateContext(
		contextProperties, deviceIdCount,
		deviceIds.data(), nullptr,
		nullptr, &err);
	CL_ERR(err, "clCreateContext");

	// Create command que
	_cmdQueue = clCreateCommandQueue(_context, deviceIds[0], 0, &err);
	CL_ERR(err, "clCreateCommandQueue");
}

const void IOpenCl::Shutdown()
{

	for (auto& p : _programs)
	{
		clReleaseKernel(p.kernel);
		clReleaseProgram(p.program);
	}

	clReleaseCommandQueue(_cmdQueue);
	clReleaseContext(_context);

	return void();
}

const size_t IOpenCl::AddProgram(const char * path)
{
	// Create the program
	cl_program program = _CreateProgram(path,
		_context);

	cl_int err;
	err = clBuildProgram(program, 1, &_deviceInfo[0].id, nullptr, nullptr, nullptr);
	CL_ERR(err, "clBuildProgram");

	cl_kernel kernel = clCreateKernel(program, path, &err);
	CL_ERR(err, "clCreateKernel");
	_programs.push_back({ kernel, program });

	return _programs.size() - 1;

}
const IOpenCl::Param IOpenCl::GetParamFromBuffer(uint32_t buffer)
{
	auto& find = _buffers.find(buffer);
	if (find != _buffers.end())
	{
		Param ret = { sizeof(cl_mem), &(find->second) };
		return ret;
	}
	throw "Buffer not found.";
}
const void IOpenCl::ExecuteProgram(size_t program, uint8_t numParam, const Param* param, cl_uint numDim, const size_t* globalWorkSize, const size_t* localWorkSize)
{
	cl_kernel& k = _programs[program].kernel;
	cl_int err;
	for (uint8_t i = 0; i < numParam; i++)
	{
		err = clSetKernelArg(k, i, param[i].byteSize, param[i].data);
		CL_ERR(err, "clSetKernelArg");
	}
	
	 err = clEnqueueNDRangeKernel(_cmdQueue, k, numDim, nullptr, globalWorkSize, localWorkSize, NULL, nullptr, nullptr);
	 CL_ERR(err, "clEnqueueNDRangeKernel");

	return void();
}
const size_t IOpenCl::CreateBuffer(size_t byteSize, cl_mem_flags flags, void* data)
{
	cl_int err;
	cl_mem buff = clCreateBuffer(_context, flags, byteSize, data, &err);
	CL_ERR(err, "clCreateBuffer");
	
	uint32_t nid = 0;
	auto& find = _buffers.find(nid);
	while (find != _buffers.end())
	{		
		nid++;
		find = _buffers.find(nid);
	}
	_buffers[nid] = buff;

	return nid;
}
const void IOpenCl::CopyHostToDevice(uint32_t buffer, size_t byteSize, void * data)
{
	auto& find = _buffers.find(buffer);
	if (find != _buffers.end())
	{
		cl_int err = clEnqueueWriteBuffer(_cmdQueue, find->second, CL_TRUE, 0, byteSize, data, NULL, nullptr, nullptr);
		CL_ERR(err, "clEnqueueWriteBuffer");
	}
	throw "Buffer not found.";
}
const void IOpenCl::CopyDeviceToHost(uint32_t buffer, size_t byteSize, void * data)
{
	auto& find = _buffers.find(buffer);
	if (find != _buffers.end())
	{
		cl_int err = clEnqueueReadBuffer(_cmdQueue, find->second, CL_TRUE, 0,
			byteSize,
			data,
			0, nullptr, nullptr);
		CL_ERR(err, "clEnqueueReadBuffer");
	}
	
	throw "Buffer not found.";
}
cl_program IOpenCl::_CreateProgram(const char* path, cl_context context)
{

	std::ifstream in(path);
	std::string result(
		(std::istreambuf_iterator<char>(in)),
		std::istreambuf_iterator<char>());

	size_t lengths[1] = { result.size() };
	const char* sources[1] = { result.data() };

	cl_int err = 0;
	cl_program program = clCreateProgramWithSource(context, 1, sources, lengths, &err);
	CL_ERR(err, "clCreateProgramWithSource");

	return program;
}
void IOpenCl::_SetGlobalMemory(DeviceInfo& info)
{
	clGetDeviceInfo(info.id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong),
		&info.totalGlobalMemory, nullptr);
}

void IOpenCl::_SetMaxBlocks(DeviceInfo& info)
{
	clGetDeviceInfo(info.id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t),
		&info.maxWorkGroupSize, nullptr);
	
}

void IOpenCl::_SetMaxDimensions(DeviceInfo& info)
{
	clGetDeviceInfo(info.id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint),
		&info.maxDimensions, nullptr);
	info.maxDimensions = (info.maxDimensions > 3) ? 3 : info.maxDimensions;
}

void IOpenCl::_SetMaxThreads(DeviceInfo& info)
{
	clGetDeviceInfo(info.id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t)*info.maxDimensions,
		&info.maxThreads, nullptr);
}


