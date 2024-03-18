#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384  // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8  // Shift page number this much

#define PTP_OFFSET 64 // How far offset in page 0 is the page table pointer table

// Simulated RAM
unsigned char mem[MEM_SIZE];

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    memset(mem, 0, MEM_SIZE);

    int zpfree_addr = get_address(0, 0);
    mem[zpfree_addr] = 1;  // Mark zero page as allocated
}

//
// Get the page table page for a given process
//
unsigned char get_page_table(int proc_num)
{
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    return mem[ptp_addr];
}

//
// Sets the page table page for a given process to a given page
//
void set_page_table(int proc_num, int page)
{
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    mem[ptp_addr] = page;
}

//
// Allocates the next available page. Returns -1 if no free page is found.
//
int allocate_next_page()
{
    int page = -1;

    for (int i = 0; i < PTP_OFFSET; i++)
    {
        if (mem[i] == 0)
        {
            mem[i] = 1;
            page = i;

            break;
        }
    }

    return page;
}

//
// Allocate pages for a new process
//
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
    int page_table = allocate_next_page();

    if (page_table == -1)
    {
        printf("OOM: proc %d: page table\n", proc_num);
        return;
    }

    set_page_table(proc_num, page_table);

    for (int i = 0; i < page_count; i++)
    {
        int page = allocate_next_page();
        if (page == -1)
        {
            printf("OOM: proc %d: data page\n", proc_num);
            return;
        }

        int pte_addr = get_address(page_table, i);
        mem[pte_addr] = page;
    }
}

//
// Unallocates pages for an existing proc_num process
//
// This includes all data pages and the process's page table.
//
void kill_process(int proc_num)
{
    int page_table = get_page_table(proc_num);

    for (int i = 0; i < PAGE_SIZE; i++)
    {
        int pte_addr = get_address(page_table, i);
        int page = mem[pte_addr];
        if (page != 0)
        {
            mem[page] = 0;
        }
    }

    mem[page_table] = 0;
}

//
// Translates a given virtual address for a process into a physical address
//
int translate_virtual_address(int proc_num, int vaddr)
{
    int pte_index = vaddr >> 8;
    int offset = vaddr & 255;

    int page_table = get_page_table(proc_num);
    int pte_addr = get_address(page_table, pte_index);
    int page = mem[pte_addr];

    return get_address(page, offset);
}

//
// Stores a value at a given virtual address for a process
//
void store_value(int proc_num, int vaddr, unsigned char value)
{
    int addr = translate_virtual_address(proc_num, vaddr);

    mem[addr] = value;

    printf("Store proc %d: %d => %d, value=%d\n", proc_num, vaddr, addr, value);
}

//
// Loads a stored value from a given vitual address for a process
//
void load_value(int proc_num, int vaddr)
{
    int addr = translate_virtual_address(proc_num, vaddr);

    unsigned char value = mem[addr];

    printf("Load proc %d: %d => %d, value=%d\n", proc_num, vaddr, addr, value);
}

//
// Print the free page map
//
// Don't modify this
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++) {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0? '.': '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
// Don't modify this
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);

    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0) {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1) {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }
    
    initialize_mem();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "pfm") == 0) {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0) {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0) {
            int proc_num = atoi(argv[++i]);
            int page_count = atoi(argv[++i]);
            new_process(proc_num, page_count);
        }
        else if (strcmp(argv[i], "kp") == 0) {
            int proc_num = atoi(argv[++i]);
            kill_process(proc_num);
        }
        else if (strcmp(argv[i], "sb") == 0) {
            int proc_num = atoi(argv[++i]);
            int vaddr = atoi(argv[++i]);
            unsigned char value = atoi(argv[++i]);

            store_value(proc_num, vaddr, value);
        }
        else if (strcmp(argv[i], "lb") == 0) {
            int proc_num = atoi(argv[++i]);
            int vaddr = atoi(argv[++i]);

            load_value(proc_num, vaddr);
        }
    }
}
