#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/init.h>

MODULE_LICENSE("GPL");

static struct pci_device_id ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82801AA_3), },
	{ 0, }
};

MODULE_DEVICE_TABLE(pci, ids);

static int pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
	u8 revision;
	u8 inter_id;

	pci_enable_device(dev);
	pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &inter_id);
	pci_read_config_byte(dev, PCI_REVISION_ID, &revision);
	if (revision == 0x42)
		return -ENODEV;

	printk("interrupt_id is %u, revision is %u", inter_id, revision);

	return 0;
}

static void pci_remove(struct pci_dev *dev)
{
	return ;
}

static struct pci_driver pci_driver = {
	.name 		= 	"pci_skel",
	.id_table 	= 	ids,
	.probe 		= 	pci_probe,
	.remove 	= 	pci_remove,
};

static int __init pci_skel_init(void)
{
	return pci_register_driver(&pci_driver);
}

static void __exit pci_skel_exit(void)
{
	pci_unregister_driver(&pci_driver);
}

module_init(pci_skel_init);
module_exit(pci_skel_exit);
