#include <stdio.h>
#include <stdlib.h>
#include "./ku_mmu.h"

char ku_traverse(void *ku_cr3, char va);

void ku_mmu_fin(FILE *fd, void *pmem)
{
	if(fd) fclose(fd);
	if(pmem) free(pmem);
}

int main(int argc, char *argv[])
{
	FILE *fd=NULL;
	char fpid, pid=0, va, pa;
	unsigned int pmem_size, swap_size;
	void *ku_cr3, *pmem=NULL;

	if(argc != 4){
		printf("ku_cpu: Wrong number of arguments\n");
		return 1;
	}

	fd = fopen(argv[1], "r");
	if(!fd){
		printf("ku_cpu: Fail to open the input file\n");
		return 1;
	}

	pmem_size = strtol(argv[2], NULL, 10);
	swap_size = strtol(argv[3], NULL, 10);
	pmem = ku_mmu_init(pmem_size, swap_size);
	if(!pmem){
		printf("ku_cpu: Fail to allocate the physical memory\n");
		ku_mmu_fin(fd, pmem);
		return 1;
	}

	while(fscanf(fd, "%hhd %hhd", &fpid, &va) != EOF){

		if(pid != fpid){ // 이전의 pid 와 다른 pid 일 경우
			// 새로운 Process 생성 단, fork 하지는 않고 새로운 PCB를 생성해줍니다.
			if(ku_run_proc(fpid, &ku_cr3) == 0)  // ku_cr3 : PDBR 을 바꿔주어야 합니다. 이는 PCB 안에 저장됩니다. 
				pid = fpid; /* context switch */
			else{
				printf("ku_cpu: Context switch is failed\n");
				ku_mmu_fin(fd, pmem); // 할당된 자원들을 해제시켜 줍니다.
				return 1;
			} 
		}

		pa = ku_traverse(ku_cr3, va); // Address translation
		if(pa == 0){
			if(ku_page_fault(pid, va) != 0){ 
				printf("ku_cpu: Fault handler is failed\n");
				ku_mmu_fin(fd, pmem);
				return 1;
			}
			printf("[%d] VA: %hhd -> Page Fault\n", pid, va);

			/* Retry after page fault */
			pa = ku_traverse(ku_cr3, va); 
			if(pa == 0){
				printf("ku_cpu: Addr translation is failed\n");
				ku_mmu_fin(fd, pmem);
				return 1;
			}
		}

		printf("[%d] VA: %hhd -> PA: %hhd\n", pid, va, pa);
	}

	ku_mmu_fin(fd, pmem);
	return 0;
}
