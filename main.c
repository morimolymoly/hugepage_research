#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/highmem.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <linux/percpu.h>
#include <linux/smp.h>
#include <linux/sched.h>

#include <linux/kernel_stat.h>
#include <linux/mm.h>
#include <linux/sched/mm.h>
#include <linux/sched/coredump.h>
#include <linux/sched/numa_balancing.h>
#include <linux/sched/task.h>
#include <linux/hugetlb.h>
#include <linux/mman.h>
#include <linux/swap.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/memremap.h>
#include <linux/ksm.h>
#include <linux/rmap.h>
#include <linux/export.h>
#include <linux/delayacct.h>
#include <linux/init.h>
#include <linux/pfn_t.h>
#include <linux/writeback.h>
#include <linux/memcontrol.h>
#include <linux/mmu_notifier.h>
#include <linux/swapops.h>
#include <linux/elf.h>
#include <linux/gfp.h>
#include <linux/migrate.h>
#include <linux/string.h>
#include <linux/dma-debug.h>
#include <linux/debugfs.h>
#include <linux/userfaultfd_k.h>
#include <linux/dax.h>
#include <linux/oom.h>
#include <linux/numa.h>

#include <asm/io.h>
#include <asm/mmu_context.h>
#include <asm/pgalloc.h>
#include <linux/uaccess.h>
#include <asm/tlb.h>
#include <asm/tlbflush.h>
#include <asm/pgtable.h>

#include <asm/io.h>

#include <linux/slab.h>
#include <linux/vmalloc.h>

#include <linux/kallsyms.h>
#include <linux/time.h>

#include "cmd.h"

/*** このデバイスに関する情報 ***/
MODULE_LICENSE("Dual BSD/GPL");
#define DRIVER_NAME "MyDevice"				/* /proc/devices等で表示されるデバイス名 */
static const unsigned int MINOR_BASE = 0;	/* このデバイスドライバで使うマイナー番号の開始番号と個数(=デバイス数) */
static const unsigned int MINOR_NUM  = 1;	/* マイナー番号は 0のみ */
static unsigned int mydevice_major;			/* このデバイスドライバのメジャー番号(動的に決める) */
static struct cdev mydevice_cdev;			/* キャラクタデバイスのオブジェクト */
static struct class *mydevice_class = NULL;	/* デバイスドライバのクラスオブジェクト */

static inline pud_t pud_mkexec(pud_t pud)
{
	return pud_clear_flags(pud, _PAGE_NX);
}

static inline int pud_exec(pud_t pud)
{
	return !(pud_flags(pud) & _PAGE_NX);
}

static void (*flush_tlb_all_p)(void);

static pte_t *walk_page_table(struct mm_struct *mm, size_t addr)
{
	printk("walk_page_table %llx\n", addr);

    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *ptep;

	// pgd
    pgd = pgd_offset(mm, addr);
    if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd))){
		printk("pgd none or pgd bad\n");
        return NULL;
	}

	// pud
    pud = pud_offset(pgd, addr);
	if (pud_large(*pud)) {
		printk("pud is large!!!!\n");
	}
    if (pud_none(*pud)){
		printk("pud none\n");
    	return NULL;
	}
	if(unlikely(pud_bad(*pud))){
		printk("pud is bad\n");
		return NULL;
	}

	// pmd
    pmd = pmd_offset(pud, addr);
	if (pmd_large(*pmd)) {
		printk("pmd is large!!!!\n");
	}
    if (pmd_none(*pmd)) {
		printk("pmd none or pmd bad\n");
		return NULL;
	}

	printk(KERN_ERR "MOLY pmd_val= 0x%lx\n", pmd_val(*pmd));
	printk(KERN_ERR "MOLY: pmd %llx\n", virt_to_phys(pmd));


    ptep = pte_offset_map(pmd, addr);
	printk(KERN_ERR "MOLY pte_val= 0x%lx\n", pte_val(*ptep));
	printk(KERN_ERR "MOLY: pte %llx\n", virt_to_phys(ptep));

	if (pte_huge(*ptep)) {
		printk("pte is huge!!!!\n");
	}

    return ptep;
}


static pte_t *walk_page_table_end_make_change_for_nx_bit_pud(struct mm_struct *mm, size_t addr)
{
	printk("walk_page_table %llx\n", addr);

    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *ptep;

	// pgd
    pgd = pgd_offset(mm, addr);
    if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd))){
		printk("pgd none or pgd bad\n");
        return NULL;
	}

	// pud
    pud = pud_offset(pgd, addr);
	if (pud_large(*pud)) {
		printk("pud is large!!!!\n");

		printk("NX bit before: %d\n", pud_exec(*pud));
		printk("make executable\n");
		pud_mkexec(*pud);
		printk("NX bit after: %d\n", pud_exec(*pud));

		unsigned long pfn;
		pfn = pud_pfn(*pud);
		set_pud(pud, *pud);
		(*flush_tlb_all_p)();
	}
    if (pud_none(*pud)){
		printk("pud none\n");
    	return NULL;
	}
	if(unlikely(pud_bad(*pud))){
		printk("pud is bad\n");
		return NULL;
	}

	// pmd
    pmd = pmd_offset(pud, addr);
	if (pmd_large(*pmd)) {
		printk("pmd is large!!!!\n");
	}
    if (pmd_none(*pmd)) {
		printk("pmd none or pmd bad\n");
		return NULL;
	}

	printk(KERN_ERR "MOLY pmd_val= 0x%lx\n", pmd_val(*pmd));
	printk(KERN_ERR "MOLY: pmd %llx\n", virt_to_phys(pmd));


    ptep = pte_offset_map(pmd, addr);
	printk(KERN_ERR "MOLY pte_val= 0x%lx\n", pte_val(*ptep));
	printk(KERN_ERR "MOLY: pte %llx\n", virt_to_phys(ptep));

	if (pte_huge(*ptep)) {
		printk("pte is huge!!!!\n");
	}

    return ptep;
}

/* ioctlテスト用に値を保持する変数 */
static struct value stored_values;
void* gigantic_address=0;
unsigned long virthuge;
int (*funcp)(void);
int (*funcp2)(void);
int (*funcp3)(void);

void get_gigantic_page(void)
{
	struct page **pages;
	int retval;
	unsigned long npages;
	unsigned long buffer_start = stored_values.addr;
	void *remapped;

	npages = 1 + (0x40000000 -1 ) / PAGE_SIZE;
	pages = vmalloc(npages * sizeof(struct page*));

	down_read(&current->mm->mmap_sem);
	retval = get_user_pages(buffer_start, npages, (FOLL_FORCE | FOLL_WRITE), pages, NULL);
	printk(KERN_INFO "target addr: 0x%llx\n", buffer_start);
	printk(KERN_INFO "get user pages 0x%llx\n", retval);
	if(retval){
		printk(KERN_INFO "got mapped\n");
		unsigned long page_va = kmap(pages);
		u64 phyhuge = page_to_phys(pages[0]);
		printk(KERN_INFO "phy addr: 0x%llx\n", phyhuge);
		virthuge = phys_to_virt(phyhuge);
		printk(KERN_INFO "virt addr: 0x%llx\n", virthuge);
		walk_page_table_end_make_change_for_nx_bit_pud(current->mm, virthuge);

		funcp = virthuge;
		__free_page(pages[0]);
	}
	up_read(&current->mm->mmap_sem);
}

void nan(void)
{
	1+10;
}

#define TIMECOUNTER 2000000000

/* ioctl時に呼ばれる関数 */
static long mydevice_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk("mydevice_ioctl\n");
	
	switch (cmd) {
	case HUGE_SET_VALUE:
		printk("MYDEVICE_SET_VALUES\n");
		if (copy_from_user(&stored_values, (void __user *)arg, sizeof(stored_values))) {
			return -EFAULT;
		}
		if(stored_values.addr != 0){
			get_gigantic_page();
		}
		break;
	case HUGE_TIME1GB:
		asm volatile("nop");
		printk("mydevice 1gb\n");
		for(int i=0; i<TIMECOUNTER; i++) {
			(*funcp)();
			nan();
		}
		break;
	case HUGE_TIME2MB:
		asm volatile("nop");
		printk("mydevice 2mb\n");
		for(int i=0; i<TIMECOUNTER; i++) {
			(*funcp2)();
			nan();
		}
		break;
	case HUGE_TIME4KB:
		asm volatile("nop");
		printk("mydevice 4kb\n");
		for(int i=0; i<TIMECOUNTER; i++) {
			(*funcp3)();
			nan();
		}
		break;
	case HUGE_WALK:
		printk("mydevice walk gigantic page\n");
		walk_page_table(current->mm, virthuge);
		break;
	/*
	case MYDEVICE_GET_VALUES:
		printk("MYDEVICE_GET_VALUES\n");
		if (copy_to_user((void __user *)arg, &stored_values, sizeof(stored_values))) {
			return -EFAULT;
		}
		break;*/
	default:
		printk(KERN_WARNING "unsupported command %d\n", cmd);
		return -EFAULT;
	}
	return 0;
}

static int mydevice_open(struct inode *inode, struct file *file)
{
	printk("mydevice_open\n");
	return 0;
}

static int mydevice_close(struct inode *inode, struct file *file)
{
	printk("mydevice_close\n");
	return 0;
}

static ssize_t mydevice_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	printk("mydevice_read\n");
	return count;
}

static ssize_t mydevice_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	printk("mydevice_write\n");
	return count;
}

static struct file_operations mydevice_fops = {
	.open    = mydevice_open,
	.release = mydevice_close,
	.read    = mydevice_read,
	.write   = mydevice_write,
	.unlocked_ioctl = mydevice_ioctl,
	.compat_ioctl   = mydevice_ioctl,	// for 32-bit App
};

char shellcode[] = "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\xc3";
void alloc_4kb_page_with_kmalloc_and_check_how_is_it(void)
{
	printk("\nalloc_4kb_page_with_kmalloc_and_check_how_is_it");
	void *tes = kmalloc(0x1000, GFP_KERNEL | __GFP_COMP);
	printk("funcp2 tested\n");
	if(!tes) {
		printk("failed to allocate!\n");
	} else {
		memcpy(tes, shellcode, strlen(shellcode));
		funcp3 = tes;
		(*funcp3)();
		walk_page_table(current->mm, tes);
	}
}


void alloc_2mb_page_with_kmalloc_and_check_how_is_it(void)
{
	printk("\nalloc_2mb_page_with_kmalloc_and_check_how_is_it");
	void *tes = kmalloc(0x200000, GFP_KERNEL | __GFP_COMP | VM_HUGETLB);
	printk("funcp2 tested\n");
	if(!tes) {
		printk("failed to allocate!\n");
	} else {
		memcpy(tes, shellcode, strlen(shellcode));
		funcp2 = tes;
		(*funcp2)();
		walk_page_table(current->mm, tes);
	}
}

static int mydevice_init(void)
{
	printk("mydevice_init\n");
	flush_tlb_all_p = (void*) kallsyms_lookup_name("flush_tlb_all");

	int alloc_ret = 0;
	int cdev_err = 0;
	dev_t dev;

	/* 空いているメジャー番号を確保する */
	alloc_ret = alloc_chrdev_region(&dev, MINOR_BASE, MINOR_NUM, DRIVER_NAME);
	if (alloc_ret != 0) {
		printk(KERN_ERR  "alloc_chrdev_region = %d\n", alloc_ret);
		return -1;
	}

	/* 取得したdev( = メジャー番号 + マイナー番号)からメジャー番号を取得して保持しておく */
	mydevice_major = MAJOR(dev);
	dev = MKDEV(mydevice_major, MINOR_BASE);	/* 不要? */

	/* cdev構造体の初期化とシステムコールハンドラテーブルの登録 */
	cdev_init(&mydevice_cdev, &mydevice_fops);
	mydevice_cdev.owner = THIS_MODULE;

	/* このデバイスドライバ(cdev)をカーネルに登録する */
	cdev_err = cdev_add(&mydevice_cdev, dev, MINOR_NUM);
	if (cdev_err != 0) {
		printk(KERN_ERR  "cdev_add = %d\n", alloc_ret);
		unregister_chrdev_region(dev, MINOR_NUM);
		return -1;
	}

	/* このデバイスのクラス登録をする(/sys/class/mydevice/ を作る) */
	mydevice_class = class_create(THIS_MODULE, "mydevice");
	if (IS_ERR(mydevice_class)) {
		printk(KERN_ERR  "class_create\n");
		cdev_del(&mydevice_cdev);
		unregister_chrdev_region(dev, MINOR_NUM);
		return -1;
	}

	/* /sys/class/mydevice/mydevice* を作る */
	for (int minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
		device_create(mydevice_class, NULL, MKDEV(mydevice_major, minor), NULL, "mydevice%d", minor);
	}

	alloc_4kb_page_with_kmalloc_and_check_how_is_it();
	alloc_2mb_page_with_kmalloc_and_check_how_is_it();

	return 0;
}

/* アンロード(rmmod)時に呼ばれる関数 */
static void mydevice_exit(void)
{
	printk("mydevice_exit\n");

	dev_t dev = MKDEV(mydevice_major, MINOR_BASE);
	
	/* /sys/class/mydevice/mydevice* を削除する */
	for (int minor = MINOR_BASE; minor < MINOR_BASE + MINOR_NUM; minor++) {
		device_destroy(mydevice_class, MKDEV(mydevice_major, minor));
	}

	/* このデバイスのクラス登録を取り除く(/sys/class/mydevice/を削除する) */
	class_destroy(mydevice_class);

	/* このデバイスドライバ(cdev)をカーネルから取り除く */
	cdev_del(&mydevice_cdev);

	/* このデバイスドライバで使用していたメジャー番号の登録を取り除く */
	unregister_chrdev_region(dev, MINOR_NUM);
}

module_init(mydevice_init);
module_exit(mydevice_exit);

