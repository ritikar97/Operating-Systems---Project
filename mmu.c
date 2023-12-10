#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>

#define PAGE_SIZE 4096
#define NUM_FRAMES 4
#define PHYSICAL_MEMORY_SIZE (NUM_FRAMES * PAGE_SIZE)
#define PROC_PAGES (20)
// #define DEBUG

#ifdef DEBUG
#define LOG(...) printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

enum algorithm
{
    FIFO = 0,
    LRU,
    OPT,
    HRPA
};

int firstIdx = 0;

enum algorithm pra = LRU;

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
    int futurePrediction;
    int accessFreq;
    int pastPrediction;
    int presentLater;
} pageData;

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


void hpraPageReplacement(MMU *mmu, int pageNum, int refString[], int currentIdx)
{
    int foundReplacement = 0;
    // Handle page fault and add the page to physical memory
    int freeFrame = findFreeFrame(mmu);
    if (freeFrame != -1)
    {
        LOG("Found a free frame for page number = %d, free frame = %d\n", pageNum, freeFrame);
        mmu->page_table.entries[pageNum].valid = 1;
        mmu->page_table.entries[pageNum].frame_number = freeFrame;
        mmu->physical_memory.frame[freeFrame].pageNumber = pageNum;
        mmu->physical_memory.frame[freeFrame].valid = 1;
        return;
    }

    LOG("No free frame\n");
    pageData pagesInFrame[NUM_FRAMES];
    int checkPageNum;
    int pageToReplace = 0;
    int minPageFrequency = INT_MIN;
    int leastDist = INT_MAX;

    for(int i = 0; i < NUM_FRAMES; i++)
    {
        LOG("For frame [%d]\n", i);
        pagesInFrame[i].futurePrediction = -1;
        pagesInFrame[i].accessFreq = 0;
        pagesInFrame[i].pastPrediction = -1;
        pagesInFrame[i].presentLater = 0;
        checkPageNum = mmu->physical_memory.frame[i].pageNumber; // Iterate through every page in the frame

        for(int j = currentIdx + 1; j < PROC_PAGES; j++)
        {
            if(refString[j] == checkPageNum)
            {
                pagesInFrame[i].futurePrediction = j;
                pagesInFrame[i].presentLater = 1;
                LOG("The page %d is referenced later\n", checkPageNum);
                break;
            }
        }

        // Counter (frequency of page access)
        for (int j = currentIdx + 1; j < PROC_PAGES; j++)
        {
            if (refString[j] == checkPageNum)
            {
                pagesInFrame[i].accessFreq++;
            }
        }

        LOG("The number of times the page is present after this is %d\n", pagesInFrame[i].accessFreq);

        // Past prediction
        for(int j = currentIdx - 1; j >= 0; j--)
        {
            if(refString[j] == checkPageNum)
            {
                pagesInFrame[i].pastPrediction = j;
                LOG("The page was present before at index %d\n", j);
                break;
            }
        }
    }


    minPageFrequency = pagesInFrame[0].accessFreq;
    LOG("The min page frequency to begin with is %d\n", minPageFrequency);

    pageToReplace = 0;
    LOG("Let's say the page to replace is %d\n", pageToReplace);

    // To figure out page to be replaced, look at all the pages
    // If the page is present later, then it shouldn't be replaced
    for(int i = 0; i < NUM_FRAMES; i++)
    {
        if(pagesInFrame[i].futurePrediction == -1)
        {
            pageToReplace = i;
            LOG("Page in frame %d won't be referenced later\n", pageToReplace);
            foundReplacement = 1;
            break;
        }

        // If you can't find a page for later, maintain counts of which page is accessed more
        if(pagesInFrame[i].accessFreq <= minPageFrequency)
        {
            minPageFrequency = pagesInFrame[i].accessFreq;
            pageToReplace = i;
            LOG("Looks like min page frequency is now %d for frame %d\n", minPageFrequency, pageToReplace);
            foundReplacement = 1;
        }

    }

    if(!foundReplacement)
    {
        for( int i = 0; i < NUM_FRAMES; i++)
        {
            if((pagesInFrame[i].pastPrediction != -1) && ((currentIdx - pagesInFrame[i].pastPrediction) < leastDist))
            {
                leastDist = currentIdx - pagesInFrame[i].pastPrediction;
                pageToReplace = i;
                LOG("Frame [%d]: Current Idx is %d and least distance is %d so page to replace is %d\n", i, currentIdx, leastDist, pageToReplace);
            }
        }
    }

    int pageDel = mmu->physical_memory.frame[pageToReplace].pageNumber;
    LOG("Did not find a free frame for page number = %d, so replacing page = %d\n", pageNum, pageToReplace);
    mmu->page_table.entries[pageDel].valid = -1;
    mmu->page_table.entries[pageNum].frame_number = pageToReplace;
    mmu->page_table.entries[pageNum].valid = 1;
    mmu->physical_memory.frame[pageToReplace].pageNumber = pageNum;
    mmu->physical_memory.frame[pageToReplace].valid = 1;
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
    else if (pra == HRPA)
    {
        hpraPageReplacement(mmu, pageNum, refString, currentIdx);
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

        printf("Current frame is %d %d %d %d\n", mmu.physical_memory.frame[0].pageNumber, 
            mmu.physical_memory.frame[1].pageNumber, mmu.physical_memory.frame[2].pageNumber, 
            mmu.physical_memory.frame[3].pageNumber);
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
