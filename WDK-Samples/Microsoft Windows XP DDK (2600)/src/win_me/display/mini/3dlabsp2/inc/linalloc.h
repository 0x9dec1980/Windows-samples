/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: linalloc.h
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/
#ifndef __LINALLOC_H_
#define __LINALLOC_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#define Invalid_Node 0
#define Allocated_Node 1
#define Free_Node 2
#define Dud_Node 3

#define NODES_PER_GROUP 100
#define START_NODE_GROUP_ID 0

// How to get from one node to another node, in this node group
// or another.
typedef struct tagNODEOFFSET
{
	DWORD dwNodeGroup;	// The 0 based group that this node is in
	DWORD dwIndex;		// The 0 based Index into the group of nodes
} NODEOFFSET, *LPNODEOFFSET;

typedef struct tagALLOCUSAGE
{
	DWORD dwFlags;
	DWORD dwPassedPointer;
	long lReferenceCount;
} ALLOCUSAGE, *LPALLOCUSAGE;

// How to walk the node list
typedef struct tagNODE
{
	DWORD dwFlag;				// What is this node being used for?

	NODEOFFSET LastNode;		// The next allocated chunk
	NODEOFFSET NextNode;		// The Next allocated chunk

	DWORD	   dwIndex;			// Index of this node
	DWORD	   dwNodeGroup;		// Node group this node is in

	DWORD dwStartAddress;		// Start Address of memory (if allocated or free)
	DWORD dwEndAddress;			// End Address of memory (if allocated or free)

	ALLOCUSAGE UsageFlags;		// What was the allocation request for this node?
} NODE, *LPNODE;


// A Node group - the actual allocated nodes.
// The node groups are simply a doubly linked list.
typedef struct tagNODEGROUP
{
	struct tagNODEGROUP* ptr16Next;		// 16 Bit pointers to next NODEGROUP
	struct tagNODEGROUP* ptr32Next;		// 32 Bit pointers to next NODEGROUP

    struct tagNODEGROUP* ptr16Prev;     // 16 Bit pointer to previous NODEGROUP
    struct tagNODEGROUP* ptr32Prev;		// 32 Bit pointer to previous NODEGROUP

	DWORD pThis16;						// 16 bit pointer to this object
	DWORD pThis32;						// 32 bit pointer to this object

	DWORD dwNodeGroup;
	DWORD dwInvalidNodes;	// Number of invalid nodes in this list.

    NODE node[NODES_PER_GROUP];	// The group of nodes

} NODEGROUP, *LPNODEGROUP;

typedef struct tagHEAPSTATS
{
	DWORD			dwFreeMem;
	DWORD			dwAllocMem;
	DWORD			dwFreeNodes;
	DWORD			dwAllocNodes;
	DWORD			dwInvalidNodes;
	DWORD			dwNodeGroups;
	DWORD			dwFragmentation;
	DWORD			dwMaxFrag;
} HEAPSTATS, *LPHEAPSTATS;

// Deletes a node group, will wipe out member nodes so they had better be allocated!
BOOL InitialiseHeapManager(struct tagThunkedData* pThisDisplay);
void UnInitialiseHeapManager(struct tagThunkedData* pThisDisplay);
BOOL AllocMem(DWORD* p16, DWORD* p32, DWORD dwSize);
void FreeMem(DWORD p16, DWORD p32);

// Node group functions
LPNODEGROUP AddNodeGroup(struct tagThunkedData* pThisDisplay);
void DeleteNodeGroup(struct tagThunkedData* pThisDisplay, LPNODEGROUP lpVictim);
LPNODEGROUP GetGroupPointer(LPNODEGROUP lpStartGroup, DWORD dwNewNodeIndex);

// Node functions
void InitialiseNodesInGroup(LPNODEGROUP lpGroup);
void AddNodeToSetAfter(struct tagThunkedData* pThisDisplay, LPNODE lpAfter, LPNODE lpAdd);
void AddNodeToSetBefore(struct tagThunkedData* pThisDisplay, LPNODE lpBefore, LPNODE lpAdd);
void RemoveNodeFromSet(struct tagThunkedData* pThisDisplay, LPNODE lpVictim);

BOOL WalkAndDrawHeap(struct tagThunkedData* pThisDisplay, DWORD MemPtr);
DWORD GetFreeMemInHeap(struct tagThunkedData* pThisDisplay);

#ifdef DEBUG
// Debugging functions
void ValidateLinearHeap(struct tagThunkedData* pThisDisplay);
void DumpHeap(struct tagThunkedData* pThisDisplay);
void DumpNode(LPNODE lpNode);
#endif

LPNODE FindSharedMemory(struct tagThunkedData* pThisDisplay, DWORD dwFlags);
DWORD AllocateVideoMemory(struct tagThunkedData* pThisDisplay, LPMEMREQUEST lpmmrq);
DWORD FreeVideoMemory(struct tagThunkedData* pThisDisplay, DWORD dwPointer);

// For freeing all the memory
void FreeAllVideoMemory(struct tagThunkedData* pThisDisplay);

#ifdef __cplusplus
} // extern "C"
#endif


#endif // __LINALLOC_H_
