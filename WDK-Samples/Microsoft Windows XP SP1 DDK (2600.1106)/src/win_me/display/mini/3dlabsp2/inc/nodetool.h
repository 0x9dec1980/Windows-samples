/******************************Module*Header**********************************\
 *
 *                           *****************************
 *                           * Permedia 2: SAMPLE CODE   *
 *                           *****************************
 *
 * Module Name: nodetool.h
 *
 * Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
 * Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
 *
\*****************************************************************************/
//******************************************************************
// NODE HELPER FUNCTIONS
//******************************************************************

//******************************************************************
//
// Function: NODE_NextNode(source node), NODE_LastNode(source node)
//
// Returns: LPNODE
//
// Description:
//
// Given a node will return a pointer to the next or last node it points to.
// Uses the flag in the node to determine the type of the current node.
// 
//
//******************************************************************
_inline LPNODE NODE_NextNode(LPP2THUNKEDDATA pThisDisplay, LPNODE lpNode)
{
	LPNODE NextPointer;
	LPNODEGROUP lpNewGroup;

#ifdef DEBUG
	if (lpNode == NULL) ASSERTDD(0,"Next node jump: invalid pointer");
#endif

	if (lpNode->dwNodeGroup == lpNode->NextNode.dwNodeGroup)
	{	
		NextPointer = &lpNode[(lpNode->NextNode.dwIndex) - (lpNode->dwIndex)];
	}
	else // We need to jump to a different node group
	{
		// Get the group pointer
		lpNewGroup = GetGroupPointer(&pThisDisplay->StartNodeGroup, lpNode->NextNode.dwNodeGroup);

		// Get the address of the node in the different group
		NextPointer = &lpNewGroup->node[(lpNode->NextNode.dwIndex)];
	}
	return NextPointer;
}

_inline LPNODE NODE_LastNode(LPP2THUNKEDDATA pThisDisplay, LPNODE lpNode)
{
	LPNODE NextPointer;
	LPNODEGROUP lpNewGroup;

#ifdef DEBUG
	if (lpNode == NULL) ASSERTDD(0,"Last node jump: invalid pointer");
#endif

	if (lpNode->dwNodeGroup == lpNode->LastNode.dwNodeGroup)
	{
		NextPointer = &lpNode[(lpNode->LastNode.dwIndex) - (lpNode->dwIndex)];
	}
	else // We need to jump to a different node group
	{
		// Get the group pointer
		lpNewGroup = GetGroupPointer(&pThisDisplay->StartNodeGroup, lpNode->LastNode.dwNodeGroup);

		// Get the address of the node in the different group
		NextPointer = &lpNewGroup->node[(lpNode->LastNode.dwIndex)];
	}
	return NextPointer;
}

//******************************************************************
//
// Function: NODE_SetNext(setting node, to target), NODE_SetLast(setting node, to target)
//
// Returns: void
//
// Description:
//
// Given a source node, will set it's next or last node to point to the
// target node passed in.
// 
//
//******************************************************************
_inline void NODE_SetNext(LPNODE lpSetNode, LPNODE lpTarget)
{
#ifdef DEBUG
	if (lpSetNode == NULL) ASSERTDD(0,"SetNext node: invalid Setable node");
	if (lpTarget == NULL) ASSERTDD(0,"SetNext node: invalid target node");
	if (lpSetNode->dwFlag == Dud_Node)
	{
		ASSERTDD(0,"SetNext node : dud setable node");
	}
	if (lpTarget->dwFlag == Dud_Node)
	{
		ASSERTDD(0,"SetNext node : dud target node");
	}
	if (lpTarget->dwFlag != lpSetNode->dwFlag)
	{
		ASSERTDD(0,"SetNext node: source and target not of same type");
	}
#endif

	lpSetNode->NextNode.dwIndex = lpTarget->dwIndex;
	lpSetNode->NextNode.dwNodeGroup = lpTarget->dwNodeGroup;
}

_inline void NODE_SetLast(LPNODE lpSetNode, LPNODE lpTarget)
{
#ifdef DEBUG
	if (lpSetNode == NULL) ASSERTDD(0,"SetNext node: invalid Setable node");
	if (lpTarget == NULL) ASSERTDD(0,"SetNext node: invalid target node");
	if (lpSetNode->dwFlag == Dud_Node)
	{
		ASSERTDD(0,"SetNext node : dud setable node");
	}
	if (lpTarget->dwFlag == Dud_Node)
	{
		ASSERTDD(0,"SetNext node : dud target node");
	}
	if (lpTarget->dwFlag != lpSetNode->dwFlag)
	{
		ASSERTDD(0,"SetNext node: source and target not of same type");
	}

#endif

	lpSetNode->LastNode.dwIndex = lpTarget->dwIndex;
	lpSetNode->LastNode.dwNodeGroup = lpTarget->dwNodeGroup;
}

#define INVALID_BASEINDEX_ID 0xFFFFFFFF
#define BASE_VALID(a) (a.dwIndex != INVALID_BASEINDEX_ID)
#define BASE_INVALIDATE(a) { a.dwIndex = INVALID_BASEINDEX_ID; }

//******************************************************************
//
// Function: NODE_GetBase(Base offset)
//
// Returns: LPNODE
//
// Description:
//
// Given a node offset will return a pointer to the node.  If the
// offset passed in is invalid, will return NULL (HOW long to find
// this bug!!!??)
// 
//
//******************************************************************
_inline LPNODE NODE_GetBase(LPP2THUNKEDDATA pThisDisplay, NODEOFFSET Offset)
{
	LPNODEGROUP lpNewGroup;

    if (!BASE_VALID(Offset))
    {
        return NULL;
    }

	if (Offset.dwNodeGroup == pThisDisplay->StartNodeGroup.dwNodeGroup)
	{	
		return &pThisDisplay->StartNodeGroup.node[Offset.dwIndex];
	}
	else
	{
		lpNewGroup = GetGroupPointer(&pThisDisplay->StartNodeGroup, Offset.dwNodeGroup);
		return &lpNewGroup->node[Offset.dwIndex];
	}
}

//******************************************************************
//
// Function: NODE_SetBase(base pointer, node pointer)
//
// Returns: LPNODEOFFSET
//
// Description:
//
// Given a node offset will set a pointer to it.  If a NULL pointer
// is passed in, the offset will be invalidated.
// 
// Created: chris.maughan@3Dlabs.com
//
//******************************************************************

_inline void NODE_SetBase(LPNODEOFFSET lpBase, LPNODE lpNode)
{
    if (lpNode == NULL)
    {
        lpBase->dwIndex = INVALID_BASEINDEX_ID;
        return;
    }

	lpBase->dwIndex = lpNode->dwIndex;
	lpBase->dwNodeGroup = lpNode->dwNodeGroup;
}
