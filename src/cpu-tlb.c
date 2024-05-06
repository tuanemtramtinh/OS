/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */
 
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp)
{
  /* TODO update all page table directory info 
   *      in flush or wipe TLB (if needed)
   */

  return tlb_flush_tlb_of(proc, mp);
}

int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp)
{
  /* TODO flush tlb cached*/

  if(mp == NULL){
    return -1;
  }
  int size_tlb = mp->maxsz;
  for(int i = 0; i < size_tlb; i++){
    mp->storage[i]  = -1;
  }
  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
{
  int addr, val;

  /* By default using vmaid = 0 */
  val = __alloc(proc, 0, reg_index, size, &addr);

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  int page_number = PAGING_PGN(addr);
  int alloc_size = PAGING_PAGE_ALIGNSZ(size);

  int numpage = alloc_size / PAGING_PAGESZ;
  for(int i = 0; i < numpage; i++){
    uint32_t *fpn = malloc(sizeof(uint32_t));
    /*uint32_t pte =*/
    pg_getpage(proc->mm, page_number + i,(int*)fpn, proc);
    // printf("frame number getting from pg_getpage in alloc: %d\n", *fpn);
    tlb_cache_write(proc, proc->tlb, proc->pid, (page_number + i), *fpn);    
  }


  return val;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size 
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
{
  __free(proc, 0, reg_index);

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  int start_region = proc->mm->symrgtbl[reg_index].rg_start;
  int end_region = proc->mm->symrgtbl[reg_index].rg_end;
  int size = end_region - start_region;
  int pgn = PAGING_PGN(start_region);
  int deallocate_sz = PAGING_PAGE_ALIGNSZ(size);
  int freed_pages = deallocate_sz / PAGING_PAGESZ;
  

  for(int i = 0; i < freed_pages; i++){
    uint32_t *fpn = malloc(sizeof(uint32_t));
    /*uint32_t pte =*/
    pg_getpage(proc->mm, pgn + i, (int*)fpn, proc);
    tlb_cache_write(proc ,proc->tlb, proc->pid, (pgn + i), *fpn);    
  }

  return 0;
}


/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t * proc, uint32_t source,
            uint32_t offset, 	uint32_t destination) 
{
  BYTE data, frmnum = -1;
	
  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frmnum is return value of tlb_cache_read/write value*/

  int start_region = proc->mm->symrgtbl[source].rg_start + offset;
  int pgn = PAGING_PGN(start_region);
  BYTE fpn_retrieved_from_tlb = 0;
  frmnum = tlb_cache_read(proc ,proc->tlb, proc->pid, pgn, &fpn_retrieved_from_tlb);
	
  // int val = tlb_cache_read(proc->mram, proc->pid, source + offset, &frmnum);

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at read region=%d offset=%d\n", 
	         source, offset);
  else 
    printf("TLB miss at read region=%d offset=%d\n", 
	         source, offset);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif


  int val = __read(proc, 0, source, offset, &data);
  /*int val2 =*/ __write(proc, 0, destination, 0, data);

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  int fpn = pg_getpage(proc->mm, pgn, &fpn, proc);
  int status = tlb_cache_write(proc, proc->tlb, proc->pid, pgn, fpn);

  return val;
}

/*tlbwrite - CPU TLB-based write a region memory
 *@proc: Process executing the instruction
 *@data: data to be wrttien into memory
 *@destination: index of destination register
 *@offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t * proc, BYTE data,
             uint32_t destination, uint32_t offset)
{
  int val;
  BYTE frmnum = -1;
  //int val = tlb_cache_read(proc->mram, proc->pid, destination + offset, &frmnum);
  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frmnum is return value of tlb_cache_read/write value*/

  int start_region = proc->mm->symrgtbl[destination].rg_start + offset;
  // printf("Start region: %d\n", start_region);
  int pgn = PAGING_PGN(start_region);
  BYTE fpn_retrieved_from_tlb = 0;
  frmnum = tlb_cache_read(proc ,proc->tlb, proc->pid, pgn, &fpn_retrieved_from_tlb);

#ifdef IODUMP
  if (frmnum >= 0)
    printf("TLB hit at write region=%d offset=%d value=%d\n",
	          destination, offset, data);
	else
    printf("TLB miss at write region=%d offset=%d value=%d\n",
            destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); //print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  //val = __write(proc, 0, destination, offset, data);

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  val = __write(proc, 0, destination, offset, data);

  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  int fpn = pg_getpage(proc->mm, pgn, &fpn, proc);
  int status = tlb_cache_write(proc, proc->tlb, proc->pid, pgn, fpn);

  return val;
}

//#endif


// /*
//  * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
//  */
// /*
//  * Source Code License Grant: Authors hereby grants to Licensee 
//  * a personal to use and modify the Licensed Source Code for 
//  * the sole purpose of studying during attending the course CO2018.
//  */

// /*
//  * CPU TLB
//  * TLB module cpu/cpu-tlb.c
//  */

// #include "os-cfg.h"
// #include "mm.h"
// #include <stdlib.h>
// #include <stdio.h>
// #ifdef CPU_TLB

// int tlb_change_all_page_tables_of(struct pcb_t *proc,  struct memphy_struct * mp)
// {
//   /* TODO update all page table directory info 
//    *      in flush or wipe TLB (if needed)
//    */

//   /* Our group's code */
//   if(!mp || !proc) {
//     return -1;
//   }

//   return tlb_flush_tlb_of(proc, mp);
//   /* Our group's code */
// }

// int tlb_flush_tlb_of(struct pcb_t *proc, struct memphy_struct * mp)
// {
//   /* TODO flush tlb cached*/

//   /* Our group's code */
//   if(!proc || !mp) {
//     return -1;
//   }

// 	int TLB_SIZE = mp->maxsz;
//   for(int i = 0; i < TLB_SIZE; i ++) {
// 		// mp->TLB[i].TLB_pid = 
// 		// mp->TLB[i].TLB_fpn = 
// 		// mp->TLB[i].TLB_pgn = 
// 		// mp->storage[i] = -1;
// 	}
//   /* Our group's code */
//   return 0;
// }

// /*tlballoc - CPU TLB-based allocate a region memory
//  *@proc:  Process executing the instruction
//  *@size: allocated size 
//  *@reg_index: memory region ID (used to identify variable in symbole table)
//  */
// int tlballoc(struct pcb_t *proc, uint32_t size, uint32_t reg_index)
// {
//   int addr, val;

//   /* Our group's code */
//   if(!proc) {
//     return -1;
//   }
//   /* Our group's code */

//   /* By default using vmaid = 0 */
//   val = __alloc(proc, 0, reg_index, size, &addr);

//   /* TODO update TLB CACHED frame num of the new allocated page(s)*/
//   /* by using tlb_cache_read()/tlb_cache_write()*/

//   /* Our group's code */
//   int pgn = PAGING_PGN(addr);
//   int pgnum = PAGING_PAGE_ALIGNSZ(size) / PAGING_PAGESZ;
//   int TLB_SIZE = proc->tlb->maxsz;

//   for(int i = 0; i < pgnum; i ++) {
//     uint32_t pte = proc->mm->pgd[i + pgn];
//     int fpn = PAGING_FPN(pte);
//     proc->tlb->TLB[(i + pgn) % TLB_SIZE].TLB_pid = proc->pid;
//     proc->tlb->TLB[(i + pgn) % TLB_SIZE].TLB_pgn = i + pgn;
//     proc->tlb->TLB[(i + pgn) % TLB_SIZE].TLB_fpn = fpn;
	
//     BYTE data_tmp;
//     int off_tmp = PAGING_OFFST(addr);
//     int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off_tmp;
//     MEMPHY_read(proc->mram, phyaddr, &data_tmp);
//     proc->tlb->storage[(i + pgn) % TLB_SIZE] = data_tmp;
//   // proc->tlb->TLB[(i + pgn) % TLB_SIZE].storage = data_tmp;
//   }
//   /* Our group's code */

//   return val;
// }

// /*pgfree - CPU TLB-based free a region memory
//  *@proc: Process executing the instruction
//  *@size: allocated size 
//  *@reg_index: memory region ID (used to identify variable in symbole table)
//  */
// int tlbfree_data(struct pcb_t *proc, uint32_t reg_index)
// {
//   /* Our group's code */
//   if(!proc) {
//     return -1;
//   }
//   /* Our group's code */

//   __free(proc, 0, reg_index);

//   /* TODO update TLB CACHED frame num of freed page(s)*/
//   /* by using tlb_cache_read()/tlb_cache_write()*/
  
//   /* Our group's code */
//   int rg_start = proc->mm->symrgtbl[reg_index].rg_start;
//   int rg_end = proc->mm->symrgtbl[reg_index].rg_end;
//   int size = rg_end - rg_start;
//   int pgn = PAGING_PGN(rg_start);
//   int pgnum = PAGING_PAGE_ALIGNSZ(size) / PAGING_PAGESZ;
//   int TLB_SIZE = proc->tlb->maxsz;

//   for(int i = 0; i < pgnum; i ++) {
//     if(proc->tlb->TLB[(i + pgn) % TLB_SIZE].TLB_pid == proc->pid
//     && proc->tlb->TLB[(i + pgn) % TLB_SIZE].TLB_pgn == i + pgn) {
//       proc->tlb->TLB[(i + pgn) % TLB_SIZE].TLB_pid = 
//       proc->tlb->TLB[(i + pgn) % TLB_SIZE].TLB_pgn = 
//       proc->tlb->TLB[(i + pgn) % TLB_SIZE].TLB_fpn =
// 	  	proc->tlb->storage[(i + pgn) % TLB_SIZE] = -1;
//     }    
//   }
//   /* Our group's code */

//   return 0;
// }


// /*tlbread - CPU TLB-based read a region memory
//  *@proc: Process executing the instruction
//  *@source: index of source register
//  *@offset: source address = [source] + [offset]
//  *@destination: destination storage
//  */
// int tlbread(struct pcb_t * proc, uint32_t source,
//             uint32_t offset, 	uint32_t *destination) 
// {
//   /* 
//    * we just add * for destination because
//    * we think that we need the return value after 
//    * reading the TLB cache
//    */
//   BYTE data;
//   int frmnum;
	
//   /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
//   /* by using tlb_cache_read()/tlb_cache_write()*/
//   /* frmnum is return value of tlb_cache_read/write value*/

// 	/* Our group's code */
//   struct vm_rg_struct *currg = get_symrg_byid(proc->mm, source);
//   int addr = currg->rg_start + offset;
//   int pgn = PAGING_PGN(addr);

//   tlb_cache_read(proc->tlb, proc->pid, pgn, &data, &frmnum);
// 	/* Our group's code */
// #ifdef IODUMP
//   if (frmnum >= 0)
//     printf("TLB hit at read region=%d offset=%d\n", 
// 	         source, offset);
//   else 
//     printf("TLB miss at read region=%d offset=%d\n", 
// 	         source, offset);
// #ifdef PAGETBL_DUMP
//   print_pgtbl(proc, 0, -1); //print max TBL
// #endif
//   MEMPHY_dump(proc->mram);
// #endif

// 	/* Our group's code */
//   int val;
//   if(frmnum >= 0) {
// 		val = 0;
//   } else {
// 		val = __read(proc, 0, source, offset, &data);
//   }  
// 	/* Our group's code */

//   *destination = (uint32_t) data;

//   /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
//   /* by using tlb_cache_read()/tlb_cache_write()*/

// 	/* Our group's code */
// 	/*
// 	 * if frmnum < 0 then update that page to TLB
// 	 * else no need to update
// 	*/
// 	if(frmnum < 0) {
// 		int TLB_SIZE = proc->tlb->maxsz;
// 		proc->tlb->TLB[pgn % TLB_SIZE].TLB_pid = proc->pid;
// 		proc->tlb->TLB[pgn % TLB_SIZE].TLB_pgn = pgn;
// 		proc->tlb->TLB[pgn % TLB_SIZE].TLB_fpn = frmnum;
// 		proc->tlb->storage[pgn % TLB_SIZE] = data;
// 	}	
// 	/* Our group's code */

//   return val;
// }

// /*tlbwrite - CPU TLB-based write a region memory
//  *@proc: Process executing the instruction
//  *@data: data to be wrttien into memory
//  *@destination: index of destination register
//  *@offset: destination address = [destination] + [offset]
//  */
// int tlbwrite(struct pcb_t * proc, BYTE data,
//              uint32_t destination, uint32_t offset)
// {
//   int val, frmnum = -1;

//   /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
//   /* by using tlb_cache_read()/tlb_cache_write()
//   frmnum is return value of tlb_cache_read/write value*/

// 	/* Our group's code */
//   struct vm_rg_struct *currg = get_symrg_byid(proc->mm, destination);
//   int addr = currg->rg_start + offset;
//   int pgn = PAGING_PGN(addr);

//   tlb_cache_write(proc->tlb, proc->pid, pgn, &data, &frmnum);
// 	/* Our group's code */

// #ifdef IODUMP
//   if (frmnum >= 0)
//     printf("TLB hit at write region=%d offset=%d value=%d\n",
// 	          destination, offset, data);
// 	else
//     printf("TLB miss at write region=%d offset=%d value=%d\n",
//             destination, offset, data);
// #ifdef PAGETBL_DUMP
//   print_pgtbl(proc, 0, -1); //print max TBL
// #endif
//   MEMPHY_dump(proc->mram);
// #endif

// 	if(frmnum < 0) {
// 		val = __write(proc, 0, destination, offset, data);
// 	} else {
// 		val = 0;
// 	}

//   /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
//   /* by using tlb_cache_read()/tlb_cache_write()*/

// 	/* Our group's code */
// 	if(frmnum < 0) {
// 		int TLB_SIZE = proc->tlb->maxsz;
// 		proc->tlb->TLB[pgn % TLB_SIZE].TLB_pid = proc->pid;
// 		proc->tlb->TLB[pgn % TLB_SIZE].TLB_pgn = pgn;
// 		proc->tlb->TLB[pgn % TLB_SIZE].TLB_fpn = frmnum;
// 		proc->tlb->storage[pgn % TLB_SIZE] = data;
// 	}	
// 	/* Our group's code */

//   return val;
// }

// #endif
