OS_MMU
===

* TLB 에 페이지 정보가 없을 시 PDBR ( page directory base register) 를 찾아봐야 한다. -> page table 을 찾아볼 수 있다.
  * 즉, PDBR 은 Page table 의 시작주소를 갖는다
 
* Page table 에 Mapping 이 되지 않은 Virtual address 에 접근하려고 할 시 Page Fault 발생  
  * Page Fault 가 발생할 시 Free list 에서 page 한개를 가져와 Mapping 을 합니다.
  * Freelist 가 없을 Page 한개를 선정하여 Swap out 해주고 새로 Mapping 을 합니다. -> Page table 업데이트 
 
* 8bit addressing : 6 bit VPN , 2 bit Offset 
* address space 256byte , page size 4 bytes , PTE 1 byte
* PDBR (CR3) 은 Linear Page Table 의 시작주소를 가리킨다.
* 1 byte PTE : 6bit PFN , 1bit User bit , 1bit Present bit
* swap space offset : 7bit offset , 1bit present bit ( swap out 시 present bit = 0 )
* swap space 의 0 번째 table 은 사용하지 않습니다. 
