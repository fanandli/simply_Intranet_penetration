#pragma once
#include "stddef.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

struct list_head {
  struct list_head *next, *prev;
};

#define list_entry(ptr, type, member) container_of(ptr, type, member)

//#define offsetof(TYPE, MEMBER) ((size_t)& ((TYPE *)0)->MEMBER)  

#define container_of(ptr, type, member) (type*)((char *)ptr - offsetof(type,member))

#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_del(_head) \
  for (list_head* __next, *_pos = (_head)->next; __next=_pos->next, _pos != (_head); _pos =__next)

#define container_of_del(_type, _member) (_type*)((char *)_pos - offsetof(_type,_member))

//#define LIST_HEAD_INIT(name) { &(name), &(name) }                 //prev��next��ָ���Լ�

inline void INIT_LIST_HEAD(struct list_head *list) {
  list->next = list;
  list->prev = list;
}

//new�ڱ������ﱻ�����ؼ���   ԭ������new ����Ϊl_new
//l_new ��Ҫ������Ľڵ�
//prev �ǲ����ǰ���һ���ڵ�
//next �ǲ��������һ���ڵ�
inline void __list_add(struct list_head *l_new,
  struct list_head *prev,
  struct list_head *next) {
  next->prev = l_new;
  l_new->next = next;
  l_new->prev = prev;
  prev->next = l_new;
}

//�������������� �¼���Ľڵ���head��head->next֮��  Ҳ����head֮��
inline void list_add(struct list_head *l_new, struct list_head *head) {
  __list_add(l_new, head, head->next);
}

//�������������� �¼���Ľڵ���head->prev��head֮��  Ҳ����head֮ǰ
inline void list_add_tail(struct list_head *l_new, struct list_head *head) {
  __list_add(l_new, head->prev, head);
}

//ɾ��һ��˫���б��е�һ���ڵ� ɾ���ڵ���prev��next֮��
inline void __list_del(struct list_head *prev, struct list_head *next) {
  next->prev = prev;
  prev->next = next;
}

//���б���ɾ��entry�ڵ㣬��������Ƕ����溯���ļ�
//entry->next = LIST_POISON1;
//entry->prev = LIST_POISON2;
inline void list_del(struct list_head *entry) {
  __list_del(entry->prev, entry->next);
//  entry->next = nullptr;    //������ָ����Բ����������list��del����һ���ڵ㣬
//  entry->prev = nullptr;    //������ ����ڵ��Ҫ��ɾ��
}

//�ж�list�ǲ������һ���ڵ�
//head�ڵ��Ƕ��еĵ�һ���ڵ�
//return list->next == head  ���� return head->prev == listһ��
inline int list_is_last(const struct list_head *list,
  const struct list_head *head) {
  return list->next == head;
}

//�б��ǲ���Ϊ��
//head�ڵ� �Ƕ��е�ͷ�ڵ�
inline int list_empty(const struct list_head *head) {
  return head->next == head;
}

#ifdef __cplusplus
}
#endif //__cplusplus

