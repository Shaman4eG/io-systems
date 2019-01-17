#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/string.h>

#include "ram_device.h"
#include "partition.h"

#define RB_DEVICE_SIZE 102656 /* sectors */
/* So, total device size = 102400 * 512 bytes = 50 MiB */
/* + 65 bytes for headers*/

/* Array where the disk stores its data */
static u8 *dev_data;

int ramdevice_init(void)
{
    dev_data = vmalloc(RB_DEVICE_SIZE * RB_SECTOR_SIZE);
    if (dev_data == NULL)
        return -ENOMEM;
    /* Setup its partition table */
    copy_mbr_n_br(dev_data);
    return RB_DEVICE_SIZE;
}

void ramdevice_cleanup(void)
{
    vfree(dev_data);
}

void ramdevice_write(sector_t sector_off, u8 *buffer, unsigned int sectors)
{
    memcpy(dev_data + sector_off * RB_SECTOR_SIZE, buffer,
           sectors * RB_SECTOR_SIZE);
}
void ramdevice_read(sector_t sector_off, u8 *buffer, unsigned int sectors)
{
    memcpy(buffer, dev_data + sector_off * RB_SECTOR_SIZE,
           sectors * RB_SECTOR_SIZE);
}