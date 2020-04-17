/****************************************************************************
 * config.c - auto-generated file
 ****************************************************************************/
#include "osm_linux.h"

#define INIT_MODULE(mod) \
	do {\
		extern int init_module_##mod(void);\
		init_module_##mod();\
	} while(0)

int init_config(void)
{
	INIT_MODULE(him_sx508x);
	INIT_MODULE(vdev_raw);
	INIT_MODULE(partition);
	INIT_MODULE(raid0);
	INIT_MODULE(raid1);
	INIT_MODULE(raid5);
	INIT_MODULE(jbod);
	return 0;
}

char driver_name[] = "rr232x";
char driver_name_long[] = "RocketRAID 232x controller driver";

/*MiSt: gcc 4.9 (and above) warns when the __DATE__ and __TIME__ macros
        are used: https://gcc.gnu.org/gcc-4.9/changes.html */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdate-time"
#endif
char driver_ver[] = "v1.10 (" __DATE__ " " __TIME__ ")";
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
#pragma GCC diagnostic pop
#endif
int  osm_max_targets = 32;

int os_max_cache_size = 0x800000;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static const struct pci_device_id hpt_pci_tbl[] = {
	{PCI_DEVICE(0x1103, 0x2320), 0, 0, 0},
	{PCI_DEVICE(0x1103, 0x2322), 0, 0, 0},
	{}
};
MODULE_DEVICE_TABLE(pci, hpt_pci_tbl);
#endif
