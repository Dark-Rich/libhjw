// vim:fdm=marker:nu:nowrap:encoding=gbk

#include "config.h"
#include "types.h"
#include "mem_pool.h"

typedef struct treapNode treapNode;

struct treapNode {
	void * val;
	uint32 priority;				// the greater the closer to the root
	treapNode * parent;				// Ϊ��ʱ��ʾΪ root
	treapNode * left;
	treapNode * right;
};

typedef struct {
	treapNode * node;
	// �� node Ϊ��ʱ, ��ʾ����һ���սڵ�(�����ڵ�), ��ʱ parent �Լ� pnode
	// ����ָʾ����սڵ��λ��
	// �� parent Ҳ�ǿյ�, ���ʾ treap Ҳ�ǿյ�
	treapNode * parent;
	treapNode ** pnode;
} treapIter;

typedef struct {
	treapNode * root;
	memPool pool;
} treap;

void treap_reset(treap * t);
boolean treap_init(treap * t);
void treap_finalize(treap * t);
void * treap_insert_at(treap * t, treapIter * iter, void * val);
void treap_delete(treap * t, void * ptr);

/*********************************
 * interface
 *********************************/

typedef treap BST;
typedef treapIter BSTIter;

#define BST_INIT treap_init
#define BST_FINALIZE treap_finalize
#define BST_RESET treap_reset
#define BST_INSERT_AT treap_insert_at
#define BST_DELETE treap_delete

#define BST_ITER_INIT(bst, piter) (piter)->node = (bst)->root; \
								 (piter)->parent = 0; \
								 (piter)->pnode = &((bst)->root)

#define BST_ITER_NOTNIL(piter) ((piter)->node != 0)
#define BST_ITER_DEREF(piter) ((piter)->node->val)
#define BST_ITER_FORWARD(piter) (piter)->parent = (piter)->node; \
								(piter)->pnode = &((piter)->node->right); \
								(piter)->node = *((piter)->pnode)

#define BST_ITER_BACKWARD(piter) (piter)->parent = (piter)->node; \
								(piter)->pnode = &((piter)->node->left); \
								(piter)->node = *((piter)->pnode)

