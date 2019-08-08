
#include "jiot_list.h"


JList_t * initHead()
{
    JList_t *self;
    if (!(self = jiot_malloc(sizeof(JList_t))))
        return NULL;

    self->_head = NULL;
    self->_tail = NULL;
    self->_size = 0;

    jiot_mutex_init(&(self->_mtx));
    
    return self;
}

void releaseHead(JList_t *self)
{
    jiot_mutex_destory(&(self->_mtx));
    jiot_free(self);
}


_JLNode_t *NextNode(JList_t *self,_JLNode_t *node)
{
    //node == NULL ,从头节点开始遍历
    _JLNode_t *curr = NULL;

    if (NULL == node)
    {
        curr = self->_head;
    }
    else
    {
        curr = node->_next;
        if (NULL == curr)
        {
            //遍历到底部，返回NULL
            return NULL ;
        }
    }
    return curr;
}


_JLNode_t * PushList(JList_t *self, void * val)
{
    jiot_mutex_lock(&(self->_mtx));

    if (self->_size >= LIST_MAX)
    {
        jiot_mutex_unlock(&(self->_mtx));
        return NULL;
    }

    _JLNode_t * node = jiot_malloc(sizeof(JList_t));
    if (NULL == node) 
    {
        jiot_mutex_unlock(&(self->_mtx));
        return NULL;
    }
    node->_val = val;
    node->_prev = NULL;
    node->_next = NULL;

    if (self->_size > 0)
    {
        node->_prev = self->_tail;
        self->_tail->_next = node;
        self->_tail = node;  
    }
    else
    {
        self->_head = self->_tail = node;
    }

    ++self->_size;

    jiot_mutex_unlock(&(self->_mtx));

    return node;
}


//从head出
_JLNode_t *PopList(JList_t *self)
{
    jiot_mutex_lock(&(self->_mtx));

    _JLNode_t *node = NULL;
    if (!self->_size) 
    {
        jiot_mutex_unlock(&(self->_mtx));
        return NULL;
    }

    node = self->_head;

    if (--self->_size)
    {
        self->_head = node->_next;
        self->_head->_prev = NULL;
    }
    else
    {
        self->_head = self->_tail = NULL;
    }

    jiot_mutex_unlock(&(self->_mtx));
    node->_next = node->_prev = NULL;
    
    return node;
}

void DelNode(JList_t *self, _JLNode_t *node)
{
    jiot_mutex_lock(&(self->_mtx));

    if(node->_prev)
    {
        node->_prev->_next = node->_next;
    }
    else
    {
        self->_head = node->_next;
    }

    if(node->_next)
    {
        node->_next->_prev = node->_prev;   
    }
    else
    {
        self->_tail = node->_prev;
    }
    
    --self->_size;
    
    jiot_mutex_unlock(&(self->_mtx));

    jiot_free(node);

}
