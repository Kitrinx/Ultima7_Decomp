#ifndef DATANODE_H
#define DATANODE_H

/* A save node. Each one links itself into SaveNodes; saving and restoring walk the list and hand
 * every node the save directory. */
struct DataNode {
	DataNode *next;
	DataNode();
	DataNode(int16_t);
	char *getName();
	virtual char *name();
	virtual void load(char *dir);
	virtual void save(char *dir);
	virtual void refresh(char *dir);
};

extern DataNode *SaveNodes;

#endif
