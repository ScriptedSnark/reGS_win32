#ifndef ENGINE_WORLD_H
#define ENGINE_WORLD_H

typedef struct areanode_s
{
	int axis;
	float dist;
	areanode_s* children[2];
	link_t trigger_edicts;
	link_t solid_edicts;
} areanode_t;

void ClearLink(link_t* l);
void RemoveLink(link_t* l);
void InsertLinkBefore(link_t* l, link_t* before);
void InsertLinkAfter(link_t* l, link_t* after);
bool SV_RecursiveHullCheck(hull_t* hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t* trace);

#endif // ENGINE_WORLD_H
