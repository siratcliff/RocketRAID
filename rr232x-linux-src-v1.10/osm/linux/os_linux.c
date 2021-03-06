/* $Id: os_linux.c,v 1.34 2009/02/18 05:47:01 gmm Exp $
 *
 * HighPoint RAID Driver for Linux
 * Copyright (C) 2005 HighPoint Technologies, Inc. All Rights Reserved.
 */
#include "osm_linux.h"

/* hardware access */
HPT_U8   os_inb  (void *port) { return inb((unsigned)(HPT_UPTR)port); }
HPT_U16  os_inw  (void *port) { return inw((unsigned)(HPT_UPTR)port); }
HPT_U32  os_inl  (void *port) { return inl((unsigned)(HPT_UPTR)port); }

void  os_outb (void *port, HPT_U8 value) { outb(value, (unsigned)(HPT_UPTR)port); }
void  os_outw (void *port, HPT_U16 value) { outw(value, (unsigned)(HPT_UPTR)port); }
void  os_outl (void *port, HPT_U32 value) { outl(value, (unsigned)(HPT_UPTR)port); }
void  os_insw (void *port, HPT_U16 *buffer, HPT_U32 count)
{ insw((unsigned)(HPT_UPTR)port, (void *)buffer, count); }
void  os_outsw(void *port, HPT_U16 *buffer, HPT_U32 count)
{ outsw((unsigned)(HPT_UPTR)port, (void *)buffer, count); }

HPT_U8   os_readb  (void *addr) { return readb(addr); }
HPT_U16  os_readw  (void *addr) { return readw(addr); }
HPT_U32  os_readl  (void *addr) { return readl(addr); }
void     os_writeb (void *addr, HPT_U8 value)  { writeb(value, addr); }
void     os_writew (void *addr, HPT_U16 value) { writew(value, addr); }
void     os_writel (void *addr, HPT_U32 value) { writel(value, addr); }

/* PCI configuration space */
HPT_U8 os_pci_readb (void *osext, HPT_U8 offset)
{
	HPT_U8 value;

	if (pci_read_config_byte(((PHBA)osext)->pcidev, offset, &value))
		return 0xff;
	else
		return value;
}

HPT_U16 os_pci_readw (void *osext, HPT_U8 offset)
{
	HPT_U16 value;

	if (pci_read_config_word(((PHBA)osext)->pcidev, offset, &value))
		return 0xffff;
	else
		return value;
}

HPT_U32 os_pci_readl (void *osext, HPT_U8 offset)
{
	HPT_U32 value;

	if (pci_read_config_dword(((PHBA)osext)->pcidev, offset, &value))
		return 0xffffffff;
	else
		return value;
}

void os_pci_writeb(void *osext, HPT_U8 offset, HPT_U8 value)
{
	pci_write_config_byte(((PHBA)osext)->pcidev, offset, value);
}

void os_pci_writew(void *osext, HPT_U8 offset, HPT_U16 value)
{
	pci_write_config_word(((PHBA)osext)->pcidev, offset, value);
}

void os_pci_writel(void *osext, HPT_U8 offset, HPT_U32 value)
{
	pci_write_config_dword(((PHBA)osext)->pcidev, offset, value);
}

void os_stallexec(HPT_U32 microseconds)
{
	while (microseconds > 1000) {
		udelay(1000);
		microseconds -= 1000;
	}
	udelay(microseconds);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0) && defined(__x86_64__)
	touch_nmi_watchdog();
#endif
}

void *os_map_pci_bar(
	void *osext,
	int index,
	HPT_U32 offset,
	HPT_U32 length
)
{
	PHBA hba = (PHBA)osext;

	unsigned long base = pci_resource_start(hba->pcidev, index);

	if (pci_resource_flags(hba->pcidev, index) & IORESOURCE_MEM)
		return ioremap(base+offset, length);

	return (char*)base+offset;
}

void os_unmap_pci_bar(void *osext, void *base)
{
	iounmap(base);
}

HPT_U32 os_get_stamp(void)
{
	HPT_U32 stamp;
	get_random_bytes(&stamp, sizeof(stamp));
	return stamp;
}

void freelist_reserve(struct freelist *list, void *osext,
					HPT_UINT size, HPT_UINT count)
{
	PVBUS_EXT vbus_ext = osext;

	if (vbus_ext->ext_type!=EXT_TYPE_VBUS)
		vbus_ext = ((PHBA)osext)->vbus_ext;

	list->next = vbus_ext->freelist_head;
	vbus_ext->freelist_head = list;
	list->dma = 0;
	list->size = size;
	list->head = 0;
#if DBG
	list->reserved_count =
#endif
	list->count = count;
}

void *freelist_get(struct freelist *list)
{
	void * result;
	if (list->count) {
		HPT_ASSERT(list->head);
		result = list->head;
		list->head = *(void **)result;
		list->count--;
		return result;
	}
	return 0;
}

void freelist_put(struct freelist * list, void *p)
{
	HPT_ASSERT(list->dma==0);
	list->count++;
	*(void **)p = list->head;
	list->head = p;
}

void freelist_reserve_dma(struct freelist *list, void *osext,
			HPT_UINT size, HPT_UINT alignment, HPT_UINT count)
{
	PVBUS_EXT vbus_ext = osext;

	if (vbus_ext->ext_type!=EXT_TYPE_VBUS)
		vbus_ext = ((PHBA)osext)->vbus_ext;

	list->next = vbus_ext->freelist_dma_head;
	vbus_ext->freelist_dma_head = list;
	list->dma = 1;
	list->alignment = alignment;
	list->size = size;
	list->head = 0;
#if DBG
	list->reserved_count =
#endif
	list->count = count;
}

void *freelist_get_dma(struct freelist *list, BUS_ADDRESS *busaddr)
{
	void *result;
	HPT_ASSERT(list->dma);
	result = freelist_get(list);
	if (result)
		*busaddr = *(BUS_ADDRESS *)((void **)result+1);
	return result;
}

void freelist_put_dma(struct freelist *list, void *p, BUS_ADDRESS busaddr)
{
	HPT_ASSERT(list->dma);
	list->count++;
	*(void **)p = list->head;
	*(BUS_ADDRESS *)((void **)p+1) = busaddr;
	list->head = p;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,18)) && defined(CONFIG_HIGHMEM)
void *os_kmap_sgptr(PSG psg)
{
	struct page * page = (struct page *)(HPT_UPTR)(psg->addr.bus>>32);

	if (page)
		return (PageHighMem(page)?
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)  /*MiSt: commit 1ec9c5ddc17aa398f05646abfcbaf315b544e62f */
				(char *)kmap_atomic(page) :
#else
				(char *)kmap_atomic(page, HPT_KMAP_TYPE) :
#endif
				(char *)page_address(page))
			+ (psg->addr.bus & 0xffffffff);
	else
		return psg->addr._logical;
}

void os_kunmap_sgptr(void *ptr)
{
	if ((HPT_UPTR)ptr >= (HPT_UPTR)high_memory)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,6,0)  /*MiSt: commit 1ec9c5ddc17aa398f05646abfcbaf315b544e62f */
		kunmap_atomic(ptr);
#else
		kunmap_atomic(ptr, HPT_KMAP_TYPE);
#endif
}
#else
void *os_kmap_sgptr(PSG psg) { return psg->addr._logical; }
void os_kunmap_sgptr(void *ptr) {}
#endif

HPT_U64 CPU_TO_LE64(HPT_U64 x) { return cpu_to_le64(x); }
HPT_U32 CPU_TO_LE32(HPT_U32 x) { return cpu_to_le32(x); }
HPT_U16 CPU_TO_LE16(HPT_U16 x) { return cpu_to_le16(x); }
HPT_U64 LE64_TO_CPU(HPT_U64 x) { return le64_to_cpu(x); }
HPT_U32 LE32_TO_CPU(HPT_U32 x) { return le32_to_cpu(x); }
HPT_U16 LE16_TO_CPU(HPT_U16 x) { return le16_to_cpu(x); }
HPT_U64 CPU_TO_BE64(HPT_U64 x) { return cpu_to_be64(x); }
HPT_U32 CPU_TO_BE32(HPT_U32 x) { return cpu_to_be32(x); }
HPT_U16 CPU_TO_BE16(HPT_U16 x) { return cpu_to_be16(x); }
HPT_U64 BE64_TO_CPU(HPT_U64 x) { return be64_to_cpu(x); }
HPT_U32 BE32_TO_CPU(HPT_U32 x) { return be32_to_cpu(x); }
HPT_U16 BE16_TO_CPU(HPT_U16 x) { return be16_to_cpu(x); }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,4)
#define __BDEV_RAW
#else
#define __BDEV_RAW , BDEV_RAW
#endif

void refresh_sd_flags(PVBUS_EXT vbus_ext)
{
	static int major[] = { SCSI_DISK0_MAJOR, SCSI_DISK1_MAJOR, SCSI_DISK2_MAJOR, SCSI_DISK3_MAJOR,
				SCSI_DISK4_MAJOR, SCSI_DISK5_MAJOR, SCSI_DISK6_MAJOR, SCSI_DISK7_MAJOR,
				SCSI_DISK8_MAJOR, SCSI_DISK9_MAJOR, SCSI_DISK10_MAJOR, SCSI_DISK11_MAJOR,
				SCSI_DISK12_MAJOR, SCSI_DISK13_MAJOR, SCSI_DISK14_MAJOR, SCSI_DISK15_MAJOR, 0 };
	int id;
	Scsi_Device *SDptr;

	vbus_ext->needs_refresh = 0;

	for (id=0; id<osm_max_targets; id++) {

		SDptr = scsi_device_lookup(vbus_ext->host, 0, id, 0);

		vbus_ext->sd_flags[id] &= ~SD_FLAG_IN_USE;

		if (SDptr) {
			int i, minor;
			for (i=0; major[i]; i++) {
				for (minor=0; minor<=240; minor+=16) {
					struct block_device *bdev = bdget(MKDEV(major[i], minor));
					if (bdev &&
/*MiSt -- v2.6.37 API change // commit e525fd89d380c4a94c0d63913a1dd1a593ed25e7
  Added 3rd argument for the active holder, previously this was hardcoded to NULL.
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
						blkdev_get(bdev, FMODE_READ, NULL)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
						blkdev_get(bdev, FMODE_READ)
#else
						blkdev_get(bdev, FMODE_READ, 0 __BDEV_RAW)
#endif
						==0) {
						if (bdev->bd_disk) {
						    struct gendisk *cur_gendisk = bdev->bd_disk;
/*MiSt -- v4.8.0 API change // commit 52c44d93c26f5a76068c0a8cc83bb8f56f38043d
  block: remove ->driverfs_dev
  See https://lkml.org/lkml/2016/2/25/662 for hints on how to handle this change.
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)  /*MiSt -- in OpenSuSE Leap this change has been backported to kernel version 4.4 (through a patch named ‘0144-block-remove-driverfs_dev.patch’, dated 2016-06-15) */
						    struct device *gendisk_dev = disk_to_dev(cur_gendisk)->parent;
#else
						    struct device *gendisk_dev = cur_gendisk->driverfs_dev;
						    /*if (bdev->bd_disk->driverfs_dev==&SDptr->sdev_gendev) {*/
#endif
						    if (gendisk_dev==&SDptr->sdev_gendev) {
							if (vbus_ext->sd_flags[id] & SD_FLAG_REVALIDATE) {
								if (bdev->bd_disk->fops->revalidate_disk)
									bdev->bd_disk->fops->revalidate_disk(bdev->bd_disk);
/*MiSt -- v4.7.0 API change // commit 9902af79c01a8e39bb99b922fa3eef6d4ea23d69
  parallel lookups: actual switch to rwsem
  See https://lkml.org/lkml/2016/5/1/212 for hints on how to handle this change.
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
								inode_lock(bdev->bd_inode);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
								mutex_lock(&bdev->bd_inode->i_mutex);
#else
								down(&bdev->bd_inode->i_sem);
#endif
								i_size_write(bdev->bd_inode, (loff_t)get_capacity(bdev->bd_disk)<<9);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
								inode_unlock(bdev->bd_inode);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
								mutex_unlock(&bdev->bd_inode->i_mutex);
#else
								up(&bdev->bd_inode->i_sem);
#endif
								vbus_ext->sd_flags[id] &= ~SD_FLAG_REVALIDATE;
							}
							if (bdev->bd_openers>1)
								vbus_ext->sd_flags[id] |= SD_FLAG_IN_USE;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
							blkdev_put(bdev, FMODE_READ);
#else
							blkdev_put(bdev __BDEV_RAW);
#endif
							goto next;
						    }
						}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
						blkdev_put(bdev, FMODE_READ);
#else
						blkdev_put(bdev __BDEV_RAW);
#endif
					}
				}
			}
next:
			scsi_device_put(SDptr);
		}
	}
}

int os_query_remove_device(void *osext, int id)
{
	PVBUS_EXT vbus_ext = osext;
	return (vbus_ext->sd_flags[id] & SD_FLAG_IN_USE)? 1 : 0;
}

struct sd_change_wrapper
{
	struct work_struct work;
	struct Scsi_Host *host;
	int id;
	int to_register;
};

static void os_sd_changed(struct work_struct *work)
{
	struct scsi_device *sdev;
	struct sd_change_wrapper *change = container_of(work, struct sd_change_wrapper, work);

	if (change->to_register) {
		void *ptr = (void *)(HPT_UPTR)scsi_add_device(change->host, 0, change->id, 0);
		if (ptr && IS_ERR(ptr))
			os_printk("Add scsi device id%d failed", change->id);
	}
	else {
		sdev = scsi_device_lookup(change->host, 0, change->id, 0);
		if (sdev != NULL) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 23)

			scsi_device_cancel(sdev, 0);
#endif
			scsi_remove_device(sdev);
			scsi_device_put(sdev);
		}
	}
	kfree(change);
}

static void sched_sd_change(void *osext, int id, int to_register)
{
	PVBUS_EXT vbus_ext = osext;
	struct Scsi_Host *host = vbus_ext->host;
	struct sd_change_wrapper *sd_change;

	sd_change = kmalloc(sizeof(*sd_change), GFP_ATOMIC);
	if (!sd_change) {
		os_printk("sd_change mem alloc failed");
		return;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 20)
	INIT_WORK(&sd_change->work, (void (*)(void *))os_sd_changed, &sd_change->work);
#else
	INIT_WORK(&sd_change->work, os_sd_changed);
#endif

	sd_change->id = id;
	sd_change->host = host;
	sd_change->to_register = to_register;

	if (schedule_work(&sd_change->work) == 0) {
		os_printk("schedule_work failed");
		kfree(sd_change);
	}
}

void os_register_device(void *osext, int id)
{
	sched_sd_change(osext, id, 1);
}

void os_unregister_device(void *osext, int id)
{
	sched_sd_change(osext, id, 0);
}

#else /* 2.4.x */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,10)
static struct gendisk *get_gendisk(kdev_t dev)
{
	struct gendisk *g;
	int m = MAJOR(dev);
	for (g = gendisk_head; g; g = g->next)
		if (g->major == m)
			break;
	return g;
}
#endif

static int hpt_remove_scsi_device(struct Scsi_Host *host, int target_id)
{
	struct proc_dir_entry *pde;
	mm_segment_t oldfs;
	char cmd[64];
	int result = -1;

	pde = host->hostt->proc_dir;
	if (pde) pde = pde->parent;
	if (pde) pde = pde->subdir;
	while (pde) {
		if (memcmp(pde->name, "scsi", 5)==0) {
			sprintf(cmd, "scsi remove-single-device %d 0 %d 0",
						host->host_no, target_id);
			if (pde->write_proc) {
				oldfs = get_fs();
				set_fs(get_ds());
				result = pde->write_proc(0, cmd, strlen(cmd), 0);
				set_fs(oldfs);
			}
			break;
		}
		pde = pde->next;
	}
	return result;
}

void refresh_sd_flags(PVBUS_EXT vbus_ext)
{
	static int major[] = { SCSI_DISK0_MAJOR, SCSI_DISK1_MAJOR, SCSI_DISK2_MAJOR, SCSI_DISK3_MAJOR,
				SCSI_DISK4_MAJOR, SCSI_DISK5_MAJOR, SCSI_DISK6_MAJOR, SCSI_DISK7_MAJOR, 0 };
	int i, minor;

	vbus_ext->needs_refresh = 0;

	for (i=0; i<osm_max_targets; i++) {
		if (vbus_ext->sd_flags[i] & SD_FLAG_REMOVE) {
			vbus_ext->sd_flags[i] &= ~SD_FLAG_REMOVE;
			hpt_remove_scsi_device(vbus_ext->host, i);
		}
	}

	for (i=0; major[i]; i++) {
		for (minor=0; minor<=240; minor+=16) {
			struct block_device *bdev = bdget(MKDEV(major[i], minor));
			if (bdev && blkdev_get(bdev, FMODE_READ, 0, BDEV_RAW)==0) {
				Scsi_Idlun idlun;
				if (ioctl_by_bdev(bdev, SCSI_IOCTL_GET_IDLUN, (unsigned long)&idlun)==0) {
					if ((idlun.dev_id>>24)==vbus_ext->host->host_no) {
						int id = idlun.dev_id & 0xff;
						if (vbus_ext->sd_flags[id] & SD_FLAG_REVALIDATE) {
							PVDEV vd = ldm_find_target((PVBUS)vbus_ext->vbus, id);
							struct gendisk *gd = get_gendisk(MKDEV(major[i], minor));
							if (vd && gd)
								gd->sizes[minor] = gd->part[0].nr_sects = vd->capacity;
							vbus_ext->sd_flags[id] &= ~SD_FLAG_REVALIDATE;
						}
					}
				}
				blkdev_put(bdev, BDEV_RAW);
			}
		}
	}
}

int os_query_remove_device(void *osext, int id)
{
	PVBUS_EXT vbus_ext = osext;
	Scsi_Device *SDptr = vbus_ext->host->host_queue;

	while (SDptr) {
		if (SDptr->id == id)
			return SDptr->access_count;
		SDptr = SDptr->next;
	}
	return 0;
}

void os_register_device(void *osext, int id) { }

void os_unregister_device(void *osext, int id)
{
	PVBUS_EXT vbus_ext = osext;
	vbus_ext->sd_flags[id] |= SD_FLAG_REMOVE;
	vbus_ext->needs_refresh = 1;
}

#endif

int os_revalidate_device(void *osext, int id)
{
	PVBUS_EXT vbus_ext = osext;
	vbus_ext->sd_flags[id] |= SD_FLAG_REVALIDATE;
	vbus_ext->needs_refresh = 1;
	return 0;
}

/*
 * shall be called with vbus_ext->lock hold.
 * there is one exception: os_schedule_task can be called at init time
 * before the worker tasklet is initialized.
 */
void os_schedule_task(void *osext, OSM_TASK *task)
{
	PVBUS_EXT vbus_ext = osext;

	HPT_ASSERT(task->next==0);

	if (vbus_ext->tasks==0)
		vbus_ext->tasks = task;
	else {
		OSM_TASK *t = vbus_ext->tasks;
		while (t->next) t = t->next;
		t->next = task;
	}

	if (vbus_ext->worker.func)
		tasklet_schedule(&vbus_ext->worker);
}

HPT_U8 os_get_vbus_seq(void *osext)
{
	return ((PVBUS_EXT)osext)->host->host_no;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
static void os_timer_for_ldm(struct timer_list *t)
#else
static void os_timer_for_ldm(unsigned long data)
#endif
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 15, 0)
	PVBUS_EXT vbus_ext = from_timer(vbus_ext, t, timer);
#else
	PVBUS_EXT vbus_ext = (PVBUS_EXT)data;
#endif
	unsigned long flags;

	spin_lock_irqsave(vbus_ext->lock, flags);
	ldm_on_timer((PVBUS)vbus_ext->vbus);
	spin_unlock_irqrestore(vbus_ext->lock, flags);
}

void  os_request_timer(void * osext, HPT_U32 interval)
{
	PVBUS_EXT vbus_ext = osext;

	HPT_ASSERT(vbus_ext->ext_type==EXT_TYPE_VBUS);

	del_timer(&vbus_ext->timer);
	vbus_ext->timer.function = os_timer_for_ldm;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
	vbus_ext->timer.data = (unsigned long)vbus_ext;
#endif
	vbus_ext->timer.expires = jiffies + 1 + interval / (1000000/HZ);
	add_timer(&vbus_ext->timer);
}

HPT_TIME os_query_time(void)
{
	return jiffies * (1000000 / HZ);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,10)
#define vsnprintf(buf, size, fmt, args) vsprintf(buf, fmt, args)
#endif
int  os_printk(char *fmt, ...)
{
	va_list args;
	static char buf[512];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	return printk("%s:%s\n", driver_name, buf);
}

#if DBG
void __os_dbgbreak(const char *file, int line)
{
	printk("*** break at %s:%d ***", file, line);
	while (1);
}

void os_check_stack(const char *location, int size)
{
#ifdef __i386__
	long esp;
	__asm__ __volatile__("andl %%esp,%0" :
				"=r" (esp) : "0" (THREAD_SIZE - 1));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	if (esp - sizeof(struct thread_info) < size) {
#else
	if (esp - sizeof(struct task_struct) < size) {
#endif
		printk("*** %s: stack too small (0x%lx) ***\n", location, esp);
		/* dump_stack(); while (1); */
	}
#endif
}

int hpt_dbg_level = 1;
#if defined(MODULE)
#if LINUX_VERSION_CODE >KERNEL_VERSION(2, 5, 0)
module_param(hpt_dbg_level, uint, 0);
#else
MODULE_PARM(hpt_dbg_level, "i");
#endif
MODULE_PARM_DESC(hpt_dbg_level, "debug level");
#endif
#endif
