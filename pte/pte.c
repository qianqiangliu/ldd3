#include <linux/module.h>
#include <linux/sched.h>
#include <linux/highmem.h>
#include <asm/pgtable.h>

static int __init pte_init(void)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	struct task_struct *tsk = next_task(&init_task);
	unsigned long vaddr = 0x08048000;
	unsigned long paddr;

	pgd = pgd_offset(tsk->mm, vaddr);
	if (pgd_none(*pgd) || pgd_bad(*pgd))
		return -1;

	pud = pud_offset(pgd, vaddr);
	if (pud_none(*pud) || pud_bad(*pud))
		return -1;

	pmd = pmd_offset(pud, vaddr);
	if (pmd_none(*pmd) || pmd_bad(*pmd))
		return -1;

	pte = pte_offset_map(pmd, vaddr);
	if (!pte_present(*pte))
		return -1;

	paddr = (pte_val(*pte) & PAGE_MASK) | (vaddr & PAGE_SHIFT);

	pte_unmap(pte);

	printk(KERN_DEBUG "vaddr = %lx, paddr = %lx\n", vaddr, paddr);
	return 0;
}

static void __exit pte_exit(void)
{
	printk(KERN_DEBUG "Goodbye, cruel world!\n");
}

MODULE_LICENSE("Dual BSD/GPL");
module_init(pte_init);
module_exit(pte_exit);
