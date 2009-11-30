// vim:fdm=marker:nu:nowrap:encoding=gbk

#ifndef _MEM_POOL_H
#define _MEM_POOL_H

#include "types.h"

typedef struct memSeg {
    struct memSeg * next;
    byte * addr_after_last;
    byte mem[0];
} memSeg;

typedef struct {
    void * free_list;               // �黹�Ŀ�������
    byte * next_addr;               // ����������Ϊ�գ������ָ�������һ���ɷ���ĵ�ַ ( �п���==addr_after_last )
    memSeg * head_seg;              // memSeg ��һ����������ͷ
    memSeg * curr_seg;              // ��ǰmemSeg
    uint32 item_sz;                 // ����˼�� ( �������� item_sz >= sizeof(void*) )
    uint32 inc_sz;                  // ÿһ��seg�а�����item����
} memPool;

boolean mem_pool_init(memPool * pool, uint32 item_sz, uint32 inc_sz);
void mem_pool_reset(memPool * pool);
void mem_pool_finalize(memPool * pool);
void * mem_pool_get(memPool * pool);
void mem_pool_release(memPool * pool, void * item);

#endif
