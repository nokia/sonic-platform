#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/io.h>

MODULE_AUTHOR("Nokia Corporation");
MODULE_DESCRIPTION("BDE-BDB Helper Module");
MODULE_LICENSE("GPL");

#define KERNEL_MOD_NAME "nokia-kernel-bdb"
#define USER_MOD_NAME   "nokia-user-bdb"
#define KERNEL_MAJOR    119

#define KINFO KERN_INFO KERNEL_MOD_NAME ": "
#define KWARN KERN_WARNING KERNEL_MOD_NAME ": "

#define atomicInc(ptr)        __atomic_fetch_add((uint32_t *)(ptr), (uint32_t)1, __ATOMIC_SEQ_CST)
#define atomicDec(ptr)        __atomic_fetch_sub((uint32_t *)(ptr), (uint32_t)1, __ATOMIC_SEQ_CST)

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

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,6,0)
struct file_operations _proc_fops = {
    owner:      THIS_MODULE,
    open:       _proc_open,
    read:       seq_read,
    llseek:     seq_lseek,
    write:      _proc_write,
    release:    _proc_release,
};
#else
struct proc_ops _proc_fops = {
    proc_open:       _proc_open,
    proc_read:       seq_read,
    proc_lseek:     seq_lseek,
    proc_write:      _proc_write,
    proc_release:    _proc_release,
};
#endif

static long _ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
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

struct file_operations _fops = 
{
    owner:      THIS_MODULE,
    unlocked_ioctl: _ioctl,
    open:           _open,
    release:        _release,
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

    return 0;
}

static void __exit nokia_bdb_exit(void)
{
    remove_proc_entry(KERNEL_MOD_NAME, NULL);
    unregister_chrdev(KERNEL_MAJOR, KERNEL_MOD_NAME);

    printk(KERN_INFO "exit\n");
}

module_init(nokia_bdb_init);
module_exit(nokia_bdb_exit);

typedef struct  {
    unsigned int dev;
    unsigned int rc;
    unsigned int d0;
    unsigned int d1;
    unsigned int d2;
    unsigned int d3;    
    uint64_t p0;
    union {
        unsigned int dw[2];
        unsigned char buf[64];
    } dx;
} lubde_ioctl_t;


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

#define BDE_SWITCH_DEV_TYPE       0x00100
#define BDE_PCI_DEV_TYPE          0x00001
#define BDE_DEV_BUS_ALT           0x04000
#define BDE_USER_DEV_TYPE         0x40000

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long       uint64;

static int nokia_debug;
module_param(nokia_debug, int, 0);
MODULE_PARM_DESC(nokia_debug,"Set debug level (default 0)");


#include <linux/mutex.h>

#define NOKIA_DEV_NAME                      "nokia-bdb"

#define VALID_DEVICE(_n)                    (_n < MAX_NOKIA_RAMONS)
#define IS_NOKIA_DEV(d)                     (VALID_DEVICE(d) && nokia_dev[d].is_valid)
#define MAX_NOKIA_RAMONS                    18
#define MAX_SFMS                            8
#define MAX_HWSLOT                          31
#define POSTED_READ                         1
#define DEV_TO_RAMON_HWSLOT(d)              (nokia_dev[d].hw_slot)
#define HW_SLOT_TO_SFM_NUM(s)               ((s)-DEFAULT_RAMON_BASE_HW_SLOT+1)   
#define DEFAULT_RAMON_BASE_HW_SLOT          17
#define SFM_NUM_TO_SFM_INDEX(_s)            ((_s)-1)
#define IOCTL_BASE                          _cpuctl_base_addr
#define A32_CPUCTL_BASE                     0x4000000000
#define CPUCTL_SIZE                         (384*1024*1024)
#define BDB_MIN_FIFO_DEPTH                  40
#define BDB_SLOT_LOCK(s)                    ({ if (bdb_parallel) mutex_lock(&bdb_slot_lock[s]);})
#define BDB_SLOT_UNLOCK(s)                  ({ if (bdb_parallel) mutex_unlock(&bdb_slot_lock[s]);})
#define BDB_TIMEOUT                         (25*1000ULL*1000ULL)

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

#define A32_SFM_FE_DEFAULT_BAR0             (GIG_2 + MEG_32)
#define BCM_FE9600_PCI_VENDOR_ID            (0x14e4)
#define BCM_FE9600_PCI_DEVICE_ID            (0x8790)

#define RAMON_BAR0(_u)                      (A32_SFM_FE_DEFAULT_BAR0 + (_u) * MEG_32)

#define RAMON_MAIN_BASE(_u)                 (RAMON_BAR0(_u) + MEG_16)7
#define RAMON_IPROC_BASE(_u)                (RAMON_BAR0(_u))

#define read32(va)                          (*(volatile uint32 *) (va))
#define read32_be(va)                       SWAP32((*(volatile uint32 *) (va)))
#define write32(va,d)                       *(volatile uint32 *) (va) = (d)
#define write32_be(va,d)                    ({ uint32_t _x = (d); *(volatile uint32 *) (va) = SWAP32(_x);})
#define BDB_WAIT_US                         1000
#define HW_BDB_CARD_PRESENT(_s)             !!(bdbSignalReg() & (1<<(_s)))


#define M_BDB_SIGNAL_WFIFO_DEPTH    0xFF000000
#define S_BDB_SIGNAL_WFIFO_DEPTH    24

#define B_GEN_CONFIG_BDB_ENABLE     0x00000001
#define B_GEN_CONFIG_P_READ_DONE    0x00000002
#define B_GEN_CONFIG_P_READ_ERR     0x00000004
#define M_GEN_CONFIG_BDB_SLOT       0x000000F8
#define M_GEN_CONFIG_BDB_3127       0x00001F00
#define B_GEN_CONFIG_P_READ         0x00002000
#define M_GEN_CONFIG_RTCCF_HOLD     0x0000C000
#define M_GEN_CONFIG_RTCCF_ACTIVE   0x001F0000
#define M_GEN_CONFIG_RTCCF_SETUP    0x00E00000
#define M_GEN_CONFIG_VERSION        0xFF000000
#define M_GEN_CONFIG_BDB_RESP_SLOT  0x1F000000

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

#define A64_XRS_SCRATCHPAD          0x00800500

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
    uint32      last_subwin_base;
    struct mutex iproc_lock;
} nokia_dev[MAX_NOKIA_RAMONS];

static void * _cpuctl_base_addr;
static int msgCount = 100;
static bool bdb_parallel = false;
static volatile uint32 parallel_ops;
static uint32 max_parallel;

static uint32 bde_read, bde_write, nok_read, nok_write, iproc_read_reg, iproc_write_reg, iproc_cache_hit, bdb_spurious_ack;
static uint32 bdb_read_fail, bdb_write_fail;
static uint32 bdb_read_flushes, bdb_write_flushes, bdb_sac_write_fail, bdb_fifo_depth_wait;
static uint32 bdb_write_retries, bdb_write_retry_failures, bdb_read_retries, bdb_read_retry_failures;
static uint32 max_retries = 3, max_wait_time;


static DEFINE_MUTEX(bdb_lock);
static struct mutex bdb_slot_lock[MAX_HWSLOT+1];

static void nokia_dump(struct seq_file *m)
{
    int idx;

    seq_printf(m, "Nokia-bdb v3 units (bdb base %p, use_count %d parallel %d (max %d) debug %d):\n", _cpuctl_base_addr, use_count, bdb_parallel, max_parallel, nokia_debug);
    seq_printf(m, " bde_read:    %10u  bde_write:   %10u\n", bde_read, bde_write);
    seq_printf(m, " nok_read:    %10u  nok_write:   %10u\n", nok_read, nok_write);
    seq_printf(m, " iproc_read:  %10u  iproc_write: %10u  cache_hit: %u\n", iproc_read_reg, iproc_write_reg, iproc_cache_hit);
    seq_printf(m, " fifo_wait:  %6u  ack flush:   %6u  sac_write:  %6u  max_wait:   %u us\n", bdb_fifo_depth_wait, bdb_spurious_ack, bdb_sac_write_fail, max_wait_time/1000);
    seq_printf(m, " read_fail:  %6u  read_flush:  %6u  read_retry: %4u  retry_fail: %u\n", bdb_read_fail,  bdb_read_flushes,  bdb_read_retries,  bdb_read_retry_failures);
    seq_printf(m, " write_fail: %6u  write_flush: %6u  write_retry:%4u  retry_fail: %u\n", bdb_write_fail, bdb_write_flushes, bdb_write_retries, bdb_write_retry_failures);

    for (idx = 0; idx < MAX_NOKIA_RAMONS; idx++) 
    {
        if (IS_NOKIA_DEV(idx))
            seq_printf(m, "\t%d (swi) : PCI device %s:%d:%d on Nokia SFM module hwslot %d\n", idx, NOKIA_DEV_NAME, nokia_dev[idx].sfm_num, nokia_dev[idx].unit, nokia_dev[idx].hw_slot);
    }

    msgCount = 100;
}

uint32 bdbSignalReg(void)
{
    void * bdb_regs   = IOCTL_BASE + IOCTL_BDB_REGS_OFFSET;
    return(read32_be(bdb_regs + BDB_SIGNAL_REG_OFF));
}

void bdbFlushRead(int hwSlot, bool read)
{
    void * bdb_regs = IOCTL_BASE + IOCTL_BDB_REGS_OFFSET;
    uint32 val, bdbSlot;

    val = read32_be(bdb_regs + BDB_CTRL_REG_OFF);
    bdbSlot = bdb_parallel ? (val & M_GEN_CONFIG_BDB_RESP_SLOT) >> S_GEN_CONFIG_BDB_RESP_SLOT : hwSlot;
    if ((val & B_GEN_CONFIG_P_READ_DONE) && (hwSlot == bdbSlot))
    {
        if (bdb_parallel)
        {
            printk(KWARN "Clearing spurious ACK from slot %d for %s", hwSlot, read ? "read":"write");
            bdb_spurious_ack++;
        }
        read32(bdb_regs + BDB_POSTED_READ_REG_OFF);
    }
}

uint32 bdbFifoDepth(uint32 hwSlot)
{
    if (bdb_parallel)
    {
        void * bdb_regs = IOCTL_BASE + IOCTL_BDB_REGS_OFFSET;
        uint32 val = read32_be(bdb_regs + BDB_CTRL_REG_OFF);
        val = (val & ~M_GEN_CONFIG_BDB_SLOT) | (hwSlot << S_GEN_CONFIG_BDB_SLOT);
        write32_be(bdb_regs + BDB_CTRL_REG_OFF, val);
    }

    return(bdbSignalReg() >> S_BDB_SIGNAL_WFIFO_DEPTH);
}

#define M_GEN_CONFIG_BDB_RESP_SLOT  0x1F000000
#define B_GEN_CONFIG_RESP_WRACK     0x20000000
#define B_GEN_CONFIG_RESP_ERROR     0x40000000

static int bdbWaitForResult(int hwSlot, int * flushes)
    {
    void * bdb_regs = IOCTL_BASE + IOCTL_BDB_REGS_OFFSET;
    uint32 ctrl, bdbSlot;
    uint64 nsecs = ktime_get_raw_ns();
    uint64 now = nsecs;
    uint64 timeout = BDB_TIMEOUT;
    bool flushed = false;

    while (true)                                                   
    {   
        uint64 old_now = now;                                                        
        now = ktime_get_raw_ns();

        ctrl = read32_be(bdb_regs + BDB_CTRL_REG_OFF);
        bdbSlot = bdb_parallel ? ((ctrl & M_GEN_CONFIG_BDB_RESP_SLOT) >> S_GEN_CONFIG_BDB_RESP_SLOT) : hwSlot;

        if ((ctrl & B_GEN_CONFIG_P_READ_DONE) && (hwSlot == bdbSlot))
        {
            if (!flushed && max_wait_time < (old_now-nsecs))
                max_wait_time = (old_now-nsecs);

            return (ctrl & (B_GEN_CONFIG_RESP_ERROR|B_GEN_CONFIG_P_READ_ERR)) ? LUBDE_FAIL : LUBDE_SUCCESS;
        }

                if ((now-nsecs) < timeout)
        {
            ndelay(32*10);
            continue;
        }

        if (!(ctrl & B_GEN_CONFIG_P_READ_DONE) || !bdb_parallel)
            break;

        (*flushes)++;
        flushed = true;

        read32(bdb_regs + BDB_POSTED_READ_REG_OFF);

        timeout = 2*BDB_TIMEOUT;
    }

    return LUBDE_FAIL;
}

int bdbReadWordRaw(uint32 hwSlot, uint32 addr, int wsize, void * ret)
{
    void * bdb_regs   = IOCTL_BASE + IOCTL_BDB_REGS_OFFSET;
    void * bdb_window = IOCTL_BASE + IOCTL_BDB_WINDOW_OFFSET;
    void * ptr;
    uint32 val;
    int rc = LUBDE_SUCCESS;
    int flushes = 0;

    if (hwSlot > MAX_HWSLOT || !HW_BDB_CARD_PRESENT(hwSlot) || wsize > 8)
        return LUBDE_FAIL;

    BDB_SLOT_LOCK(hwSlot);

    mutex_lock(&bdb_lock);

    bdbFlushRead(hwSlot, true);

    val = BDB_BITS_DEFAULT | B_GEN_CONFIG_P_READ | (hwSlot << S_GEN_CONFIG_BDB_SLOT) | ((addr >> 27) << S_GEN_CONFIG_BDB_3127);
    write32_be(bdb_regs + BDB_CTRL_REG_OFF, val);

    ptr = bdb_window + (addr & ((1<<27)-1));

    if (wsize == 1)        *(uint8_t  *)ret = *(volatile uint8_t  *)ptr;
    else if (wsize == 2)   *(uint16_t *)ret = *(volatile uint16_t *)ptr;
    else if (wsize == 4)   *(uint32_t *)ret = *(volatile uint32_t *)ptr;
    else                   *(uint64_t *)ret = *(volatile uint64_t *)ptr;

    if (bdb_parallel)
        mutex_unlock(&bdb_lock);

    atomicInc(&parallel_ops);

    rc = bdbWaitForResult(hwSlot, &flushes);
    if (rc == LUBDE_SUCCESS)
    {
        ptr = bdb_regs + BDB_POSTED_READ_REG_OFF + (addr & 3);
        if (wsize == 1)        *(uint8_t *)ret  = *(volatile uint8_t *)ptr;
        else if (wsize == 2)   *(uint16_t *)ret = *(volatile uint16_t *)ptr;
        else if (wsize == 4)   *(uint32_t *)ret = *(volatile uint32_t *)ptr;
        else                   *(uint64_t *)ret = *(volatile uint64_t *)ptr;
    }

    if (!bdb_parallel)
        mutex_unlock(&bdb_lock);

    if (parallel_ops > max_parallel)
        max_parallel = parallel_ops;

    atomicDec(&parallel_ops);

    bdb_read_flushes += flushes;
    if (rc == LUBDE_FAIL)
    {
        if (bdb_read_fail < 20)
        {
            printk(KWARN "Slot %d BDB read timeout from %x (stat=%x) sig=%x flushes=%d\n", hwSlot, addr, val, bdbSignalReg(), flushes);
        }
        bdb_read_fail++;
    }

    BDB_SLOT_UNLOCK(hwSlot);

    return rc;
} 

int bdbReadWord(uint32 hwSlot, uint32 addr, int wsize, void * ret)
{
    int rc, retries = max_retries;

    while (retries)
    {
        rc = bdbReadWordRaw(hwSlot, addr, wsize, ret);
        if (rc == 0)
        {
            if (retries != max_retries)
                printk(KWARN "Slot %d BDB read#%d retry SUCCESS addr %x data = %x\n", hwSlot, max_retries-retries+1, addr, *(uint32_t *)ret);
            return rc;
        }
        retries--;
        bdb_read_retries++;
    }
    bdb_read_retry_failures++;
    return rc;
}

int bdbRead32(int d, uint32_t addr, uint32_t *ret)
{
    return (bdbReadWord(DEV_TO_RAMON_HWSLOT(d), addr, 4, ret));
}

int bdbWriteWordRaw(uint32 hwSlot, uint32 addr, int wsize, void * data)
{
    void * bdb_regs   = IOCTL_BASE + IOCTL_BDB_REGS_OFFSET;
    void * bdb_window = IOCTL_BASE + IOCTL_BDB_WINDOW_OFFSET;
    void * ptr;
    uint32 val;
    int rc = LUBDE_SUCCESS;

    if (hwSlot > MAX_HWSLOT || !HW_BDB_CARD_PRESENT(hwSlot) || wsize > 8)
        return LUBDE_FAIL;


    BDB_SLOT_LOCK(hwSlot);

    mutex_lock(&bdb_lock);

    bdbFlushRead(hwSlot, false);

    while (bdbFifoDepth(hwSlot) >= (BDB_MIN_FIFO_DEPTH+8-wsize))
        if (bdb_parallel)
        {
            bdb_fifo_depth_wait++;

            mutex_unlock(&bdb_lock);
            ndelay(32*10);
            mutex_lock(&bdb_lock);
        }

    val = BDB_BITS_DEFAULT | B_GEN_CONFIG_P_READ | (hwSlot << S_GEN_CONFIG_BDB_SLOT) | ((addr >> 27) << S_GEN_CONFIG_BDB_3127);
    write32_be(bdb_regs + BDB_CTRL_REG_OFF, val);

    ptr = bdb_window + (addr & ((1<<27)-1));
    if (wsize == 1)         *(volatile uint8_t *)ptr  = *(uint8_t *)data;
    else if (wsize == 2)    *(volatile uint16_t *)ptr = *(uint16_t *)data;
    else if (wsize == 4)    *(volatile uint32_t *)ptr = *(uint32_t *)data;
    else                    *(volatile uint64_t *)ptr = *(uint64_t *)data;
    mutex_unlock(&bdb_lock);
    atomicInc(&parallel_ops);

    if (bdb_parallel)
    {
        int flushes = 0;
        rc = bdbWaitForResult(hwSlot, &flushes);
        read32(bdb_regs + BDB_POSTED_READ_REG_OFF);
        bdb_write_flushes += flushes;

        if (addr == A64_XRS_SCRATCHPAD && rc == LUBDE_FAIL)
        {
            bdb_sac_write_fail++;
            rc = LUBDE_SUCCESS;
        }

        if (rc == LUBDE_FAIL)
        {
            if (bdb_write_fail < 20)
            {
                printk(KWARN "Slot %d BDB write ack timeout from %x (stat=%x) sig=%x flushes=%d\n", hwSlot, addr, val, bdbSignalReg(), flushes);
            }
            bdb_write_fail++;
        }
    }

    if (parallel_ops > max_parallel)
        max_parallel = parallel_ops;

    atomicDec(&parallel_ops);

    BDB_SLOT_UNLOCK(hwSlot);

    return rc;
} 

int bdbWriteWord(uint32 hwSlot, uint32 addr, int wsize, void * data)
{
    int rc, retries = max_retries;

    while (retries)
    {
        rc = bdbWriteWordRaw(hwSlot, addr, wsize, data);
        if (rc == 0)
            return rc;
        retries--;
        bdb_write_retries++;
    }
    bdb_write_retry_failures++;
    return rc;
}


int bdbWrite32(int d, uint32 addr, uint32 data)
{
    return(bdbWriteWord(DEV_TO_RAMON_HWSLOT(d), addr, 4, &data));
}

#define BAR0_PAXB_IMAP0_7                       (0x2c1c)

static uint32 iproc_map_addr(int d, unsigned int addr)
{
    uint32 data, subwin_base =(addr & ~0xfff);
    uint32 iprocBase = nokia_dev[d].hw_iproc_baseaddr;

    if ((subwin_base == 0x10231000) || (subwin_base == 0x18013000)) 
    {
        addr = 0x6000 + (addr & 0xfff);
    }
    else 
    {
        if (nokia_dev[d].last_subwin_base != subwin_base)
        {
            bdbWrite32(d, iprocBase + BAR0_PAXB_IMAP0_7, subwin_base | 1);
            bdbRead32(d, iprocBase + BAR0_PAXB_IMAP0_7, &data);
            nokia_dev[d].last_subwin_base = subwin_base;
        }
        else
            iproc_cache_hit++;

        addr = 0x7000 + (addr & 0xfff);
    }

    return(iprocBase + addr);
}

static int nokia_ioctl(unsigned int cmd, unsigned long arg)
{
    lubde_ioctl_t io;
    int i, rc = 0;

    if (copy_from_user(&io, (void *)arg, sizeof(io))) 
        return -EFAULT;
  
    io.rc = LUBDE_SUCCESS;

    switch (cmd)
    {
    case LUBDE_VERSION:
        io.d0 = KBDE_VERSION;
        break;
    case LUBDE_GET_NUM_DEVICES:
        io.d0 = MAX_NOKIA_RAMONS;
        break;
    case LUBDE_ATTACH_INSTANCE:
        break;
    case LUBDE_GET_DEVICE:
        io.d0 = nokia_dev[io.dev].device_id;
        io.d1 = nokia_dev[io.dev].device_rev;
        io.dx.dw[0] = io.dev;
        io.d2 = 0;
        io.d3 = 0;
        break;
    case LUBDE_GET_DEVICE_TYPE:
        io.d0 = BDE_USER_DEV_TYPE | BDE_SWITCH_DEV_TYPE; 
        break;
    case LUBDE_GET_BUS_FEATURES:
        io.d0 = io.d1 = io.d2 = 0;
        break;
    case LUBDE_GET_DMA_INFO:
        io.d0 = io.d1 = 0;
        break;
    case LUBDE_READ_REG_16BIT_BUS:
        bde_read++;
        io.rc = bdbRead32(io.dev, nokia_dev[io.dev].hw_main_baseaddr + io.d0, &io.d1);
        break;
    case LUBDE_WRITE_REG_16BIT_BUS:
        bde_write++;
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
    case LUBDE_IPROC_WRITE_REG:
        if (!IS_NOKIA_DEV(io.dev))
            return -EINVAL;

        mutex_lock(&nokia_dev[io.dev].iproc_lock);

        if (cmd == LUBDE_IPROC_READ_REG)
            io.rc = bdbReadWord(DEV_TO_RAMON_HWSLOT(io.dev), iproc_map_addr(io.dev, io.d0), 4, &io.d1);
        else
            io.rc = bdbWriteWord(DEV_TO_RAMON_HWSLOT(io.dev), iproc_map_addr(io.dev, io.d0), 4, &io.d1);

        (cmd == LUBDE_IPROC_READ_REG) ? iproc_read_reg++ : iproc_write_reg++;

        mutex_unlock(&nokia_dev[io.dev].iproc_lock);

        break;
    case LUBDE_GET_DEVICE_STATE:
        io.d0 = BDE_DEV_STATE_NORMAL;
        break;
    case LUBDE_REPROBE:
        io.rc = LUBDE_SUCCESS;
        break;
    case LUBDE_NOKIA_OP_BDB_INIT:
        printk(KINFO "BDB (new) init @ %llx sz %x parallel %x\n", io.p0, io.d0, io.d1);
        _cpuctl_base_addr = ioremap(io.p0, io.d0);
        bdb_parallel = !!io.d1;
        if (bdb_parallel)
            for (i=0; i<=MAX_HWSLOT; i++)
                mutex_init(&bdb_slot_lock[i]);
        break;
    case LUBDE_NOKIA_OP_BDB_READ:
        io.rc = bdbReadWord(io.dev, io.d0, io.d1, io.dx.buf);
        nok_read++;
        if (nokia_debug && msgCount)
        {
            printk(KINFO "BDB read slot %d addr %x size %d = %x (%d)\n", io.dev, io.d0, io.d1, io.dx.dw[0], io.rc);
            msgCount--;
        }
        break;
    case LUBDE_NOKIA_OP_BDB_WRITE:
        io.rc = bdbWriteWord(io.dev, io.d0, io.d1, io.dx.buf);
        nok_write++;
        if (nokia_debug && msgCount)
        {
            printk(KINFO "BDB write slot %d addr %x size %d : %x (%d)\n", io.dev, io.d0, io.d1, io.dx.dw[0], io.rc);
            msgCount--;
        }
        break;
    case LUBDE_NOKIA_OP_ADD_UNIT:
        if (!VALID_DEVICE(io.dev))
            return -EINVAL;
        if (io.d0)
        {
            nokia_dev[io.dev].is_valid = true;
            nokia_dev[io.dev].sfm_num = io.d0;
            nokia_dev[io.dev].unit = io.d1;
            nokia_dev[io.dev].device_id = io.d2;
            nokia_dev[io.dev].device_rev = io.d3;
            nokia_dev[io.dev].hw_main_baseaddr = io.dx.dw[0];
            nokia_dev[io.dev].hw_iproc_baseaddr = io.dx.dw[1];
            nokia_dev[io.dev].hw_slot = io.dx.dw[2];
            nokia_dev[io.dev].last_subwin_base = -1;
            mutex_init(&nokia_dev[io.dev].iproc_lock);
            printk(KINFO "Create Nokia dev %d rev=%x sfmnum=%d hwslot=%d base1=%x base2=%x\n", io.dev, nokia_dev[io.dev].device_rev, nokia_dev[io.dev].sfm_num, nokia_dev[io.dev].hw_slot, nokia_dev[io.dev].hw_main_baseaddr, nokia_dev[io.dev].hw_iproc_baseaddr);
        }
        else
        {
            nokia_dev[io.dev].is_valid = false;
            printk(KINFO "Disable Nokia dev %d rev=%x sfmnum=%d hwslot=%d base1=%x base2=%x\n", io.dev, nokia_dev[io.dev].device_rev, nokia_dev[io.dev].sfm_num, nokia_dev[io.dev].hw_slot, nokia_dev[io.dev].hw_main_baseaddr, nokia_dev[io.dev].hw_iproc_baseaddr);
        }
        break;
    default:
        io.rc = LUBDE_FAIL;
        break;
    }

    if (copy_to_user((void *)arg, &io, sizeof(io)))
        return -EFAULT;

    return rc;
}





