
__kernel void dMult(__global float*a, __global float*b, __global float*c)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	int index = y*get_global_size(0) + x;
	c[index] =  a[index] + b[index];
}