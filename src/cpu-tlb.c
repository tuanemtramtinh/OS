/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee
 * a personal to use and modify the Licensed Source Code for
 * the sole purpose of studying during attending the course CO2018.
 */
// #ifdef CPU_TLB
/*
 * CPU TLB
 * TLB module cpu/cpu-tlb.c
 */

#include "mm.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int tlb_change_all_page_tables_of(struct pcb_t *proc,
                                  struct memphy_struct *mp) {
  /* TODO update all page table directory info
   *      in flush or wipe TLB (if needed)
   */
  int checking_flush = tlb_flush_tlb_of(proc, mp);
  return checking_flush;
}

int tlb_flush_tlb_of(struct pcb_t *process, struct memphy_struct *memory) {
  /* TODO flush tlb cached*/

  if (memory == NULL) {
    return -1;
  }
  int tlb_size = memory->maxsz;
  for (int index = 0; index < tlb_size; index++) {
    memory->storage[index] = -1;
  }
  return 0;
}

/*tlballoc - CPU TLB-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlballoc(struct pcb_t *process, uint32_t allocation_size,
             uint32_t region_index) {
  int address, result;

  /* By default using vmaid = 0 */
  result = __alloc(process, 0, region_index, allocation_size, &address);

  /* TODO update TLB CACHED frame num of the new allocated page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  int page_number = PAGING_PGN(address);
  int aligned_allocation_size = PAGING_PAGE_ALIGNSZ(allocation_size);

  int num_pages = aligned_allocation_size / PAGING_PAGESZ;

  int i = 0;

  while (i < num_pages){
    uint32_t *frame_number = malloc(sizeof(uint32_t));
    pg_getpage(process->mm, page_number + i, (int *)frame_number, process);
    tlb_cache_write(process, process->tlb, process->pid, (page_number + i),
                    *frame_number);
    i++;
  }
  return result;
}

/*pgfree - CPU TLB-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int tlbfree_data(struct pcb_t *process, uint32_t region_index) {
  __free(process, 0, region_index);

  /* TODO update TLB CACHED frame num of freed page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  int start_address = process->mm->symrgtbl[region_index].rg_start;
  int end_address = process->mm->symrgtbl[region_index].rg_end;
  int region_size = end_address - start_address;
  int page_number = PAGING_PGN(start_address);
  int aligned_deallocate_size = PAGING_PAGE_ALIGNSZ(region_size);
  int number_of_freed_pages = aligned_deallocate_size / PAGING_PAGESZ;

  int i = 0;

  while (i < number_of_freed_pages){
    uint32_t *frame_number = malloc(sizeof(uint32_t));
    pg_getpage(process->mm, page_number + i, (int *)frame_number, process);
    tlb_cache_write(process, process->tlb, process->pid, (page_number + i),
                    *frame_number);
    i++;
  }
  return 0;
}

/*tlbread - CPU TLB-based read a region memory
 *@proc: Process executing the instruction
 *@source: index of source register
 *@offset: source address = [source] + [offset]
 *@destination: destination storage
 */
int tlbread(struct pcb_t *process, uint32_t source_region, uint32_t byte_offset,
            uint32_t destination_region) {

  BYTE data, frame_number = -1;

  /* TODO retrieve TLB CACHED frame num of accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  /* frame_number is return value of tlb_cache_read/write value*/

  int start_address =
      process->mm->symrgtbl[source_region].rg_start + byte_offset;
  int page_number = PAGING_PGN(start_address);
  // int frame_num_from_desired_page = -1;
  // pg_getpage(process->mm, page_number, &frame_num_from_desired_page, process);
  BYTE frame_number_retrieved_from_tlb = 0;
  frame_number = tlb_cache_read(process, process->tlb, process->pid,
                                page_number, &frame_number_retrieved_from_tlb);

  usleep(10);
#ifdef IODUMP
  if (frame_number >= 0  /*frame_number == frame_num_from_desired_page*/)
    printf("TLB hit at read region=%d offset=%d\n", source_region, byte_offset);
  else{
    printf("TLB miss at read region=%d offset=%d\n", source_region,
           byte_offset);
  }  
#ifdef PAGETBL_DUMP
  print_pgtbl(process, 0, -1); // print max TBL
#endif
  MEMPHY_dump(process->mram);
#endif
  usleep(10);
  int read_status = __read(process, 0, source_region, byte_offset, &data);
  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/

  int frame_page_number;
  pg_getpage(process->mm, page_number, &frame_page_number, process);
  tlb_cache_write(process, process->tlb, process->pid, page_number, frame_page_number);
  return read_status;
}

/*tlbwrite - CPU TLB-based write a region memory
 *@proc: Process executing the instruction
 *@data: data to be wrttien into memory
 *@destination: index of destination register
 *@offset: destination address = [destination] + [offset]
 */
int tlbwrite(struct pcb_t *process, BYTE data, uint32_t destination_region,
             uint32_t byte_offset) {
  int write_status;
  BYTE frame_number = -1;
  // int result = tlb_cache_read(process->mram, process->pid, destination_region
  // + byte_offset, &frame_number);
  /* TODO retrieve TLB CACHED frame num of accessing page(s))*/
  /* by using tlb_cache_read()/tlb_cache_write()
  frame_number is return value of tlb_cache_read/write value*/

  int start_address = process->mm->symrgtbl[destination_region].rg_start + byte_offset;
  int page_number = PAGING_PGN(start_address);
  // uint32_t page_entry = process->mm->pgd[page_number];
  // int frame_num_from_desired_page = PAGING_FPN(page_entry);
  BYTE frame_number_retrieved_from_tlb = 0;
  frame_number = tlb_cache_read(process, process->tlb, process->pid, page_number, &frame_number_retrieved_from_tlb);

  usleep(10);
#ifdef IODUMP
  if (frame_number >= 0  /*frame_number == frame_num_from_desired_page*/)
    printf("TLB hit at write region=%d offset=%d value=%d\n",
           destination_region, byte_offset, data);
  else{
    printf("TLB miss at write region=%d offset=%d value=%d\n",
           destination_region, byte_offset, data);
  }
#ifdef PAGETBL_DUMP
  print_pgtbl(process, 0, -1); // print max TBL;
#endif
  MEMPHY_dump(process->mram);
#endif
  /* TODO update TLB CACHED with frame num of recent accessing page(s)*/
  /* by using tlb_cache_read()/tlb_cache_write()*/
  usleep(10);
  write_status = __write(process, 0, destination_region, byte_offset, data);

  int frame_page_number;
  pg_getpage(process->mm, page_number, &frame_page_number, process);
  tlb_cache_write(process, process->tlb, process->pid, page_number, frame_page_number);

  return write_status;
}

// #endif