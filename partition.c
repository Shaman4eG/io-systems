#include <linux/string.h>
#include <linux/kernel.h>
#include "partition.h"

#define SEC_PER_TRACK 63
#define HEAD_COUNT 255
#define H HEAD_COUNT
#define S SEC_PER_TRACK
#define s(LBA) (((LBA) % S) + 1)
#define h(LBA) ((((LBA) - (s(LBA) - 1)) / S) % H + 1) 
#define c(LBA) (((LBA) - (s(LBA) - 1) - (h(LBA) - 1) * S) / (H * S))
#define SYS_TYPE_LINUX 0x83
#define SYS_TYPE_EXT 0x05
#define DISK_IDENTIFIER 0xDEADBEEF
#define SECTOR_SIZE 512
#define MBR_SIZE SECTOR_SIZE
#define MBR_DISK_SIGNATURE_OFFSET 440
#define MBR_DISK_SIGNATURE_SIZE 4
#define PARTITION_TABLE_OFFSET 446
#define PARTITION_ENTRY_SIZE 16 // sizeof(PartEntry)
#define PARTITION_TABLE_SIZE 64 // sizeof(PartTable)
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE_SIZE 2
#define MBR_SIGNATURE 0xAA55
#define BR_SIZE SECTOR_SIZE
#define BR_SIGNATURE_OFFSET 510
#define BR_SIGNATURE_SIZE 2
#define BR_SIGNATURE 0xAA55

#define PARTITION_HEADER_SIZE 0x0000003f

#define PRIMARY_SIZE    0x00005000  // in blocks 10 Mb 
#define LOG_SIZE        0x0000A000  // in blocks 20 Mb  

//#define PRIMARY_SIZE  0x00001800      // in blocks, 30 Mb
//#define LOG_SIZE      0x00005000      // in blocks, 10 Mb

typedef struct
{
    unsigned char boot_type; // 0x00 - Inactive; 0x80 - Active (Bootable)
    unsigned char start_head;
    unsigned char start_sec : 6;
    unsigned char start_cyl_hi : 2;
    unsigned char start_cyl;
    unsigned char part_type;
    unsigned char end_head;
    unsigned char end_sec : 6;
    unsigned char end_cyl_hi : 2;
    unsigned char end_cyl;
    unsigned long abs_start_sec;
    unsigned long sec_in_part;
} PartEntry;

typedef PartEntry PartTable[4];

static PartTable def_part_table =
    {
        {
            boot_type : 0x00,
            start_head : 0x01,
            start_sec : 0x01,
            start_cyl : 0x00,
            part_type : SYS_TYPE_LINUX,
            end_head : h(PRIMARY_SIZE),    
            end_sec : s(PRIMARY_SIZE - 1), 
            end_cyl : c(PRIMARY_SIZE),    
            abs_start_sec : PARTITION_HEADER_SIZE,
            sec_in_part : PRIMARY_SIZE
        },
        {
            boot_type : 0x00,
            start_head : h(PRIMARY_SIZE + 1),
            start_sec : s(PRIMARY_SIZE + 1), 
            start_cyl : c(PRIMARY_SIZE + 1),  
            part_type : SYS_TYPE_EXT,
            end_head : h(PRIMARY_SIZE + 2 * LOG_SIZE),                
            end_sec : s(PRIMARY_SIZE + 2 * LOG_SIZE),                 
            end_cyl : c(PRIMARY_SIZE + 2 * LOG_SIZE),                 
            abs_start_sec : PRIMARY_SIZE + PARTITION_HEADER_SIZE + 1,
            sec_in_part : LOG_SIZE * 2                                
        },
        {},
        {}};
static unsigned int def_log_part_lba[] = {PRIMARY_SIZE + PARTITION_HEADER_SIZE + 1,
                                          PRIMARY_SIZE + LOG_SIZE + 2 * (PARTITION_HEADER_SIZE + 1)};
static const PartTable def_log_part_table[] =
    {
        {{
             boot_type : 0x00,
             start_head : h(PRIMARY_SIZE + 1) + 1, 
             start_sec : s(PRIMARY_SIZE + 1),     
             start_cyl : c(PRIMARY_SIZE + 1),      
             part_type : SYS_TYPE_LINUX,
             end_head : h(PRIMARY_SIZE + LOG_SIZE), 
             end_sec : s(PRIMARY_SIZE + LOG_SIZE),  
             end_cyl : c(PRIMARY_SIZE + LOG_SIZE),  
             abs_start_sec : PARTITION_HEADER_SIZE,
             sec_in_part : LOG_SIZE
         },
         {
             boot_type : 0x00,
             start_head : h(PRIMARY_SIZE + LOG_SIZE + 1), 
             start_sec : s(PRIMARY_SIZE + LOG_SIZE + 1), 
             start_cyl : c(PRIMARY_SIZE + LOG_SIZE + 1),  
             part_type : SYS_TYPE_EXT,
             end_head : h(PRIMARY_SIZE + 2 * LOG_SIZE), 
             end_sec : s(PRIMARY_SIZE + 2 * LOG_SIZE),  
             end_cyl : c(PRIMARY_SIZE + 2 * LOG_SIZE),  
             abs_start_sec : LOG_SIZE + PARTITION_HEADER_SIZE + 1,
             sec_in_part : LOG_SIZE + PARTITION_HEADER_SIZE
         }},
        {{
             boot_type : 0x00,
             start_head : h(PRIMARY_SIZE + 2 * LOG_SIZE + 1) + 1, 
             start_sec : s(PRIMARY_SIZE + 2 * LOG_SIZE + 1),      
             start_cyl : c(PRIMARY_SIZE + 2 * LOG_SIZE + 1),      
             part_type : SYS_TYPE_LINUX,
             end_head : h(PRIMARY_SIZE + 2 * LOG_SIZE), 
             end_sec : s(PRIMARY_SIZE + 2 * LOG_SIZE),  
             end_cyl : c(PRIMARY_SIZE + 2 * LOG_SIZE),  
             abs_start_sec : PARTITION_HEADER_SIZE,
             sec_in_part : LOG_SIZE
         }}
	};

static void copy_mbr(u8 *disk)
{
    //alloc mem for mbr
    memset(disk, 0x0, MBR_SIZE);
    //set disk identifier
    *(unsigned long *)(disk + MBR_DISK_SIGNATURE_OFFSET) = DISK_IDENTIFIER;
    //set mbr
    memcpy(disk + PARTITION_TABLE_OFFSET, &def_part_table, PARTITION_TABLE_SIZE);
    //set mbr signature
    *(unsigned short *)(disk + MBR_SIGNATURE_OFFSET) = MBR_SIGNATURE;
}

static void copy_br(u8 *disk, int lba, const PartTable *part_table)
{
    disk += lba * SECTOR_SIZE;
    //alloc mem for br
    memset(disk, 0x0, BR_SIZE);
    //set br
    memcpy(disk + PARTITION_TABLE_OFFSET, part_table,
           PARTITION_TABLE_SIZE);
    //set br signature
    *(unsigned short *)(disk + BR_SIGNATURE_OFFSET) = BR_SIGNATURE;
}

void copy_mbr_n_br(u8 *disk)
{
    int i;
    copy_mbr(disk);
    for (i = 0; i < ARRAY_SIZE(def_log_part_table); i++)
    {
        copy_br(disk, def_log_part_lba[i], &def_log_part_table[i]);
    }
}