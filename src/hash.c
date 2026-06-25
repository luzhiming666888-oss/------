/**
 * @file    hash.c
 * @brief   拉链法哈希表实现 —— 校园选课记录检索与大数据分析系统
 *
 * 实现要点：
 *   - 使用 FNV-1a 哈希函数，对 key 字符串产生槽位下标。
 *   - 每个槽位维护一条 HashNode 单链表（拉链）。
 *   - 哈希表仅持有 CourseRecord 指针（共享所有权），不负责释放 record 内存。
 *   - hash_destroy() 只释放 HashNode 节点本身及槽位数组，不 free data。
 *
 * @author  鹿智明
 * @date    2026-06-24
 * @version 1.2 (健壮性与统计完善版)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/hash.h"
#include "../include/record.h"

/* -----------------------------------------------------------------------
 * 内部私有辅助函数（static 限制作用域，仅本文件可见）
 * ---------------------------------------------------------------------- */

/**
 * @brief   FNV-1a 32-bit 哈希算法
 * @details 将任意长度的字符串（key），转化成一个高度随机、分布均匀的 32位无符号整数
 */
unsigned long hash_fnv1a(const char *key) {
    unsigned long hash = 2166136261UL;
    const unsigned long FNV_PRIME = 16777619UL;
    while (*key) {
        hash ^= (unsigned char)*key++;
        hash *= FNV_PRIME;
    }
    return hash;
}

/**
 * @brief   计算槽位下标
 * @details 采用 const 保护只读参数，提高代码的健壮性
 */
static int get_bucket_index(const HashTable *ht, const char *key) {
    unsigned long hash_value = hash_fnv1a(key);
    return (int)(hash_value % ht->capacity);
}

/* -----------------------------------------------------------------------
 * 初始化 / 销毁
 * ---------------------------------------------------------------------- */

int hash_init(HashTable *ht, int capacity) {
    if (ht == NULL) return -1; 

    if (capacity <= 0) {
        ht->capacity = HASH_DEFAULT_CAPACITY;
    } else {
        ht->capacity = capacity;
    }

    // 分配并清空内存
    ht->buckets = (HashNode **)calloc(ht->capacity, sizeof(HashNode *));
    if (ht->buckets == NULL) {
        ht->capacity = 0;
        return -1; // 内存分配失败
    }
    ht->size = 0;
    return 0;
}

void hash_destroy(HashTable *ht) {
    if (ht == NULL || ht->buckets == NULL) return;

    for (int i = 0; i < ht->capacity; i++) {
        HashNode *node = ht->buckets[i];
        while (node) {
            HashNode *temp = node;
            node = node->next;
            free(temp); // 仅释放链表节点本身，不释放共享的 CourseRecord 内存
        }
    }
    free(ht->buckets);
    ht->buckets = NULL;
    ht->capacity = 0;
    ht->size = 0;
}

/* -----------------------------------------------------------------------
 * 核心操作
 * ---------------------------------------------------------------------- */

int hash_insert(HashTable *ht, CourseRecord *record) {
    if (ht == NULL || record == NULL) {
        return -1; // 异常校验
    }

    int index = get_bucket_index(ht, record->key);
    HashNode *current = ht->buckets[index];
    
    // 检查重复 key
    while (current) {
        if (strcmp(current->data->key, record->key) == 0) {
            return 1; // key 已存在
        }
        current = current->next;
    }

    // 申请新节点内存
    HashNode *new_node = (HashNode *)malloc(sizeof(HashNode));
    if (new_node == NULL) {
        return -1; // 内存分配失败
    }
    
    new_node->data = record;
    new_node->next = ht->buckets[index]; // 头插法连链
    ht->buckets[index] = new_node;
    
    ht->size++; 
    return 0;
}

int hash_delete(HashTable *ht, const char *key) {
    if (ht == NULL || key == NULL) return -1;

    int index = get_bucket_index(ht, key);
    HashNode *prev = NULL;
    HashNode *current = ht->buckets[index];

    while (current) {
        if (strcmp(current->data->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {    
                ht->buckets[index] = current->next;
            }
            free(current);
            ht->size--; // 递减记录总数
            return 0;  // 删除成功
        }
        prev = current;
        current = current->next;
    }
    return -1; // 未找到该 key
}

CourseRecord *hash_search(HashTable *ht, const char *key) {
    if (ht == NULL || key == NULL) return NULL;

    int index = get_bucket_index(ht, key);
    HashNode *current = ht->buckets[index];
    while (current) {
        if (strcmp(current->data->key, key) == 0) {
            return current->data; // 返回匹配成功的记录指针
        }
        current = current->next;
    }
    return NULL; // 未找到
}

int hash_update(HashTable *ht, const char *key, CourseRecord *record) {
    if (ht == NULL || key == NULL || record == NULL) return -1;

    int index = get_bucket_index(ht, key);
    HashNode *current = ht->buckets[index];
    while (current) {
        if (strcmp(current->data->key, key) == 0) {
            current->data = record; // 更新共享所有权的指针
            return 0;
        }
        current = current->next;
    }
    return -1;
}

/* -----------------------------------------------------------------------
 * 调试：打印槽位统计
 * ---------------------------------------------------------------------- */

void hash_print_stats(const HashTable *ht) {
    if (ht == NULL || ht->buckets == NULL) {
        printf("[HashTable Stats] Error: HashTable is uninitialized.\n");
        return;
    }

    int max_len = 0;
    int min_len = ht->size;
    int empty_buckets = 0;
    int non_empty_buckets = 0;

    // 统计每个槽位的链表长度
    for (int i = 0; i < ht->capacity; i++) {
        int current_len = 0;
        HashNode *node = ht->buckets[i];
        while (node) {
            current_len++;
            node = node->next;
        }

        if (current_len > 0) {
            non_empty_buckets++;
            if (current_len > max_len) max_len = current_len;
            if (current_len < min_len) min_len = current_len;
        } else {
            empty_buckets++;
        }
    }

    // 边界情况处理：如果表为空，则最短链表长应为 0
    if (non_empty_buckets == 0) {
        min_len = 0;
    }

    double load_factor = (double)ht->size / ht->capacity;
    double avg_len = non_empty_buckets > 0 ? (double)ht->size / non_empty_buckets : 0.0;

    printf("\n====== 哈希表运行状态统计 (HashTable Stats) ======\n");
    printf("  槽位总容量 (Capacity) : %d\n", ht->capacity);
    printf("  已存记录数 (Size)     : %d\n", ht->size);
    printf("  装载因子 (Load Factor): %.2f%%\n", load_factor * 100.0);
    printf("  空闲槽位数 (Empty)    : %d\n", empty_buckets);
    printf("  非空槽位数 (Active)   : %d\n", non_empty_buckets);
    printf("  最长链表长度 (Max Len): %d\n", max_len);
    printf("  最短链表长度 (Min Len): %d\n", min_len);
    printf("  平均活跃链长 (Avg Len): %.2f\n", avg_len);
    printf("==================================================\n\n");
}