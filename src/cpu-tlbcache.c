// /*
//  * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
//  */
// /*
//  * Source Code License Grant: Authors hereby grants to Licensee 
//  * a personal to use and modify the Licensed Source Code for 
//  * the sole purpose of studying during attending the course CO2018.
//  */
// //#ifdef MM_TLB
// /*
//  * Memory physical based TLB Cache
//  * TLB cache module tlb/tlbcache.c
//  *
//  * TLB cache is physically memory phy
//  * supports random access 
//  * and runs at high speed
//  */


// #include "mm.h"
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>

// #define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))

// /*
//  *  tlb_cache_read read TLB cache device
//  *  @mp: memphy struct
//  *  @pid: process id
//  *  @pgnum: page number
//  *  @value: obtained value
//  */

// int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, BYTE * value)
// {
//    /* : the identify info is mapped to 
//     *      cache line by employing:
//     *      direct mapped, associated mapping etc.
//     */
   
//    if (mp == NULL || value == NULL){
//       return -1;
//    }

//    int index = (pid ^ pgnum) % mp->maxsz;
//    *value = mp->storage[index];

//    return 0;
// }

// /*
//  *  tlb_cache_write write TLB cache device
//  *  @mp: memphy struct
//  *  @pid: process id
//  *  @pgnum: page number
//  *  @value: obtained value
//  */
// int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, BYTE value)
// {
//    /* TODO: the identify info is mapped to 
//     *      cache line by employing:
//     *      direct mapped, associated mapping etc.
//     */
//    if (mp == NULL) {
//         return -1;  // Return error if the input pointer is invalid
//     }

//    // Compute index for direct mapped cache
//    int index = (pid ^ pgnum) % mp->maxsz;  // Simple hash combining pid and page number
//    mp->storage[index] = value;  // Store the value in the cache
//    return 0;
// }

// /*
//  *  TLBMEMPHY_read natively supports MEMPHY device interfaces
//  *  @mp: memphy struct
//  *  @addr: address
//  *  @value: obtained value
//  */
// int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
// {
//    if (mp == NULL)
//      return -1;

//    /* TLB cached is random access by native */
//    *value = mp->storage[addr];

//    return 0;
// }


// /*
//  *  TLBMEMPHY_write natively supports MEMPHY device interfaces
//  *  @mp: memphy struct
//  *  @addr: address
//  *  @data: written data
//  */
// int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
// {
//    if (mp == NULL)
//      return -1;

//    /* TLB cached is random access by native */
//    mp->storage[addr] = data;

//    return 0;
// }

// /*
//  *  TLBMEMPHY_format natively supports MEMPHY device interfaces
//  *  @mp: memphy struct
//  */


// int TLBMEMPHY_dump(struct memphy_struct * mp)
// {
//    /*TODO dump memphy contnt mp->storage 
//     *     for tracing the memory content
//     */
//    for (int i = 0; i < mp->maxsz; i++) {
//         printf("TLB Entry %d: %d\n", i, mp->storage[i]);
//    }
//    return 0;
// }


// /*
//  *  Init TLBMEMPHY struct
//  */
// int init_tlbmemphy(struct memphy_struct *mp, int max_size)
// {
//    mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
//    mp->maxsz = max_size;

//    mp->rdmflg = 1;

//    return 0;
// }

// //#endif


/*
 * Copyright (C) 2024 pdnguyen of the HCMC University of Technology
 */
/*
 * Source Code License Grant: Authors hereby grants to Licensee 
 * a personal to use and modify the Licensed Source Code for 
 * the sole purpose of studying during attending the course CO2018.
 */

/*
 * Memory physical based TLB Cache
 * TLB cache module tlb/tlbcache.c
 *
 * TLB cache is physically memory phy
 * supports random access 
 * and runs at high speed
 */

#include "os-cfg.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#ifdef CPU_TLB

#define init_tlbcache(mp,sz,...) init_memphy(mp, sz, (1, ##__VA_ARGS__))

/*
 *  tlb_cache_read read TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */

/* Groups' define memphy_struct TLB:
 *    + maxsz will be the size of the TLB
 *    + storage will be 
 *    + rmdflg will always be 1
 *    + tlb_fp_list->owner->tlbpgd[pgnum] = frmnum
 *    + 
*/

int tlb_cache_read(struct memphy_struct * mp, int pid, int pgnum, BYTE *value, int *frmnum)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */

   /* Our group's code */
   if(!mp || pgnum < 0) {
      return -1;
   }

   *value = *frmnum = -1;
   int TLB_SIZE = mp->maxsz;
   if(mp->TLB[pgnum % TLB_SIZE].TLB_pid == pid
   && mp->TLB[pgnum % TLB_SIZE].TLB_pgn == pgnum) {
      *frmnum = mp->TLB[pgnum % TLB_SIZE].TLB_fpn;
      TLBMEMPHY_read(mp, pgnum % TLB_SIZE, value);
      return 0;
   }   

   return -1;
   /* Our group's code */
}

/*
 *  tlb_cache_write write TLB cache device
 *  @mp: memphy struct
 *  @pid: process id
 *  @pgnum: page number
 *  @value: obtained value
 */
int tlb_cache_write(struct memphy_struct *mp, int pid, int pgnum, BYTE *value, int *frmnum)
{
   /* TODO: the identify info is mapped to 
    *      cache line by employing:
    *      direct mapped, associated mapping etc.
    */

   /* Our group's code */
   if(!mp || pgnum < 0) {
      return -1;
   }

   *frmnum = -1;
   int TLB_SIZE = mp->maxsz;
   if(mp->TLB[pgnum % TLB_SIZE].TLB_pid == pid
   && mp->TLB[pgnum % TLB_SIZE].TLB_pgn == pgnum) {
      *frmnum = mp->TLB[pgnum % TLB_SIZE].TLB_fpn;
      TLBMEMPHY_write(mp, pgnum % TLB_SIZE, *value);
      return 0;
   }   
   
   return -1;
   /* Our group's code */
}

/*
 *  TLBMEMPHY_read natively supports MEMPHY device interfaces
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int TLBMEMPHY_read(struct memphy_struct * mp, int addr, BYTE *value)
{
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
int TLBMEMPHY_write(struct memphy_struct * mp, int addr, BYTE data)
{
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
int TLBMEMPHY_dump(struct memphy_struct * mp)
{
   /*TODO dump memphy contnt mp->storage 
    *     for tracing the memory content
    */

   /* Our group's code */
   if(!mp || !mp->storage) {
      return -1;
   }

   int i;
   for(i = 0; i < mp->maxsz; i ++) {
      printf("%c\n", mp->storage[i]);
   }
   printf("\n");
   /* Our group's code */

   return 0;
}


/*
 *  Init TLBMEMPHY struct
 */
int init_tlbmemphy(struct memphy_struct *mp, int max_size)
{
   mp->storage = (BYTE *)malloc(max_size*sizeof(BYTE));
   mp->maxsz = max_size;

   // /* Our group's code */
   mp->TLB = (struct tlb_property_struct *)malloc(max_size * sizeof(struct tlb_property_struct));
   for(int i = 0; i < max_size; i ++) {
      mp->TLB[i].TLB_pid = 
      mp->TLB[i].TLB_fpn = 
      mp->TLB[i].TLB_pgn = 
      mp->storage[i] = -1;
   }
   // mp->TLB_entry = (int*)malloc(max_size*sizeof(int));
   // /* Our group's code */

   mp->rdmflg = 1;

   return 0;
}

#endif
