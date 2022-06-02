// TLB 에 페이지 정보가 없을 시 PDBR ( page directory base register) 를 찾아봐야 한다. -> page table 을 찾아볼 수 있다.
// 즉, PDBR 은 Page table 의 시작주소를 갖는다
// Page table 에 Mapping 이 되지 않은 Virtual address 에 접근하려고 할 시 Page Fault 발생  
// Page Fault 가 발생할 시 Free list 에서 page 한개를 가져와 Mapping 을 합니다.
// Freelist 가 없을 Page 한개를 선정하여 Swap out 해주고 새로 Mapping 을 합니다. -> Page table 업데이트 
// 8bit addressing : 6 bit VPN , 2 bit Offset 
// address space 256byte , page size 4 bytes , PTE 1 byte
// PDBR (CR3) 은 Linear Page Table 의 시작주소를 가리킨다.
// 1 byte PTE : 6bit PFN , 1bit User bit , 1bit Present bit
// swap space offset : 7bit offset , 1bit present bit ( swap out 시 present bit = 0 )
// swap space 의 0 번째 table 은 사용하지 않습니다. 


#include<stdbool.h>
#include <stdlib.h>
#include <string.h>

#define PAGE_SIZE 4
#define PRESENT_BIT_MASK 0b00000001
#define PRESENT_BIT 0b00000001
#define NOT_PRESENT 0b00000000
#define VPN_MASK 0b11111100
#define VPN_SHIFT 2
#define PFN_MASK 0b11111100
#define PFN_SHIFT 2
#define NOT_MAPPING 0b00000000
#define SWAP_OFFSET_MASK 0b11111110
#define SWAP_OFFSET_SHIFT 1

// PCB 구조체
typedef struct PCB{
    char pid;
    void* pdbr;
    struct PCB *next;
}pcb;

// Node 구조체
typedef struct Node{
    void* address;
    struct Node *next;
}node;

// MappingNode 구조체
typedef struct MappingNode{
    char* pte;
    struct MappingNode *next;
}mappingNode;

void* ku_mmu_pmem_address; // physical memory 시작주소
void* ku_mmu_swap_address; // swap swpace 시작 주소
pcb* ku_mmu_pcbList = NULL; // pcb 관리 리스트
node* ku_mmu_freeList = NULL; // free 리스트
node* ku_mmu_swapList = NULL; // swap space 리스트 
mappingNode* ku_mmu_mappingList = NULL; // page단위로 pte가 들어감

// process control block 생성구간
bool isExistPCB(char _pid){
    pcb* curr = ku_mmu_pcbList;
    while(curr != NULL && curr->pid != _pid) curr = curr->next;
    if(curr != NULL && curr->pid == _pid) return true;
    else return false;
}

void* getPDBR(char _pid){
    pcb* curr = ku_mmu_pcbList;
    while(curr->pid != _pid) curr = curr->next;
    return curr->pdbr;
}

// free list 관련 함수
void initFreeList(unsigned int _pmem_size){ // 최초에 freeList 생성 : 모든 physical memory를 페이지 단위로 나눠서 freeList에 넣는다
    node* curr = (node*) malloc(sizeof(node));
    curr->address = ku_mmu_pmem_address;
    curr->next = NULL;

    ku_mmu_freeList = curr;
    
    for(int i=PAGE_SIZE; i<_pmem_size; i+=PAGE_SIZE){
        node* tmpNode = (node*)malloc(sizeof(node));
        tmpNode->address = ku_mmu_pmem_address+i;
        tmpNode->next = NULL;
        curr->next = tmpNode;
        curr = curr->next;
    }
}

bool isFreeListEmpty(){
    if(ku_mmu_freeList == NULL) return true;
    else return false;
}

void* freeListDequeue(){
    node* curr = ku_mmu_freeList;
    void* address = NULL;

    address = curr->address;
    
    ku_mmu_freeList = ku_mmu_freeList->next;
    free(curr);
    return address;
}



// swap 관련 함수
void initSwapSpaceList(unsigned int _swap_size){
    ku_mmu_swapList = (node*)malloc(sizeof(node));
    ku_mmu_swapList->address = ku_mmu_swap_address+PAGE_SIZE;
    ku_mmu_swapList->next = NULL;

    node* curr = ku_mmu_swapList;
    for(int i=PAGE_SIZE+PAGE_SIZE; i<_swap_size; i+=PAGE_SIZE){
        node* tmpNode = (node*)malloc(sizeof(node));
        tmpNode->address = ku_mmu_swap_address+i;
        tmpNode->next = NULL;
        curr->next = tmpNode;
        curr = curr->next;
    }
}

bool isSwapSpaceEmpty(){
    if(ku_mmu_swapList==NULL) return true;
    else return false;
}

void* ku_mmu_swapListDequeue(){
    node* curr = ku_mmu_swapList;
    void* address = NULL;

    address = curr->address;

    ku_mmu_swapList = ku_mmu_swapList->next;
    free(curr);
    return address;
}



// mapping /////////////////////////////////////
bool isMappingListEmpty(){
    if(ku_mmu_mappingList == NULL) return true;
    else return false;
}

void mappingListEnqueue(char* pte){
    if(ku_mmu_mappingList==NULL){
        ku_mmu_mappingList = (mappingNode*)malloc(sizeof(mappingNode));
        ku_mmu_mappingList->pte = pte;
        ku_mmu_mappingList->next = NULL;
        return;
    }

    else{
        mappingNode* curr = ku_mmu_mappingList;
        while(curr->next != NULL){
            curr = curr->next;
        }
        mappingNode* tmpNode = (mappingNode*)malloc(sizeof(mappingNode));
        tmpNode->pte = pte;
        tmpNode->next = NULL;
        curr-> next = tmpNode;
    }
}

void mappingListDequeue(char** _pte){
    mappingNode* curr = ku_mmu_mappingList;
    *_pte = curr->pte;

    ku_mmu_mappingList = ku_mmu_mappingList->next;
    free(curr);
}


void* ku_mmu_init(unsigned int _pmem_size, unsigned int _swap_size){
    ku_mmu_pmem_address = malloc(_pmem_size); // physical memory 할당
    if(ku_mmu_pmem_address == NULL) return 0;
    memset(ku_mmu_pmem_address, 0, _pmem_size); // 0으로 초기화

    ku_mmu_swap_address = malloc(_swap_size); // swap space 할당
    if(ku_mmu_swap_address == NULL) return 0;
    memset(ku_mmu_swap_address, 0, _swap_size); // 0으로 초기화

    initFreeList(_pmem_size); // ku_mmu_freeList 생성
    initSwapSpaceList(_swap_size); // ku_mmu_swapList 생성
    return ku_mmu_pmem_address;
}



int ku_run_proc(char _fpid, void **_ku_cr3){
    if(ku_mmu_pcbList == NULL){
        //작업 만들고 넣기
        pcb * newProcess = (pcb*)malloc(sizeof(pcb));
        newProcess->pid = _fpid;
        newProcess->next = NULL;

        if(isFreeListEmpty()){ // freeList에 빈공간이 없을때
            if(isSwapSpaceEmpty()) return -1;
            void* swapSpace = NULL;
            swapSpace = ku_mmu_swapListDequeue(); // swap space 하나 가져옴
            
            if(isMappingListEmpty()) return -1;

            char* charPte;
            mappingListDequeue(&charPte);
            char pte = *charPte;

            int pfNumber = (pte & PFN_MASK) >> PFN_SHIFT;
            void*changeAddress = ku_mmu_pmem_address + (pfNumber*PAGE_SIZE);

            memcpy(swapSpace, changeAddress, PAGE_SIZE); // dest, src, size
            memset(changeAddress, 0, PAGE_SIZE); 

            pte = (((swapSpace - ku_mmu_swap_address)/PAGE_SIZE) << 1) & 0b11111110;
            *charPte = pte;

            newProcess->pdbr = changeAddress;
            // pdbr이라 swap out 되지 않아서 mapping list에 추가하지 않음
        }
        else{ // freeList에 빈공간이 있을때
            newProcess->pdbr = freeListDequeue();
        }
        ku_mmu_pcbList = newProcess;
        *_ku_cr3 = ku_mmu_pcbList->pdbr;
        return 0;
    }
    else{
        // pcb찾고 있을때
        if(isExistPCB(_fpid)){ // 이미 등록된 PCB 일때
            void* changePDBR = NULL;
            changePDBR = getPDBR(_fpid);
            *_ku_cr3 = changePDBR;
            return 0;
        }
        // pcb 찾고 없을때
        else{
            pcb * newProcess = (pcb*)malloc(sizeof(pcb));
            newProcess->pid = _fpid;
            newProcess->next = NULL;

            if(isFreeListEmpty()){ // freeList에 빈공간이 없을때
                if(isSwapSpaceEmpty()) return -1;
                void* swapSpace = NULL;
                swapSpace = ku_mmu_swapListDequeue(); // swap space 하나 가져옴
                
                if(isMappingListEmpty()) return -1;

                char* charPte;
                mappingListDequeue(&charPte);
                char pte = *charPte;

                int pfNumber = (pte & 0b11111100) >> 2;
                void*changeAddress = ku_mmu_pmem_address + (pfNumber*PAGE_SIZE);

                memcpy(swapSpace, changeAddress, PAGE_SIZE); // dest, src, size
                memset(changeAddress, 0, PAGE_SIZE); 

                pte = (((swapSpace - ku_mmu_swap_address)/PAGE_SIZE) << 1) & 0b11111110;
                *charPte = pte;

                newProcess->pdbr = changeAddress;
            }
            else{ // freeList에 빈공간이 있을때
                newProcess->pdbr = freeListDequeue();
            }
            pcb* curr = ku_mmu_pcbList;
            while(curr->next !=NULL) curr = curr->next;
            curr->next = newProcess;
            *_ku_cr3 = curr->next->pdbr;
            return 0;
        }
    }
    return 1;
}




// ////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////


int ku_page_fault(char _pid, char _va){
    void* pdbr = getPDBR(_pid); // 해당 pid없을경우?
    void* pageAddress;
    void* ptIdx = (_va & VPN_MASK) >> VPN_SHIFT;
    char pte = *(char*) ptIdx;
    char ptePresentBit = pte & PRESENT_BIT_MASK;
    
    if(ptePresentBit == NOT_PRESENT){
        if(pte == NOT_MAPPING){
            if(isFreeListEmpty()){
                if(isSwapSpaceEmpty()) return -1;
                void* swapSpace = NULL;
                swapSpace = ku_mmu_swapListDequeue();
                
                if(isMappingListEmpty()) return -1;

                char* charMapPte;
                mappingListDequeue(&charMapPte);
                char mapPte = *charMapPte; 

                int mapPfNumber = (mapPte & PFN_MASK) >> PFN_SHIFT;
                void* mapChangeAddress = ku_mmu_pmem_address + (mapPfNumber*PAGE_SIZE);

                memcpy(swapSpace, mapChangeAddress, PAGE_SIZE);
                memset(mapChangeAddress, 0, PAGE_SIZE);

                mapPte = (((swapSpace - ku_mmu_swap_address)/PAGE_SIZE) << 1) & 0b11111110;
                *charMapPte = mapPte;
    
                pageAddress = mapChangeAddress;
                pte = ((pageAddress - ku_mmu_pmem_address)/PAGE_SIZE) << 2 | PRESENT_BIT;
                *(char*)ptIdx = pte;
                mappingListEnqueue((char*)ptIdx);
            }
            else{
                pageAddress = freeListDequeue();
                pte = (((pageAddress - ku_mmu_pmem_address)/PAGE_SIZE) << 2) | PRESENT_BIT;
                *(char*)ptIdx = pte;
                mappingListEnqueue((char*)ptIdx);
            }
        }
        else{ // swap space에 있는 경우
            // swap은 pte안에 있음
            int swapOffset = (pte & SWAP_OFFSET_MASK) >> 1;
            void* swapSpace = ku_mmu_swap_address + (swapOffset*PAGE_SIZE);
            
            // 사용중인 page를 담고있는 pte 하나 가져옴
            if(isMappingListEmpty()) return -1;
            char* charMapPte;
            mappingListDequeue(&charMapPte);
            char mapPte = *charMapPte;

            int mapPfNumber = (mapPte & PFN_MASK) >> PFN_SHIFT;
            void* mapChangeAddress = ku_mmu_pmem_address + (mapPfNumber*PAGE_SIZE);

            void* tmp = malloc(PAGE_SIZE);

            memcpy(tmp, mapChangeAddress, PAGE_SIZE);
            memcpy(mapChangeAddress, swapSpace, PAGE_SIZE);
            memcpy(swapSpace, tmp, PAGE_SIZE);
            free(tmp);

            // mapping -> swap으로 간 mapPte 수정
            mapPte = (((swapSpace - ku_mmu_swap_address)/PAGE_SIZE) << SWAP_OFFSET_SHIFT);
            *charMapPte = mapPte;

            // swap -> mapping으로 온 pte 수정
            pageAddress = mapChangeAddress;
            pte = ((pageAddress - ku_mmu_pmem_address)/PAGE_SIZE) << 2 | PRESENT_BIT;
            *(char*)ptIdx = pte;
            mappingListEnqueue((char*)ptIdx);
        }
    }
    return 0;
}