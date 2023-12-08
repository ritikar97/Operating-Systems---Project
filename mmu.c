#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define NUM_FRAMES 3
#define PHYSICAL_MEMORY_SIZE (NUM_FRAMES * PAGE_SIZE)
#define PROC_PAGES (20)
#define DEBUG

#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

enum algorithm
{
    FIFO = 0,
    LRU,
    OPT
};

int firstIdx = 0;

enum algorithm pra = OPT;

int page_faults = 0;

typedef struct
{
    int valid;
    int frame_number;
    int last_accessed; // For LRU algorithm
    int future_access; // For OPT algorithm
} PageTableEntry;

typedef struct
{
    PageTableEntry entries[PROC_PAGES];
} PageTable;

typedef struct
{
    int pageNumber;
    int valid;
    uint8_t data[PAGE_SIZE];
} Page;

typedef struct
{
    Page frame[NUM_FRAMES];
} PhysicalMemory;

typedef struct
{
    PageTable page_table;
    PhysicalMemory physical_memory;
} MMU;

void initializeMMU(MMU *mmu)
{
    // Initialize MMU and associated data structures
    for (int i = 0; i < PROC_PAGES; i++)
    {
        mmu->page_table.entries[i].valid = -1;
        mmu->page_table.entries[i].last_accessed = 0; // Initialize last accessed time for LRU
        mmu->page_table.entries[i].future_access = -1; // Initialize future access time for OPT
    }

    for (int i = 0; i < NUM_FRAMES; i++)
    {
        mmu->physical_memory.frame[i].pageNumber = -1;
        mmu->physical_memory.frame[i].valid = -1;
    }
}

int findFreeFrame(MMU *mmu)
{
    // Find the first free frame in physical memory
    for (int i = 0; i < NUM_FRAMES; i++)
    {
        if (mmu->physical_memory.frame[i].valid == -1)
        {
            return i;
        }
    }
    return -1; // No free frames available
}

void fifoPageReplacement(MMU *mmu, int pageNum)
{
    // Handle page fault and add the page to physical memory
    int freeFrame = findFreeFrame(mmu);
    if (freeFrame != -1)
    {
        LOG("Found a free frame for page number = %d, free frame = %d\n", pageNum, freeFrame);
        mmu->page_table.entries[pageNum].valid = 1;
        mmu->page_table.entries[pageNum].frame_number = freeFrame;
        mmu->physical_memory.frame[freeFrame].pageNumber = pageNum;
        mmu->physical_memory.frame[freeFrame].valid = 1;
    }
    else
    {
        int pageNumDel = mmu->physical_memory.frame[firstIdx].pageNumber;
        LOG("Did not find a free frame for page number = %d, so replacing page = %d\n", pageNum, pageNumDel);
        mmu->page_table.entries[pageNumDel].valid = -1;
        mmu->page_table.entries[pageNum].frame_number = firstIdx;
        mmu->page_table.entries[pageNum].valid = 1;
        mmu->physical_memory.frame[firstIdx].pageNumber = pageNum;
        firstIdx = (firstIdx + 1) % NUM_FRAMES;
    }
}

void lruPageReplacement(MMU *mmu, int pageNum)
{
    // Handle page fault and add the page to physical memory
    int freeFrame = findFreeFrame(mmu);
    if (freeFrame != -1)
    {
        LOG("Found a free frame for page number = %d, free frame = %d\n", pageNum, freeFrame);
        mmu->page_table.entries[pageNum].valid = 1;
        mmu->page_table.entries[pageNum].frame_number = freeFrame;
        mmu->physical_memory.frame[freeFrame].pageNumber = pageNum;
        mmu->physical_memory.frame[freeFrame].valid = 1;
    }
    else
    {
        // Find the least recently used frame
        int lruFrame = 0;

        for (int i = 1; i < NUM_FRAMES; i++)
        {
            if (mmu->page_table.entries[mmu->physical_memory.frame[i].pageNumber].last_accessed <
                mmu->page_table.entries[mmu->physical_memory.frame[lruFrame].pageNumber].last_accessed)
            {
                lruFrame = i;
            }
        }

        int pageNumDel = mmu->physical_memory.frame[lruFrame].pageNumber;
        LOG("Did not find a free frame for page number = %d, so replacing page = %d\n", pageNum, pageNumDel);
        mmu->page_table.entries[pageNumDel].valid = -1;
        mmu->page_table.entries[pageNum].frame_number = lruFrame;
        mmu->page_table.entries[pageNum].valid = 1;
        mmu->physical_memory.frame[lruFrame].pageNumber = pageNum;
        mmu->physical_memory.frame[lruFrame].valid = 1;
    }
}

void optPageReplacement(MMU *mmu, int pageNum, int refString[], int currentIdx)
{
    // Handle page fault and add the page to physical memory
    int freeFrame = findFreeFrame(mmu);
    if (freeFrame != -1)
    {
        LOG("Found a free frame for page number = %d, free frame = %d\n", pageNum, freeFrame);
        mmu->page_table.entries[pageNum].valid = 1;
        mmu->page_table.entries[pageNum].frame_number = freeFrame;
        mmu->physical_memory.frame[freeFrame].pageNumber = pageNum;
        mmu->physical_memory.frame[freeFrame].valid = 1;
    }
    else
    {
        // Find the page with the maximum future access distance
        int maxFutureAccess = -1;
        int pageToReplace = -1;

        for (int i = 0; i < NUM_FRAMES; i++)
        {
            int futureAccess = -1;

            for (int j = currentIdx + 1; j < PROC_PAGES; j++)
            {
                if (refString[j] == mmu->physical_memory.frame[i].pageNumber)
                {
                    futureAccess = j;
                    break;
                }
            }

            if (futureAccess == -1)
            {
                futureAccess = PROC_PAGES;
            }

            if (futureAccess > maxFutureAccess)
            {
                maxFutureAccess = futureAccess;
                pageToReplace = i;
            }
        }

        int pageNumDel = mmu->physical_memory.frame[pageToReplace].pageNumber;
        LOG("Did not find a free frame for page number = %d, so replacing page = %d\n", pageNum, pageNumDel);
        mmu->page_table.entries[pageNumDel].valid = -1;
        mmu->page_table.entries[pageNum].frame_number = pageToReplace;
        mmu->page_table.entries[pageNum].valid = 1;
        mmu->physical_memory.frame[pageToReplace].pageNumber = pageNum;
        mmu->physical_memory.frame[pageToReplace].valid = 1;
    }
}


void addPage(MMU *mmu, int pageNum, int refString[], int currentIdx)
{
    if (pra == FIFO)
    {
        fifoPageReplacement(mmu, pageNum);
    }
    else if (pra == LRU)
    {
        lruPageReplacement(mmu, pageNum);
    }
    else if (pra == OPT)
    {
        optPageReplacement(mmu, pageNum, refString, currentIdx);
    }
}

int main()
{
    MMU mmu;
    int numPageFaults = 0;
    int numPageHits = 0;
    initializeMMU(&mmu);
    int refString[PROC_PAGES]= {7, 0, 1, 2, 0, 3, 0, 4, 2, 3, 0, 3, 2, 1, 2, 0, 1, 7, 0, 1};

    // for (int i = 0; i < PROC_PAGES; i++)
    // {
    //     refString[i] = rand() % PROC_PAGES;
    // }

    LOG("Reference string is:\n");

    for (int i = 0; i < PROC_PAGES; i++)
    {
        LOG("%d\t", refString[i]);
    }

    LOG("\n");

    for (int i = 0; i < PROC_PAGES; i++)
    {
        int pageNum = refString[i];
        mmu.page_table.entries[pageNum].last_accessed = i;       // Update last accessed time for LRU
        mmu.page_table.entries[pageNum].future_access = -1;      // Reset future access time for OPT
        for (int j = i + 1; j < PROC_PAGES; j++)
        {
            if (refString[j] == pageNum)
            {
                mmu.page_table.entries[pageNum].future_access = j; // Set future access time for OPT
                break;
            }
        }

        if (mmu.page_table.entries[pageNum].valid == -1)
        {
            LOG("Page fault for %d\n", pageNum);
            numPageFaults++;
            addPage(&mmu, pageNum, refString, i);
        }
        else
        {
            LOG("YAY: Page hit for %d\n", pageNum);
            numPageHits++;
        }
    }

    LOG("Ultimately, what's there is:\n");

    for (int i = 0; i < NUM_FRAMES; i++)
    {
        LOG("%d\t", mmu.physical_memory.frame[i].pageNumber);
    }

    LOG("\n");

    printf("Page Faults: %d\n", numPageFaults);
    printf("Page Hits: %d\n", numPageHits);

    return 0;
}
