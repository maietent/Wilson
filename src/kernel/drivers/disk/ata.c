#include "ata.h"
#include "ports.h"
#include "idt.h"
#include "klog.h"

extern void ata_stub();
static volatile bool ata_irq_received = false;

static void ata_wait_bsy()
{
    uint8_t status;
    int timeout = 1000000;
    while (timeout--)
    {
        status = inb(ATA_PRIMARY_BASE + 7);
        if (!(status & ATA_SR_BSY)) return;
        if (status == 0xFF) return;
    }
    klogf("ATA: BSY timeout (status: 0x%x)\n", status);
}

static void ata_wait_drq()
{
    uint8_t status;
    int timeout = 1000000;
    while (timeout--)
    {
        status = inb(ATA_PRIMARY_BASE + 7);
        if (status & ATA_SR_DRQ) return;
        if (status & 0x01) return;
        if (status == 0xFF) return;
    }
    klogf("ATA: DRQ timeout (status: 0x%x)\n", status);
}

static void ata_select_drive(uint8_t drive, uint32_t lba)
{
    outb(ATA_PRIMARY_BASE + 6, 0xE0 | ((drive & 1) << 4) | ((lba >> 24) & 0x0F));
}

bool ata_read_sector(uint32_t lba, uint8_t *buffer)
{
    ata_irq_received = false;
    ata_wait_bsy();
    ata_select_drive(0, lba);

    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);

    outb(ATA_PRIMARY_BASE + 2, 1);
    outb(ATA_PRIMARY_BASE + 3, lba & 0xFF);
    outb(ATA_PRIMARY_BASE + 4, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_BASE + 5, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_BASE + 7, ATA_CMD_READ);

    uint8_t status = inb(ATA_PRIMARY_BASE + 7);

    if (status == 0 || status == 0xFF)
    {
        klogf("ATA: No drive detected (status: 0x%x)\n", status);
        return false;
    }
    ata_wait_bsy();

    status = inb(ATA_PRIMARY_BASE + 7);

    if (status & 0x01)
    {
        uint8_t error = inb(ATA_PRIMARY_BASE + 1);
        klogf("ATA: Error 0x%x, Status 0x%x\n", error, status);
        return false;
    }

    if (!(status & ATA_SR_DRQ))
    {
        klogf("ATA: DRQ not set (status: 0x%x)\n", status);
        return false;
    }

    for (int i = 0; i < 256; i++)
    {
        ((uint16_t*)buffer)[i] = inw(ATA_PRIMARY_BASE);
    }

    return true;
}

bool ata_write_sector(uint32_t lba, const uint8_t *buffer)
{
    ata_irq_received = false;

    ata_wait_bsy();

    ata_select_drive(0, lba);

    outb(ATA_PRIMARY_BASE + 2, 1);
    outb(ATA_PRIMARY_BASE + 3, lba & 0xFF);
    outb(ATA_PRIMARY_BASE + 4, (lba >> 8) & 0xFF);
    outb(ATA_PRIMARY_BASE + 5, (lba >> 16) & 0xFF);
    outb(ATA_PRIMARY_BASE + 7, ATA_CMD_WRITE);

    ata_wait_bsy();
    ata_wait_drq();

    for (int i = 0; i < 256; i++)
    {
        outw(ATA_PRIMARY_BASE, ((uint16_t*)buffer)[i]);
    }

    outb(ATA_PRIMARY_CTRL, 0x08);
    while (!ata_irq_received);
    return true;
}

void ata_handler()
{
    ata_irq_received = true;
    outb(0x20, 0x20);
}

void ata_init()
{
    idt_set_entry(ATA_PRIMARY_IRQ + 32, (uint64_t)ata_stub, 0x08, 0x8E);
    outb(ATA_PRIMARY_CTRL, 0x02);
    ata_wait_bsy();
    outb(ATA_PRIMARY_BASE + 6, 0xA0);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    inb(ATA_PRIMARY_CTRL);
    uint8_t status = inb(ATA_PRIMARY_BASE + 7);
    if (status == 0xFF)
    {
        klogf("ATA: No drive detected\n");
        return;
    }
    outb(ATA_PRIMARY_CTRL, 0x00);
    klogf("ATA Initialized (status: 0x%x)\n", status);
}
