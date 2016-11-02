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

	clReleaseCommandQueue(_cmdQueue);
	clReleaseContext(_context);

	return void();
}

const size_t IOpenCl::AddProgram(const char * path)
{
	// Create the program
	cl_program program = CreateProgram(path,
		_context);

	cl_int err;
	err = clBuildProgram(program, 1, &_deviceInfo[0].id, nullptr, nullptr, nullptr);
	CL_ERR(err, "clBuildProgram");

	cl_kernel kernel = clCreateKernel(program, path, &err);
	CL_ERR(err, "clCreateKernel");
	_programs.push_back(kernel);

	return _programs.size() - 1;

}
cl_program IOpenCl::CreateProgram(const char* path, cl_context context)
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


