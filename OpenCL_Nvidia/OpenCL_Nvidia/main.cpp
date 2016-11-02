
#include <CL\cl.h>
#include <iostream>
#include <vector>
#include <random>
#include <fstream>
#include <string>
inline void checkErr(cl_int err, const char * name) {
	if (err != CL_SUCCESS) {
		std::cout << "ERROR: " << name << " (" << err << ")" << std::endl;
		std::getchar();
		exit(EXIT_FAILURE);
	}
}

struct Matrix
{
	uint32_t size;
	float* e;
	Matrix(uint32_t size, float* data) : size(size), e(data)
	{

	}
	Matrix(uint32_t size) : size(size), e(nullptr)
	{
		e = new float[size*size];
		for (uint32_t i = 0; i < size; i++)
		{
			for (uint32_t j = 0; j < size; j++)
			{
				e[j*size + i] = (rand()%100)/100.0f;
			}
		}
	}
	~Matrix()
	{
		delete[] e;
	}

	void Print(size_t x, size_t y)
	{
		for (size_t i = 0; i < x; i++)
		{
			for (size_t j = 0; j < y; j++)
				std::cout << e[i*size + j] << " ";
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
};

#define clErr(x) checkErr(err, x)

std::string GetPlatformName(cl_platform_id id)
{
	size_t size = 0;
	clGetPlatformInfo(id, CL_PLATFORM_NAME, 0, nullptr, &size);

	std::string result;
	result.resize(size);
	clGetPlatformInfo(id, CL_PLATFORM_NAME, size,
		const_cast<char*> (result.data()), nullptr);

	return result;
}

std::string GetDeviceName(cl_device_id id)
{
	size_t size = 0;
	clGetDeviceInfo(id, CL_DEVICE_NAME, 0, nullptr, &size);

	std::string result;
	result.resize(size);
	clGetDeviceInfo(id, CL_DEVICE_NAME, size,
		const_cast<char*> (result.data()), nullptr);

	return result;
}
cl_ulong GetGlobalMemory(cl_device_id id)
{
	cl_ulong result;
	clGetDeviceInfo(id, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong),
		&result, nullptr);

	return result;
}
size_t GetMaxBlocks(cl_device_id id)
{
	size_t result;
	clGetDeviceInfo(id, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t),
		&result, nullptr);

	return result;
}
cl_uint GetMaxDimensions(cl_device_id id)
{
	cl_uint result;
	clGetDeviceInfo(id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint),
		&result, nullptr);

	return result;
}
size_t* GetMaxThreads(cl_device_id id, cl_uint maxDim)
{
	size_t* result = new size_t[maxDim];
	clGetDeviceInfo(id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t)*maxDim,
		result, nullptr);

	return result;
}
void PrintDeviceInfo(cl_device_id id)
{
	std::cout << "Device Name: " << GetDeviceName(id) << std::endl;
	std::cout << "\tGlobal Memory: " << GetGlobalMemory(id) << std::endl;
	std::cout << "\tMax Work Group Size: " << GetMaxBlocks(id) << std::endl;

	cl_uint maxDim = GetMaxDimensions(id);
	std::cout << "\tMax Dimensions: " << maxDim << std::endl;
	size_t* maxThreads = GetMaxThreads(id, maxDim);

	std::cout << "\tMax threads: ";
	std::cout << "( " << maxThreads[0] << " ";
	for (cl_uint i = 1; i < maxDim; i++)
	{
		std::cout << ", " << maxThreads[i] << " ";
	}
	std::cout << ")" << std::endl;
	delete maxThreads;
}
std::string LoadKernel(const char* name)
{
	std::ifstream in(name);
	std::string result(
		(std::istreambuf_iterator<char>(in)),
		std::istreambuf_iterator<char>());
	return result;
}

cl_program CreateProgram(const std::string& source,
	cl_context context)
{
	size_t lengths[1] = { source.size() };
	const char* sources[1] = { source.data() };

	cl_int err = 0;
	cl_program program = clCreateProgramWithSource(context, 1, sources, lengths, &err);
	clErr("clCreateProgramWithSource");

	return program;
}



int main()
{
	srand(1337);
	cl_int err;

	// Get various information
	cl_uint platformIdCount = 0;
	clGetPlatformIDs(0, nullptr, &platformIdCount);
	checkErr(platformIdCount != 0 ? CL_SUCCESS : -1, "No platforms found");

	std::vector<cl_platform_id> platformIds(platformIdCount);
	clGetPlatformIDs(platformIdCount, platformIds.data(), nullptr);

	//std::string name = GetPlatformName(platformIds[0]);

	cl_uint deviceIdCount = 0;

	clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, 0, nullptr,
		&deviceIdCount);

	checkErr(deviceIdCount != 0 ? CL_SUCCESS : -1, "No devices found");

	std::vector<cl_device_id> deviceIds(deviceIdCount);
	clGetDeviceIDs(platformIds[0], CL_DEVICE_TYPE_GPU, deviceIdCount,
		deviceIds.data(), nullptr);

//	std::string name = GetDeviceName(deviceIds[0]);
	PrintDeviceInfo(deviceIds[0]);
	// Create a context
	const cl_context_properties contextProperties[] =
	{
		CL_CONTEXT_PLATFORM,
		reinterpret_cast<cl_context_properties> (platformIds[0]),
		0, 0
	};

	cl_context context = clCreateContext(
		contextProperties, deviceIdCount,
		deviceIds.data(), nullptr,
		nullptr, &err);
	clErr("clCreateContext");

	// Create command que
	cl_command_queue queue = clCreateCommandQueue(context, deviceIds[0], 0, &err);
	clErr("clCreateCommandQueue");

	// Create the program
	cl_program program = CreateProgram(LoadKernel("dMult.cl"),
		context);

	err = clBuildProgram(program, deviceIdCount, deviceIds.data(), nullptr, nullptr, nullptr);
	clErr("clBuildProgram");

	cl_kernel dMult = clCreateKernel(program, "dMult", &err);
	clErr("clCreateKernel");

	// Create the data
#define Width 32
	Matrix a(Width);
	Matrix b(Width);
	Matrix c(Width, new float[1024 * 1024]);

	cl_mem aBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float)* Width * Width, nullptr, &err);
	clErr("clCreateBuffer");
	err = clEnqueueWriteBuffer(queue, aBuffer, CL_TRUE, 0, sizeof(float) * Width * Width, a.e, NULL, nullptr, nullptr);
	clErr("clEnqueueWriteBuffer");
	
	cl_mem bBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * Width * Width, nullptr, &err);
	clErr("clCreateBuffer");
	err = clEnqueueWriteBuffer(queue, bBuffer, CL_TRUE, 0, sizeof(float) * Width * Width, b.e, NULL, nullptr, nullptr);
	clErr("clEnqueueWriteBuffer");

	cl_mem cBuffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * Width * Width, nullptr, &err);
	clErr("clCreateBuffer");

	// Set args for program
	clSetKernelArg(dMult, 0, sizeof(cl_mem), &aBuffer);
	clSetKernelArg(dMult, 1, sizeof(cl_mem), &bBuffer);
	clSetKernelArg(dMult, 2, sizeof(cl_mem), &cBuffer);

	// Execute the program
	const size_t globalWorkSize[] = { Width, Width, 0}; // Total size
	const size_t localWorkSize[] = { 8, 8, 0 }; // Threads
	err = clEnqueueNDRangeKernel(queue, dMult, 2, nullptr, globalWorkSize, localWorkSize, NULL, nullptr, nullptr);
	clErr("clEnqueueNDRangeKernel");


	// Get the calculated data
	err = clEnqueueReadBuffer(queue, cBuffer, CL_TRUE, 0,
		sizeof(float) * Width * Width,
		c.e,
		0, nullptr, nullptr);
	clErr("clEnqueueReadBuffer");


	a.Print(Width, Width);
	b.Print(Width, Width);
	c.Print(Width, Width);

	// Cleanup
	clReleaseCommandQueue(queue);
	clReleaseMemObject(aBuffer);
	clReleaseMemObject(bBuffer);
	clReleaseMemObject(cBuffer);

	clReleaseKernel(dMult);
	clReleaseProgram(program);

	clReleaseContext(context);

	std::getchar();
	return 0;
}
