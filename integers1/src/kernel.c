#include <stddef.h>
#include <stdint.h>

void kernel_main(int *arr, int n, int index, int c)
{
	for(int i=index; i<n; i+=c){
		int sum = 0;
		for(int j=i; j< i + 100;j++){
			sum += j;
		}
		arr[i] = sum;
	}
}
