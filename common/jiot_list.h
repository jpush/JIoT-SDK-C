#ifndef JIOT_LIST_H
#define JIOT_LIST_H

#include <stdlib.h>
#include "jiot_pthread.h"


#define LIST_MAX 30 

typedef struct _JLNode 
{
  struct _JLNode *_next;
  struct _JLNode *_prev;
  void * _val;
} _JLNode_t;

typedef struct {
  _JLNode_t *    _head;
  _JLNode_t *    _tail;
  unsigned int  _size;
  S_JIOT_MUTEX  _mtx;
} JList_t;


JList_t* initHead();

void    releaseHead(JList_t *);

_JLNode_t* NextNode(JList_t *self, _JLNode_t *node);

_JLNode_t* PushList(JList_t *self, void *val);

_JLNode_t* PopList(JList_t *self);

void DelNode(JList_t *self, _JLNode_t *node);

#endif

