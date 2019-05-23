//list.h
#ifndef _LIST_H
#define _LIST_H

#include "shome-types.h"

//用户必须实现malloc free !!
extern malloc_cb list_malloc;
extern free_cb list_free;

//条件函数指针
typedef bool (*required_callback)(void *data, void *tag);
//操作函数指针
typedef void (*manipulate_callback)(void *data, void *tag);
//销毁函数指针
typedef void (*destory_callback)(void *data);
//链表结构体
typedef struct node {
    void *data; //数据指针
    struct node *next; //下一个结点
} node_t;

/**
 * 插入链表
 * @head: 头指针, 改变头指针
 * @data: 数据指针
*/
bool appendList(node_t **head, void *data);
bool appendTailList(node_t **head, void *data);

/**
 * 根据条件找一个结点数据
 * @head:     头指针, 可能改变头指针
 * @call_back:比较函数指针
*/
void *seachOneByRequired(node_t *head, required_callback required, void *tag);

/**
 * 根据条件找多个数据，返回链表头指针
 * @head:     头指针, 可能改变头指针
 * @call_back:比较函数指针
*/
node_t *seachByRequired(node_t *head, required_callback required, void *tag);

/**
 * 根据idx找一个数据，返回链表头指针
 * @head:     头指针, 可能改变头指针
 * @idx: index
*/
void *seachOneByIdx(node_t *head, unsigned int idx);

/**
 * 遍历链表
 * @head:     头指针, 可能改变头指针
 * @call_back:操作函数指针
*/
void travelList(node_t *head, manipulate_callback manipulate, void *tag);

/**
 * 删除某个结点
 * @head:     头指针, 可能改变头指针
 * @call_back:比较函数指针
*/
bool deleteNode(node_t **head, required_callback required, void *tag, destory_callback destory);

/**
 * 销毁链表
 * @head: 头指针, 销毁后head = NULL
 * 数据在函数中没有销毁
*/
void deleteList(node_t **head, destory_callback destory);

/**
 * 获得结点个数
*/
unsigned int getNodeCount(node_t *head);

/**
 * 是否为空链表
*/
bool isEmptyList(node_t *head);

#endif //_LIST_H