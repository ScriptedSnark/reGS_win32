#include "quakedef.h"
#include "world.h"

void ClearLink(link_t* l)
{
	l->next = l->prev = l;
}

void RemoveLink(link_t* l)
{
	l->next->prev = l->prev;
	l->prev->next = l->next;
}

void InsertLinkBefore(link_t* l, link_t* before)
{
	l->next = before;
	l->prev = before->prev;
	l->next->prev = l;
	l->prev->next = l;
}

void InsertLinkAfter(link_t* l, link_t* after)
{
	l->prev = after;
	l->next = after->next;
	after->next = l;
	l->next->prev = l;
}

bool SV_RecursiveHullCheck(hull_t* hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t* trace)
{
	// TODO: implement - Solokiller
	return false;
}
