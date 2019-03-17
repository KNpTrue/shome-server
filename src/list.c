//list.c
#include "list.h"
#include <stdio.h>

bool appendList(node_t **head, void *data)
{
    node_t *newNode = (node_t *)malloc(sizeof(node_t));
    if(newNode == NULL)     return false;
    newNode->data = data;
    //头部插入
    newNode->next = *head;
    *head = newNode;
    return true;
}

bool appendTailList(node_t **head, void *data)
{
    if(!data)   return false;
    node_t *newNode = (node_t *)malloc(sizeof(node_t));
    if(newNode == NULL)     return false;
    newNode->data = data;
    newNode->next = NULL;
    //尾部插入
    node_t **t = head;
    while(*t != NULL)    t = &(*t)->next;
    *t = newNode;
    return true;
}

void *seachOneByRequired(node_t *head, required_callback required, void *tag)
{
    node_t *t = head;
    while(t != NULL)
    {
        if(required(t->data, tag) == true)
        {
            return t->data;
        }
        t = t->next;
    }
    return NULL;
}

node_t *seachByRequired(node_t *head, required_callback required, void *tag)
{
    node_t *newList = NULL;
    node_t *t = head;
    while(t!= NULL)
    {
        if(required(t->data, tag) == true)
        {
            appendList(&newList, t->data);
        }
        t = t->next;
    }
    return newList;
}

void travelList(node_t *head, manipulate_callback manipulate, void *tag)
{
    while(head != NULL)
    {
        manipulate(head->data, tag);
        head = head->next;
    }
}

bool deleteNode(node_t **head, required_callback required, void *tag, destory_callback destory)
{
    
    node_t *cur = *head, **t = head;
    while(*t != NULL)
    {
        if(required((*t)->data, tag) == true)
        {
            cur = *t;
            *t = (*t)->next;
            if(destory) destory(cur->data);
            free(cur);
            return true;
        }
        t = &(*t)->next;
    }
    return false;
}

void deleteList(node_t **head, destory_callback destory)
{
    node_t *cur, *t = *head;
    while(t != NULL)
    {
        cur = t;
        t = t->next;
        if(destory) destory(cur->data);
        free(cur);
    }
    *head = NULL;
}

unsigned int getNodeCount(node_t *head)
{
    unsigned int count = 0;
    while(head != NULL)
    {
        head = head->next;
        count++;
    }
    return count;
}

bool isEmptyList(node_t *head)
{
    return head == NULL;
}