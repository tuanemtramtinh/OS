/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee
 * a personal to use and modify the Licensed Source Code for
 * the sole purpose of studying during attending the course CO2018.
 */
//#ifdef MM_TLB
/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access
 * and runs at high speed
 */

#include "mm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define init_tlbcache(mp, sz, ...) init_memphy(mp, sz, (1, ##__VA_ARGS__))

pthread_mutex_t tlb_lock;

/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */

int tlb_cache_read(struct pcb_t *proc, struct memphy_struct *mp, int pid,
                   int pgnum, BYTE *value) {
  /* TODO: the identify info is mapped to
   *      cache line by employing:
   *      direct mapped, associated mapping etc.
   */
  if (mp == NULL) {
    return -1;
  }
  //*
  uint32_t tlb_index = (uint32_t)pgnum % mp->maxsz;

  pthread_mutex_lock(&tlb_lock);

  if (mp->storage[tlb_index] == -1) {
    //* do not have that entry in tlb, fail to read
    //* update the tlb entries outside this function
    pthread_mutex_unlock(&tlb_lock);
    return -1;
  }
  //* checking pid
  if (pid != mp->pid_hold) {
    //* pid changed so that the data in tlb is not accurate anymore
    //* therefore, flush all and conclude that it is a miss hit
    // printf("Different process come in TLB, current process is: %d, new process is: %d\n", pid, mp->pid_hold);
    // printf("Flush every pages of process %d in stored in TLB\n", pid);
    tlb_change_all_page_tables_of(proc,mp);
    mp->pid_hold = pid;
    pthread_mutex_unlock(&tlb_lock);
    return -1;
  }
  //*value = mp->storage[tlbIndex];
  // return mp->storage[tlbIndex];
  TLBMEMPHY_read(mp, tlb_index, value);
  pthread_mutex_unlock(&tlb_lock);
  //* return the value if value == -1 mean it not exist in TLB, else it does exist
  return *value;
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_write(struct pcb_t *proc, struct memphy_struct *mp, int pid,
                    int pgnum, BYTE value) {
  /* TODO: the identify info is mapped to
   *      cache line by employing:
   *      direct mapped, associated mapping etc.
   */
  if (mp == NULL) {
    return -1;
  }
  if (pid != mp->pid_hold) {
    //* pid changed so that the data in tlb is not accurate anymore
    //* therefore, flush all and conclude that it is a miss hit
    // printf("Different process come in TLB, current process is: %d, new process is: %d\n", pid, mp->pid_hold);
    // printf("Flush every pages of process %d in stored in TLB\n", pid);
    //printf("pid_hold in tlb_cache_write before assignment : %d\n", mp->pid_hold);
    tlb_change_all_page_tables_of(proc,mp);
    //* update pid hold
    mp->pid_hold = pid;
    //printf("pid_hold in tlb_cache_write after assignment : %d\n", mp->pid_hold);
    // return 0;
  }
  uint32_t tlb_index = (uint32_t)pgnum % mp->maxsz;
  pthread_mutex_lock(&tlb_lock);
  // mp->storage[address] = value;
  TLBMEMPHY_write(mp, tlb_index, value);
  pthread_mutex_unlock(&tlb_lock);
  return 0;
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct *mp, int addr, BYTE *value) {
  if (mp == NULL)
    return -1;

  /* TLB cached is random access by native */
  *value = mp->storage[addr];

  return 0;
}

/*
 *  TLBMEMPHY_write natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int TLBMEMPHY_write(struct memphy_struct *mp, int addr, BYTE data) {
  if (mp == NULL)
    return -1;

  /* TLB cached is random access by native */
  mp->storage[addr] = data;
  return 0;
}

/*
 *  TLBMEMPHY_format natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 */

int TLBMEMPHY_dump(struct memphy_struct *mp) {
  /*TODO dump memphy contnt mp->storage
   *     for tracing the memory content
   */
  if (!mp || !mp->storage) {
    return -1;
  }

  printf("---TLB MEM DUMP---\n");
  uint32_t *word_storage = (uint32_t *)mp->storage;
  // printf("%d\n", mp->maxsz);
  int i;
  for (i = 0; i < mp->maxsz / 4; i++)
    if (word_storage[i] != -1)
      // printf("%d : %d\n", i, word_storage[i]);
      printf("%08x: %08x\n", i * 4, word_storage[i]);
  return 0;
}

/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size) {

  mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
  int i = 0;
  while (i < max_size) {
    mp->storage[i] = -1;
    i++;
  }
  mp->maxsz = max_size;
  mp->pid_hold = -1;
  mp->rdmflg = 1;
  pthread_mutex_init(&tlb_lock, NULL);
  return 0;
}

//#endif