//#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Virtual memory module mm/mm-vm.c
 */

#include "mm.h"
#include "string.h"
#include <stdio.h>
#include <stdlib.h>

/*enlist_vm_freerg_list - add new rg to freerg_list
 *@mm: memory region
 *@rg_elmt: new region
 *
 */
int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt) {
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;

  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;

  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;

  /* Enlist the new region */
  mm->mmap->vm_freerg_list = rg_elmt;

  return 0;
}

/*get_vma_by_num - get vm area by numID
 *@mm: memory region
 *@vmaid: ID vm area to alloc memory region
 *
 */
struct vm_area_struct *get_vma_by_num(struct mm_struct *mm, int vmaid) {
  struct vm_area_struct *pvma = mm->mmap;

  if (mm->mmap == NULL)
    return NULL;

  int vmait = 0;

  while (vmait < vmaid) {
    if (pvma == NULL)
      return NULL;

    vmait++;
    pvma = pvma->vm_next;
  }

  return pvma;
}

/*get_symrg_byid - get mem region by region ID
 *@mm: memory region
 *@rgid: region ID act as symbol index of variable
 *
 */
struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid) {
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;

  return &mm->symrgtbl[rgid];
}

/*__alloc - allocate a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *@alloc_addr: address of allocated memory region
 *
 */
int __alloc(struct pcb_t *process, int vma_id, int region_id, int memory_size,
            int *allocated_address) {

  // Kiểm tra và giải phóng vùng nhớ nếu cần thiết

  if (process->mm->symrgtbl[region_id].rg_start <
      process->mm->symrgtbl[region_id].rg_end) {
    pgfree_data(process, region_id);
  }

  // Khởi tạo và lấy thông tin cần thiết

  struct vm_rg_struct *new_region = malloc(sizeof(struct vm_rg_struct));
  struct vm_area_struct *current_vma = get_vma_by_num(process->mm, vma_id);

  //Đi tìm vùng trống và cấp phát bộ nhớ từ vùng trống hiện có

  if (get_free_vmrg_area(process, vma_id, memory_size, new_region) == 0) {
    process->mm->symrgtbl[region_id].rg_start = new_region->rg_start;
    process->mm->symrgtbl[region_id].rg_end = new_region->rg_end;
    *allocated_address = new_region->rg_start;
    return 0;
  }

  /* TODO get_free_vmrg_area FAILED handle the region management (Fig.6)*/

  // Tăng giới hạn vùng nhớ để tạo không gian

  int increased_size = PAGING_PAGE_ALIGNSZ(memory_size);
  int previous_sbrk = current_vma->sbrk;

  if (inc_vma_limit(process, vma_id, increased_size) < 0) {
    printf("Unable to increase the limit\n");
    return -1;
  }

  // if (inc_sz > size) {
  //   struct vm_rg_struct *rgnode =
  //       init_vm_rg(size + old_sbrk + 1, inc_sz + old_sbrk);
  //   enlist_vm_freerg_list(caller->mm, rgnode);
  // }

  // Cập nhật giá trị sbrk và thông tin vùng nhớ mới
  current_vma->sbrk += increased_size;
  process->mm->symrgtbl[region_id].rg_start = previous_sbrk;
  process->mm->symrgtbl[region_id].rg_end = previous_sbrk + memory_size;

  *allocated_address = previous_sbrk;
  return 0;
}

/*__free - remove a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __free(struct pcb_t *process, int vma_id, int region_id) {
  struct vm_rg_struct *region_node = get_symrg_byid(process->mm, region_id);
  // struct vm_area_struct * curr_vma = get_vma_by_num(caller->mm, vmaid);

  if (region_id < 0 || region_id > PAGING_MAX_SYMTBL_SZ)
    return -1;

  /* TODO: Manage the collect freed region to freerg_list */

  /*enlist the obsoleted memory region */
  enlist_vm_freerg_list(process->mm, region_node);
  return 0;
}

/*pgalloc - PAGING-based allocate a region memory
 *@proc:  Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */
int pgalloc(struct pcb_t *proc, uint32_t size, uint32_t reg_index) {
  int addr;

  /* By default using vmaid = 0 */
  return __alloc(proc, 0, reg_index, size, &addr);
}

/*pgfree - PAGING-based free a region memory
 *@proc: Process executing the instruction
 *@size: allocated size
 *@reg_index: memory region ID (used to identify variable in symbole table)
 */

int pgfree_data(struct pcb_t *proc, uint32_t reg_index) {
  return __free(proc, 0, reg_index);
}

/*pg_getpage - get the page in ram
 *@mm: memory region
 *@pagenum: PGN
 *@framenum: return FPN
 *@caller: caller
 *
 */
int pg_getpage(struct mm_struct *mm, int page_num, int *frame_num,
               struct pcb_t *pcb) {
  uint32_t page_entry = mm->pgd[page_num];

  if (!PAGING_PAGE_PRESENT(
          page_entry)) { /* Page is not online, make it actively living */
    int victim_page_num, swap_frame_num;
    int victim_frame_num;
    uint32_t victim_page_entry;

    int target_frame_num =
        PAGING_SWP(page_entry); // the target frame storing our variable
    int free_frame_num;         // finding free frame in RAM

    /* TODO: Play with your paging theory here */

    if (MEMPHY_get_freefp(pcb->mram, &free_frame_num) == 0) {
      __swap_cp_page(pcb->active_mswp, target_frame_num, pcb->mram,
                     free_frame_num);
      pte_set_fpn(&mm->pgd[page_num], free_frame_num);
      return 0;
    }

    /* Find victim page */
    if (find_victim_page(pcb->mm, &victim_page_num) < 0) {
      return -1;
    }

    victim_page_entry = pcb->mm->pgd[victim_page_num];
    victim_frame_num = PAGING_FPN(victim_page_entry);

    /* Get free frame in MEMSWP */
    if (MEMPHY_get_freefp(pcb->active_mswp, &swap_frame_num) < 0) {
      printf("Not enough space in swap\n");
      return -1;
    }

    __swap_cp_page(pcb->mram, victim_frame_num, pcb->active_mswp,
                   swap_frame_num);

    /* Copy target frame from swap to mem */
    __swap_cp_page(pcb->active_mswp, target_frame_num, pcb->mram,
                   victim_frame_num);

    MEMPHY_put_freefp(pcb->active_mswp, target_frame_num);

    /* Update page table */
    pte_set_swap(&pcb->mm->pgd[victim_page_num], 0, swap_frame_num);
    pte_set_fpn(&mm->pgd[page_num], victim_frame_num);

#ifdef CPU_TLB
    /* Update its online status of TLB (if needed) */
#endif

    enlist_pgn_node(&pcb->mm->fifo_pgn, page_num);
    return 0;
  }

  *frame_num = PAGING_FPN(page_entry);
  return 0;
}

/*pg_getval - read value at given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_getval(struct mm_struct *mm, int addr, BYTE *data,
              struct pcb_t *caller) {
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_read(caller->mram, phyaddr, data);

  return 0;
}

/*pg_setval - write value to given offset
 *@mm: memory region
 *@addr: virtual address to acess
 *@value: value
 *
 */
int pg_setval(struct mm_struct *mm, int addr, BYTE value,
              struct pcb_t *caller) {
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  /* Get the page to MEMRAM, swap from MEMSWAP if needed */
  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1; /* invalid page access */

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;

  MEMPHY_write(caller->mram, phyaddr, value);

  return 0;
}

/*__read - read value in region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __read(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE *data) {
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
    return -1;

  pg_getval(caller->mm, currg->rg_start + offset, data, caller);

  return 0;
}

/*pgwrite - PAGING-based read a region memory */
int pgread(struct pcb_t *proc, // Process executing the instruction
           uint32_t source,    // Index of source register
           uint32_t offset,    // Source address = [source] + [offset]
           uint32_t destination) {
  BYTE data;
  int val = __read(proc, 0, source, offset, &data);

  destination = (uint32_t)data;
#ifdef IODUMP
  printf("read region=%d offset=%d value=%d\n", source, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return val;
}

/*__write - write a region memory
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@offset: offset to acess in memory region
 *@rgid: memory region ID (used to identify variable in symbole table)
 *@size: allocated size
 *
 */
int __write(struct pcb_t *caller, int vmaid, int rgid, int offset, BYTE value) {
  struct vm_rg_struct *currg = get_symrg_byid(caller->mm, rgid);

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  if (currg == NULL || cur_vma == NULL) /* Invalid memory identify */
    return -1;

  pg_setval(caller->mm, currg->rg_start + offset, value, caller);

  return 0;
}

/*pgwrite - PAGING-based write a region memory */
int pgwrite(struct pcb_t *proc,   // Process executing the instruction
            BYTE data,            // Data to be wrttien into memory
            uint32_t destination, // Index of destination register
            uint32_t offset) {
#ifdef IODUMP
  printf("write region=%d offset=%d value=%d\n", destination, offset, data);
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1); // print max TBL
#endif
  MEMPHY_dump(proc->mram);
#endif

  return __write(proc, 0, destination, offset, data);
}

/*free_pcb_memphy - collect all memphy of pcb
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 */
int free_pcb_memph(struct pcb_t *caller) {
  int pagenum, fpn;
  uint32_t pte;

  for (pagenum = 0; pagenum < PAGING_MAX_PGN; pagenum++) {
    pte = caller->mm->pgd[pagenum];

    if (!PAGING_PAGE_PRESENT(pte)) {
      fpn = PAGING_FPN(pte);
      MEMPHY_put_freefp(caller->mram, fpn);
    } else {
      fpn = PAGING_SWP(pte);
      MEMPHY_put_freefp(caller->active_mswp, fpn);
    }
  }

  return 0;
}

/*get_vm_area_node - get vm area for a number of pages
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@incpgnum: number of page
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid,
                                             int size, int alignedsz) {
  struct vm_rg_struct *newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = newrg->rg_start + size;

  return newrg;
}

/*validate_overlap_vm_area
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@vmastart: vma end
 *@vmaend: vma end
 *
 */
int validate_overlap_vm_area(struct pcb_t *process, int vma_id, int start_addr,
                             int end_addr) {
  struct vm_area_struct *current_vma = process->mm->mmap;

  /* TODO validate the planned memory area is not overlapped */

  while (current_vma != NULL) {
    if (vma_id != current_vma->vm_id) {
      if (OVERLAP(start_addr, end_addr, current_vma->vm_start,
                  current_vma->vm_end)) {
        if ((start_addr != current_vma->vm_end ||
             end_addr == current_vma->vm_start) &&
            (start_addr == current_vma->vm_end ||
             end_addr != current_vma->vm_start)) {
          return -1;
        }
      }
    }
    current_vma = current_vma->vm_next;
  }
  return 0;
}

/*inc_vma_limit - increase vm area limits to reserve space for new variable
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@inc_sz: increment size
 *
 */
int inc_vma_limit(struct pcb_t *caller, int vmaid, int inc_sz) {
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = PAGING_PAGE_ALIGNSZ(inc_sz);
  int incnumpage = inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area =
      get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  int old_end = cur_vma->vm_end;

  /*Validate overlap of obtained region */
  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
    return -1; /*Overlap and failed allocation */

  /* The obtained vm area (only)
   * now will be alloc real ram region */
  cur_vma->vm_end += inc_sz;
  if (vm_map_ram(caller, area->rg_start, area->rg_end, old_end, incnumpage,
                 newrg) < 0)
    return -1; /* Map the memory to MEMRAM */

  return 0;
}

/*find_victim_page - find victim page
 *@caller: caller
 *@pgn: return page number
 *
 */
int find_victim_page(struct mm_struct *mm, int *victim_page) {
  struct pgn_t *page = mm->fifo_pgn;
  struct pgn_t *page_prev = NULL;

  /* TODO: Implement the theorical mechanism to find the victim page */
  if (page == NULL) {
    *victim_page = -1;
    return -1;
  }

  while (page->pg_next != NULL) {
    page_prev = page;
    page = page->pg_next;
  }

  if (page_prev == NULL) {
    mm->fifo_pgn = NULL;
  } else {
    page_prev->pg_next = NULL;
  }

  *victim_page = page->pgn;
  free(page);

  return 0;
}

/*get_free_vmrg_area - get a free vm region
 *@caller: caller
 *@vmaid: ID vm area to alloc memory region
 *@size: allocated size
 *
 */
int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size,
                       struct vm_rg_struct *newrg) {
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, vmaid);

  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;

  if (rgit == NULL)
    return -1;

  /* Probe unintialized newrg */
  newrg->rg_start = newrg->rg_end = -1;

  /* Traverse on list of free vm region to find a fit space */
  while (rgit != NULL) {
    if (rgit->rg_start + size <=
        rgit->rg_end) { /* Current region has enough space */
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      /* Update left space in chosen region */
      if (rgit->rg_start + size < rgit->rg_end) {
        rgit->rg_start = rgit->rg_start + size;
      } else { /*Use up all space, remove current node */
        /*Clone next rg node */
        struct vm_rg_struct *nextrg = rgit->rg_next;

        /*Cloning */
        if (nextrg != NULL) {
          rgit->rg_start = nextrg->rg_start;
          rgit->rg_end = nextrg->rg_end;

          rgit->rg_next = nextrg->rg_next;

          free(nextrg);
        } else {                         /*End of free list */
          rgit->rg_start = rgit->rg_end; // dummy, size 0 region
          rgit->rg_next = NULL;
        }
      }

      // break;
    } else {
      rgit = rgit->rg_next; // Traverse next rg
    }
  }

  if (newrg->rg_start == -1) // new region not found
    return -1;
  return 0;
}

//#endif
