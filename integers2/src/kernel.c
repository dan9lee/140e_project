#include <stddef.h>
#include <stdint.h>

void kernel_main(int *arr, int n, int index, int c)
{
	for(int i=index; i<n; i+=c){
		arr[i] = i*i;
	}
}
