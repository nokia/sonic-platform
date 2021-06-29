#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>  // copy to/from user
#include <linux/delay.h>

MODULE_AUTHOR("Nokia Corporation");
MODULE_DESCRIPTION("BDE-BDB Helper Module");
MODULE_LICENSE("GPL");

#define KERNEL_MOD_NAME "nokia-kernel-bdb"
#define USER_MOD_NAME   "nokia-user-bdb"
#define KERNEL_MAJOR    119

#define KINFO KERN_INFO KERNEL_MOD_NAME ": "
#define KWARN KERN_WARNING KERNEL_MOD_NAME ": "

static void nokia_dump(struct seq_file *m);
static int nokia_ioctl(unsigned int cmd, unsigned long arg);

static int use_count = 0;

static int _proc_show(struct seq_file *m, void *v)
{
    nokia_dump(m);
    return 0;
}

static int _proc_open(struct inode * inode, struct file * file) 
{
    return single_open(file, _proc_show, NULL);
}

static ssize_t _proc_write(struct file *file, const char *buffer,
                   size_t count, loff_t *loff)
{
    return count;
}

static int _proc_release(struct inode * inode, struct file * file)
{
    return single_release(inode, file);
}

struct file_operations _proc_fops = {
    owner:      THIS_MODULE,
    open:       _proc_open,
    read:       seq_read,
    llseek:     seq_lseek,
    write:      _proc_write,
    release:    _proc_release,
};

/* FILE OPERATIONS */

static long _ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    //printk(KINFO ": ioctl %d %lx\n", cmd, arg);

    return nokia_ioctl(cmd, arg);
}

static int _open(struct inode *inode, struct file *filp)
{
    use_count++;

    printk(KINFO ": _open %p %p\n", inode, filp);

    return 0;
}

static int _release(struct inode *inode, struct file *filp)
{
    printk(KINFO ": _release %p %p\n", inode, filp);

    use_count--;

    return 0;
}

//static int _mmap(struct file *filp, struct vm_area_struct *vma)
//{
//    return -1;
//}

struct file_operations _fops = 
{
    owner:      THIS_MODULE,
    unlocked_ioctl: _ioctl,
    open:           _open,
    release:        _release,
//    mmap:           _mmap,
    compat_ioctl:   _ioctl,
};


static int __init nokia_bdb_init(void)
{
    void * ent1;

    int rc = register_chrdev(KERNEL_MAJOR, KERNEL_MOD_NAME, &_fops);
    if (rc < 0)
    {
	    printk(KWARN "can't get major %d", KERNEL_MAJOR);
	    return rc;
    }

    ent1 = proc_create(KERNEL_MOD_NAME, S_IRUGO | S_IWUGO, NULL, &_proc_fops);
    
    printk(KINFO "proc_create 2 = %p, kern_major=%d\n", ent1, KERNEL_MAJOR);

    return 0;    // Non-zero return means that the module couldn't be loaded.
}

static void __exit nokia_bdb_exit(void)
{
    remove_proc_entry(KERNEL_MOD_NAME, NULL);
    unregister_chrdev(KERNEL_MAJOR, KERNEL_MOD_NAME);

    printk(KERN_INFO "exit\n");
}

module_init(nokia_bdb_init);
module_exit(nokia_bdb_exit);

// Broadcom constants

/* Ioctl control structure */
typedef struct  {
    unsigned int dev;   /* Device ID */
    unsigned int rc;    /* Operation Return Code */
    unsigned int d0;    /* Operation specific data */
    unsigned int d1;
    unsigned int d2;
    unsigned int d3;    
    uint64_t p0;        // bde_kernel_addr_t
    union {
        unsigned int dw[2];
        unsigned char buf[64];
    } dx;
} lubde_ioctl_t;


/* LUBDE ioctls */
#define LUBDE_MAGIC 'L'

#define LUBDE_VERSION             _IO(LUBDE_MAGIC, 0)
#define LUBDE_GET_NUM_DEVICES     _IO(LUBDE_MAGIC, 1)
#define LUBDE_GET_DEVICE          _IO(LUBDE_MAGIC, 2)
#define LUBDE_PCI_CONFIG_PUT32    _IO(LUBDE_MAGIC, 3)
#define LUBDE_PCI_CONFIG_GET32    _IO(LUBDE_MAGIC, 4)
#define LUBDE_GET_DMA_INFO        _IO(LUBDE_MAGIC, 5)
#define LUBDE_ENABLE_INTERRUPTS   _IO(LUBDE_MAGIC, 6)
#define LUBDE_DISABLE_INTERRUPTS  _IO(LUBDE_MAGIC, 7)
#define LUBDE_USLEEP              _IO(LUBDE_MAGIC, 8)
#define LUBDE_WAIT_FOR_INTERRUPT  _IO(LUBDE_MAGIC, 9)
#define LUBDE_SEM_OP              _IO(LUBDE_MAGIC, 10)
#define LUBDE_UDELAY              _IO(LUBDE_MAGIC, 11)
#define LUBDE_GET_DEVICE_TYPE     _IO(LUBDE_MAGIC, 12)
#define LUBDE_SPI_READ_REG        _IO(LUBDE_MAGIC, 13)
#define LUBDE_SPI_WRITE_REG       _IO(LUBDE_MAGIC, 14)
#define LUBDE_READ_REG_16BIT_BUS  _IO(LUBDE_MAGIC, 19)
#define LUBDE_WRITE_REG_16BIT_BUS _IO(LUBDE_MAGIC, 20)
#define LUBDE_GET_BUS_FEATURES    _IO(LUBDE_MAGIC, 21)
#define LUBDE_WRITE_IRQ_MASK      _IO(LUBDE_MAGIC, 22)
#define LUBDE_CPU_WRITE_REG       _IO(LUBDE_MAGIC, 23)
#define LUBDE_CPU_READ_REG        _IO(LUBDE_MAGIC, 24)
#define LUBDE_CPU_PCI_REGISTER    _IO(LUBDE_MAGIC, 25)
#define LUBDE_DEV_RESOURCE        _IO(LUBDE_MAGIC, 26)
#define LUBDE_IPROC_READ_REG      _IO(LUBDE_MAGIC, 27)
#define LUBDE_IPROC_WRITE_REG     _IO(LUBDE_MAGIC, 28)
#define LUBDE_ATTACH_INSTANCE     _IO(LUBDE_MAGIC, 29)
#define LUBDE_GET_DEVICE_STATE    _IO(LUBDE_MAGIC, 30)
#define LUBDE_REPROBE             _IO(LUBDE_MAGIC, 31)

#define LUBDE_SUCCESS 0
#define LUBDE_FAIL ((unsigned int)-1)

#define KBDE_VERSION    2

#define BDE_DEV_STATE_NORMAL        0

#define BDE_SWITCH_DEV_TYPE       0x00100           // SAL_SWITCH_DEV_TYPE
#define BDE_PCI_DEV_TYPE          0x00001           // SAL_PCI_DEV_TYPE
#define BDE_DEV_BUS_ALT           0x04000           // SAL_DEV_BUS_ALT - legacy
#define BDE_USER_DEV_TYPE         0x40000           // SAL_USER_DEV_TYPE - added by BCM to support BDB


// NOKIA STUFFS

typedef unsigned char		uint8;		/* 8-bit quantity  */
typedef unsigned short		uint16;		/* 16-bit quantity */
typedef unsigned int		uint32;		/* 32-bit quantity LP64 */
typedef unsigned long		uint64;		/* 64-bit quantity LP64 */


/* Debug output */
static int nokia_debug;

module_param(nokia_debug, int, 0);
MODULE_PARM_DESC(nokia_debug,"Set debug level (default 0");

#include <linux/mutex.h>

#define NOKIA_DEV_NAME                      "nokia-bdb"

#define VALID_DEVICE(_n)                    (_n < MAX_NOKIA_RAMONS)
#define IS_NOKIA_DEV(d)                     (VALID_DEVICE(d) && nokia_dev[d].is_valid)
#define MAX_NOKIA_RAMONS                    18                              // may not all be present. Must not exceed LINUX_BDE_MAX_DEVICES
#define POSTED_READ                         1
#define DEV_TO_RAMON_HWSLOT(d)              (nokia_dev[d].hw_slot)           // starts at zero
#define DEFAULT_RAMON_BASE_HW_SLOT          17
#define SFM_NUM_TO_SFM_INDEX(_s)            ((_s)-1)
#define IOCTL_BASE                          _cpuctl_base_addr
#define A32_CPUCTL_BASE                     0x4000000000
#define CPUCTL_SIZE                         (384*1024*1024)     // 256MB plus 128MB BDB window
#define BDB_MIN_FIFO_DEPTH                  40

#define LUBDE_NOKIA_OP_ADD_UNIT             _IO(LUBDE_MAGIC, 100)
#define LUBDE_NOKIA_OP_BDB_INIT             _IO(LUBDE_MAGIC, 101)
#define LUBDE_NOKIA_OP_BDB_READ             _IO(LUBDE_MAGIC, 102)
#define LUBDE_NOKIA_OP_BDB_WRITE            _IO(LUBDE_MAGIC, 103)

#define BDB_BITS_DEFAULT                    (B_GEN_CONFIG_BDB_ENABLE | M_GEN_CONFIG_RTCCF_HOLD | M_GEN_CONFIG_RTCCF_ACTIVE | M_GEN_CONFIG_RTCCF_SETUP)

#define SWAP32(_x)                          ({ uint32_t x = (_x); ((((x) & 0x000000ff) << 24) | (((x) & 0x0000ff00) <<  8) | (((x) & 0x00ff0000) >>  8) | (((x) & 0xff000000) >> 24)); })
#define NELEMENTS(_a)                       (sizeof(_a) / sizeof((_a)[0]))

#define GIG_2                               (0x80000000)
#define MEG_16                              (16*1024*1024)
#define MEG_32                              (32*1024*1024)
#define MEG_64                              (64*1024*1024)
#define MEG_96                              (96*1024*1024)

#define A32_SFM_FE_DEFAULT_BAR0             (GIG_2 + MEG_32)        // 0x82000000 - BAR0 for V4 SFM (no PCIe switch)
#define BCM_FE9600_PCI_VENDOR_ID            (0x14e4)
#define BCM_FE9600_PCI_DEVICE_ID            (0x8790)

#define RAMON_BAR0(_u)                      (A32_SFM_FE_DEFAULT_BAR0 + (_u) * MEG_32)

#define RAMON_MAIN_BASE(_u)                 (RAMON_BAR0(_u) + MEG_16)7
#define RAMON_IPROC_BASE(_u)                (RAMON_BAR0(_u))

#define read32(va)                          (*(volatile uint32 *) (va))
#define write32(va,d)                       *(volatile uint32 *) (va) = (d)
#define BDB_WAIT_US                         1000
#define HW_BDB_CARD_PRESENT(_s)             !!(bdbSignalReg() & (1<<(_s)))


// hardware register offsets
#define M_BDB_SIGNAL_WFIFO_DEPTH    0xFF000000
#define S_BDB_SIGNAL_WFIFO_DEPTH    24

#define B_GEN_CONFIG_BDB_ENABLE     0x00000001
#define B_GEN_CONFIG_P_READ_DONE    0x00000002  /* BDB posted read done */
#define B_GEN_CONFIG_P_READ_ERR     0x00000004  /* BDB posted read failed */
#define M_GEN_CONFIG_BDB_SLOT       0x000000F8  /* Back door bus slot select */
#define M_GEN_CONFIG_BDB_3127       0x00001F00  /* BDB bits 31:27 on target */
#define B_GEN_CONFIG_P_READ         0x00002000  /* Enable BDB posted reads */
#define M_GEN_CONFIG_RTCCF_HOLD     0x0000C000  /* # RTC/CF hold time clks */
#define M_GEN_CONFIG_RTCCF_ACTIVE   0x001F0000  /* # RTC/CF active clks 104Mhz*/
#define M_GEN_CONFIG_RTCCF_SETUP    0x00E00000  /* # RTC/CF setup clks 104Mhz */
#define M_GEN_CONFIG_VERSION        0xFF000000  /* FPGA Version */
#define M_GEN_CONFIG_BDB_RESP_SLOT  0x1F000000  /* parallel BDB response slot */

#define S_GEN_CONFIG_BDB_SLOT       3
#define S_GEN_CONFIG_BDB_3127       8
#define S_GEN_CONFIG_RTCCF_HOLD     14
#define S_GEN_CONFIG_RTCCF_ACTIVE   16
#define S_GEN_CONFIG_RTCCF_SETUP    21
#define S_GEN_CONFIG_VERSION        24
#define S_GEN_CONFIG_BDB_RESP_SLOT  24

#define IOCPUCTL_VERSION_OFFSET     0x00800070
#define IOCPUCTL_PCIE_BDF           0x008000A8
#define IOCPUCTL_PCIE_CFG           0x008000D8
#define IOCPUCTL_CARDTYPE_OFFSET    0x008000E0

#define IOCTL_BDB_REGS_OFFSET       0x02700000
#define IOCTL_BDB_WINDOW_OFFSET     0x10000000

#define BDB_WINDOW_SIZE             (128*1024*1024)
#define BDB_REGS_SIZE               0x100
#define BDB_CTRL_REG_OFF            0x00
#define BDB_SLOT_REG_OFF            0x04
#define BDB_SIGNAL_REG_OFF          0x10
#define BDB_ERROR_REG_OFF           0x14
#define BDB_POSTED_READ_REG_OFF     0x20




struct
{
    int         is_valid;
    uint32      unit;
    uint32      device_id;
    uint32      device_rev;
    uint32      dma_offset;
    uint32      sfm_num;
    uint32      hw_slot;
    uint32      hw_main_baseaddr;
    uint32      hw_iproc_baseaddr;
} nokia_dev[MAX_NOKIA_RAMONS];

static void * _cpuctl_base_addr;
static int msgCount = 10;

static DEFINE_MUTEX(bdb_lock);
static DEFINE_MUTEX(iproc_lock);            // since BDB mutex is not recursive


static void nokia_dump(struct seq_file *m)
{
    int idx;

    seq_printf(m, "Nokia-bdb v3 units (bdb base %p, use_count %d):\n", _cpuctl_base_addr, use_count);

    for (idx = 0; idx < MAX_NOKIA_RAMONS; idx++) 
    {
        if (IS_NOKIA_DEV(idx))
            seq_printf(m, "\t%d (swi) : PCI device %s:%d:%d on Nokia SFM module hwslot %d\n", idx, NOKIA_DEV_NAME, nokia_dev[idx].sfm_num, nokia_dev[idx].unit, nokia_dev[idx].hw_slot);
    }

    msgCount = 0;       // output BDB again
}

uint32 bdbSignalReg(void)
{
    void * bdb_regs   = IOCTL_BASE + IOCTL_BDB_REGS_OFFSET;
    return(SWAP32(read32(bdb_regs + BDB_SIGNAL_REG_OFF)));
}

int bdbReadWord(uint32 hwSlot, uint32 addr, int wsize, void * ret)
{
    void * bdb_regs   = IOCTL_BASE + IOCTL_BDB_REGS_OFFSET;
    void * bdb_window = IOCTL_BASE + IOCTL_BDB_WINDOW_OFFSET;
    void * ptr;
    uint32 val, i;
    static int errdone = 0;
    int rc = LUBDE_SUCCESS;

    // if slot not present, fail cycle. 
    if (!HW_BDB_CARD_PRESENT(hwSlot))
        return LUBDE_FAIL;

    // lock access to ioctl r/m/w and bdb addr/data. Do we need to save/restore any regs?
    mutex_lock(&bdb_lock);

    // flush posted read
    val = read32(bdb_regs + BDB_CTRL_REG_OFF);
    if (val & B_GEN_CONFIG_P_READ_DONE)
        read32(bdb_regs + BDB_POSTED_READ_REG_OFF);

    val = BDB_BITS_DEFAULT | B_GEN_CONFIG_P_READ | (hwSlot << S_GEN_CONFIG_BDB_SLOT) | ((addr >> 27) << S_GEN_CONFIG_BDB_3127);
    write32(bdb_regs + BDB_CTRL_REG_OFF, SWAP32(val));

    ptr = bdb_window + (addr & ((1<<27)-1));

    /* do the read (volatile as result is thrown away)  */
    if (wsize == 1)        *(uint8_t  *)ret = *(volatile uint8_t  *)ptr;
    else if (wsize == 2)   *(uint16_t *)ret = *(volatile uint16_t *)ptr;
    else if (wsize == 4)   *(uint32_t *)ret = *(volatile uint32_t *)ptr;
    else                   *(uint64_t *)ret = *(volatile uint64_t *)ptr;

    // wait for result
    for (i=0; i<BDB_WAIT_US; i++)        // 10us plenty?
    {
        val = SWAP32(read32(bdb_regs + BDB_CTRL_REG_OFF));
        if (val & B_GEN_CONFIG_P_READ_DONE)
            break;
        udelay(1);
    }

    // the result is in the posted_read_reg register, unsure why this isn't addr & 7 but this is what it sez in srl/panos
    ptr = bdb_regs + BDB_POSTED_READ_REG_OFF + (addr & 3);
    if (wsize == 1)        *(uint8_t *)ret  = *(volatile uint8_t *)ptr;
    else if (wsize == 2)   *(uint16_t *)ret = *(volatile uint16_t *)ptr;
    else if (wsize == 4)   *(uint32_t *)ret = *(volatile uint32_t *)ptr;
    else                   *(uint64_t *)ret = *(volatile uint64_t *)ptr;

    mutex_unlock(&bdb_lock);

    if (i >= BDB_WAIT_US)
    {
        if (errdone < 10)
        {
            printk(KWARN "Slot %d BDB read timeout %dus from %x (stat=%x) sig=%x\n", hwSlot, i, addr, val, bdbSignalReg());
            errdone++;
        }
        rc = LUBDE_FAIL;
    }

    return rc;

} 

int bdbRead32(int d, uint32_t addr, uint32_t *ret)
{
    return (bdbReadWord(DEV_TO_RAMON_HWSLOT(d), addr, 4, ret));
}

int bdbWriteWord(uint32 hwSlot, uint32 addr, int wsize, void * data)
{
    void * bdb_regs   = IOCTL_BASE + IOCTL_BDB_REGS_OFFSET;
    void * bdb_window = IOCTL_BASE + IOCTL_BDB_WINDOW_OFFSET;
    void * ptr;
    uint32 val;

    // if slot not present, fail cycle. 
    if (!HW_BDB_CARD_PRESENT(hwSlot) || wsize > 32)
        return LUBDE_FAIL;

//    printf("bdbWrite32(%d, %x, %x) \n", d, addr, val);

    mutex_lock(&bdb_lock);

    // flush posted read
    val = read32(bdb_regs + BDB_CTRL_REG_OFF);
    if (val & B_GEN_CONFIG_P_READ_DONE)
        read32(bdb_regs + BDB_POSTED_READ_REG_OFF);
    
    // check for write overrun (busy-wait!)
    while ((bdbSignalReg() >> S_BDB_SIGNAL_WFIFO_DEPTH) >= (BDB_MIN_FIFO_DEPTH+8-wsize));

    val = BDB_BITS_DEFAULT | B_GEN_CONFIG_P_READ | (hwSlot << S_GEN_CONFIG_BDB_SLOT) | ((addr >> 27) << S_GEN_CONFIG_BDB_3127);
    write32(bdb_regs + BDB_CTRL_REG_OFF, SWAP32(val));

    // write the data
    ptr = bdb_window + (addr & ((1<<27)-1));
    if (wsize == 1)         *(volatile uint8_t *)ptr  = *(uint8_t *)data;
    else if (wsize == 2)    *(volatile uint16_t *)ptr = *(uint16_t *)data;
    else if (wsize == 4)    *(volatile uint32_t *)ptr = *(uint32_t *)data;
    else while (wsize)    { *(volatile uint64_t *)ptr = *(uint64_t *)data; ptr += 8; data += 8; wsize -= 8; }
    mutex_unlock(&bdb_lock);

    return LUBDE_SUCCESS;           // no write acks so never failz
} 

int bdbWrite32(int d, uint32 addr, uint32 data)
{
    return(bdbWriteWord(DEV_TO_RAMON_HWSLOT(d), addr, 4, &data));
}

#define BAR0_PAXB_IMAP0_7                       (0x2c1c)

static uint32 iproc_map_addr(int d, unsigned int addr)
{
    uint32 data, subwin_base =(addr & ~0xfff);        // 4K subwindow
    uint32 iprocBase = nokia_dev[d].hw_iproc_baseaddr;

    if ((subwin_base == 0x10231000) || (subwin_base == 0x18013000)) 
    {
        /* Route the INTC block access through IMAP0_6 */
        addr = 0x6000 + (addr & 0xfff);
    }
    else 
    {
        /* Update base address for sub-window 7 */
        bdbWrite32(d, iprocBase + BAR0_PAXB_IMAP0_7, subwin_base | 1);    // 1 is valid bit
        bdbRead32(d, iprocBase + BAR0_PAXB_IMAP0_7, &data);
        // could check result
        
        /* Read register through sub-window 7 */
        addr = 0x7000 + (addr & 0xfff);
    }

    return(iprocBase + addr);
}

static int nokia_ioctl(unsigned int cmd, unsigned long arg)
{
    lubde_ioctl_t io;
    int rc = 0;

    if (copy_from_user(&io, (void *)arg, sizeof(io))) 
        return -EFAULT;
  
    io.rc = LUBDE_SUCCESS;

    // printk(KINFO "Nokia ioctl cmd %x: %d %x %x %x\n", cmd, io.dev, io.d0, io.d1, io.d2);

    switch (cmd)
    {
    case LUBDE_VERSION:
        io.d0 = KBDE_VERSION;
        break;
    case LUBDE_GET_NUM_DEVICES:
        io.d0 = MAX_NOKIA_RAMONS;                  // needs to return all possible devices, even ones that are not present, otherwise will not iterate through all bits of _inst_dev_mask
        break;
    case LUBDE_ATTACH_INSTANCE:
        break;                                     // no need to do anything here as we have no DMA memory
    case LUBDE_GET_DEVICE:
        io.d0 = nokia_dev[io.dev].device_id;       // BCM88790_DEVICE_ID
        io.d1 = nokia_dev[io.dev].device_rev;
        io.dx.dw[0] = io.dev;                      // probably not used (unique_id)
        io.d2 = 0;                                 // zero memory addresses
        io.d3 = 0;
        break;
    case LUBDE_GET_DEVICE_TYPE:
        io.d0 = BDE_PCI_DEV_TYPE | BDE_DEV_BUS_ALT | BDE_USER_DEV_TYPE | BDE_SWITCH_DEV_TYPE;
        break;
    case LUBDE_GET_BUS_FEATURES:
        io.d0 = io.d1 = io.d2 = 0;      // no big endian anywhere
        break;
    case LUBDE_GET_DMA_INFO:
        io.d0 = io.d1 = 0;      // zero size / address means no dma
        break;
    case LUBDE_READ_REG_16BIT_BUS:
        io.rc = bdbRead32(io.dev, nokia_dev[io.dev].hw_main_baseaddr + io.d0, &io.d1);
        break;
    case LUBDE_WRITE_REG_16BIT_BUS:
        io.rc = bdbWrite32(io.dev, nokia_dev[io.dev].hw_main_baseaddr + io.d0, io.d1);
        break;
    case LUBDE_CPU_WRITE_REG:
        printk(KWARN "%d: LUBDE_CPU_WRITE_REG %x\n", io.dev, io.d0);
        io.rc = LUBDE_FAIL;
        break;
    case LUBDE_CPU_READ_REG:
        printk(KWARN "%d: LUBDE_CPU_READ_REG %x\n", io.dev, io.d0);
        io.rc = LUBDE_FAIL;
        break;
    case LUBDE_CPU_PCI_REGISTER:
        printk(KWARN "%d: LUBDE_CPU_PCI_REGISTER\n", io.dev);
        io.rc = LUBDE_FAIL;
        break;
    case LUBDE_IPROC_READ_REG:
        mutex_lock(&iproc_lock);
        io.rc = bdbRead32(io.dev, iproc_map_addr(io.dev, io.d0), &io.d1);
        mutex_unlock(&iproc_lock);
        break;
    case LUBDE_IPROC_WRITE_REG:
        mutex_lock(&iproc_lock);
        io.rc = bdbWrite32(io.dev, iproc_map_addr(io.dev, io.d0), io.d1);
        mutex_unlock(&iproc_lock);
        break;
    case LUBDE_GET_DEVICE_STATE:
        io.d0 = BDE_DEV_STATE_NORMAL;
        break;
    case LUBDE_NOKIA_OP_BDB_INIT:
        // this is the global BDB init for devmgr etc
        printk(KINFO "BDB init @ %llx sz %x\n", io.p0, io.d0);
        _cpuctl_base_addr = ioremap_nocache(io.p0, io.d0);
        break;
    case LUBDE_NOKIA_OP_BDB_READ:
        io.rc = bdbReadWord(io.dev, io.d0, io.d1, io.dx.buf);
        if (nokia_debug && msgCount)
        {
            printk(KINFO "BDB read slot %d addr %x size %d = %x (%d)\n", io.dev, io.d0, io.d1, io.dx.dw[0], io.rc);
            msgCount--;
        }
        break;
    case LUBDE_NOKIA_OP_BDB_WRITE:
        if (nokia_debug && msgCount)
        {
            printk(KINFO "BDB write slot %d addr %x size %d : %x\n", io.dev, io.d0, io.d1, io.dx.dw[0]);
            msgCount--;
        }
        io.rc = bdbWriteWord(io.dev, io.d0, io.d1, io.dx.buf);
        break;
    case LUBDE_NOKIA_OP_ADD_UNIT:
        // do we ever disable a slot using this API? TBD.
        if (!VALID_DEVICE(io.dev))
            return -EINVAL;
        nokia_dev[io.dev].is_valid = 1;         // can't be changed
        nokia_dev[io.dev].sfm_num = io.d0;
        nokia_dev[io.dev].unit = io.d1;
        nokia_dev[io.dev].device_id = io.d2;
        nokia_dev[io.dev].device_rev = io.d3;
        nokia_dev[io.dev].hw_main_baseaddr = io.dx.dw[0];
        nokia_dev[io.dev].hw_iproc_baseaddr = io.dx.dw[1];
        nokia_dev[io.dev].hw_slot = io.dx.dw[2] ? io.dx.dw[2] : (  nokia_dev[io.dev].sfm_num ? DEFAULT_RAMON_BASE_HW_SLOT+SFM_NUM_TO_SFM_INDEX(nokia_dev[io.dev].sfm_num) : 0);
        printk(KINFO "Create Nokia dev %d rev=%x sfmnum=%d hwslot=%d base1=%x base2=%x\n", io.dev, nokia_dev[io.dev].device_rev, nokia_dev[io.dev].sfm_num, nokia_dev[io.dev].hw_slot, nokia_dev[io.dev].hw_main_baseaddr, nokia_dev[io.dev].hw_iproc_baseaddr);
        //if (_dma_resource_alloc(1, &nokia_dev[io.dev].dma_offset) < 0)
        //    return -EFAULT;
        //printk(KINFO "nokia dev %d allocated dma @ offset %d\n", io.dev, nokia_dev[io.dev].dma_offset);
        break;
    default:
        io.rc = LUBDE_FAIL;
        break;
    }

    if (copy_to_user((void *)arg, &io, sizeof(io)))
        return -EFAULT;

    // printk(KINFO "Nokia ioctl ret %x: %d %x %x %x\n", cmd, io.rc, io.d0, io.d1, io.d2);
    return rc;
}





