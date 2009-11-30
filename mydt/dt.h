// vim:fdm=marker:nu:nowrap:encoding=gbk
#ifndef _DT_H_
#define _DT_H_

#include "config.h"
#include "types.h"

// �ڵ�
typedef struct {
	real x;
	real y;
} node;

// +y -x Ϊ��
#define NODE_ORD_CMP(n1, n2) ((n1)->y > (n2)->y || ( (n1)->y == (n2)->y && (n1)->x < (n2)->x ))

typedef void (*edgeHandler)(const node *, const node *);

// main struct
typedef void* myDt;

// dt struct creation and destruction
boolean dt_create(myDt * pdt);
void dt_destroy(myDt * pdt);

/* ��������api���ɻ���, ͬʱ�ڵ��õ���������, ���뱣֤���е�����겻�ܱ��ı�*/

void dt_begin(myDt dt, edgeHandler handler);
/* ��һ��api
 * dt_next/dt_end �÷�
 * dt_begin ��ʼ��dt�ṹ, ��ʾ׼����ʼһ���µ�һ�����У�֮ǰ�����ݽ��ᱻ���
 * dt_next �������㼯�е�ÿһ����
 * dt_end ���� dt_next ����ĵ㼯
 */
void dt_next(myDt dt, node * n);
void dt_end(myDt dt);

/* �ڶ���api
 * �÷������͵�һ��һ�£�Ψһ��֮ͬ������ʹ����һ��api���뱣֤
 * dt_next_sorted ����ĵ㼯���밴�� NODE_ORD_CMP ָ����˳������
 * ��ЩӦ�ó����µ㼯�����Ѿ���ĳ����ʽ�ź���, ʹ��
 * ��һ��api����ʡȴ�����ȵ�һ��Ҫ����ЧһЩ, ͬʱռ���ڴ�Ҳ����
 */
void dt_next_sorted(myDt dt, node * pt);
void dt_end_sorted(myDt dt);

#ifdef COUNT_CALL
// after_break_point count
extern uint32 abp_cnt;
// candidate_circle_event count
extern uint32 cce_cnt;
// handle_site_event count
extern uint32 hse_cnt;
// handle_cirl_event count
extern uint32 hce_cnt;
#endif

#endif
