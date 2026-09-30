/* Black Gate U7.EXE, resident segment 77 (file offsets 0x02b73a to 0x02b80b, 209 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "datanode.h"

DataNode *SaveNodes = 0;

DataNode::DataNode()
{
	next = SaveNodes;
	SaveNodes = this;
}

/* joins the end of the list rather than the front */
DataNode::DataNode(int16_t)
{
	DataNode *prev, *n;

	n = SaveNodes;
	prev = 0;
	while (n != 0) {
		prev = n;
		n = n->next;
	}
	if (prev != 0) {
		prev->next = this;
		next = 0;
	}
}

char *DataNode::getName()
{
	return name() ? name() : (char *)"Unknown";
}

char *DataNode::name()
{
	return 0;
}

void DataNode::load(char *)
{
}

void DataNode::save(char *)
{
}

void DataNode::refresh(char *dir)
{
	save(dir);
}

extern "C" void ResetDatanodeGlobals(void)
{
	SaveNodes = 0;
}
