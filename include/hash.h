/**
 * @file    hash.h
 * @brief   拉链法哈希表（Chaining Hash Table）接口声明 —— 校园选课记录检索与大数据分析系统
 *
 * 本模块实现以 CourseRecord.key（学号_课程号）为键的拉链法哈希表，
 * 支持 O(1) 均摊的精确增删改查，作为 AVL 树的辅助快速索引。
 *
 * 哈希函数采用 FNV-1a 算法，将任意长度的字符串 key 转化为一个高度随机、分布均匀的 32位无符号整数（unsigned long）。
 * 结合表容量对键字符串产生均匀分布的槽位下标。
 *
 * 说明：哈希表中存储的 CourseRecord 指针与 AVL 树共享同一块堆内存，
 * 请勿在两处重复 free()，统一由 AVL 树节点负责释放。
 *
 * @author  鹿智明
 * @date    2026-06-23
 * @version 1.0
 */

#ifndef HASH_H
#define HASH_H

#include "record.h"

/* -----------------------------------------------------------------------
 * 哈希表参数宏
 * ---------------------------------------------------------------------- */

/** 默认槽位数量（建议为质数，减少碰撞）*/
#define HASH_DEFAULT_CAPACITY  1021

/** 装载因子上限；超过后可视需求扩容（本版本不强制动态扩容）*/
#define HASH_LOAD_FACTOR_MAX   0.75

/* -----------------------------------------------------------------------
 * 哈希表节点（拉链法链表节点）
 * ---------------------------------------------------------------------- */

/**
 * @struct HashNode
 * @brief  哈希表拉链中的单个节点。
 *
 * 存储对应 CourseRecord 的指针（与 AVL 树共享，不拥有所有权），
 * 以及指向同槽下一节点的指针（拉链）。
 */
typedef struct HashNode {
    CourseRecord    *data;  /**< 指向 CourseRecord 的指针（非所有权）*/
    struct HashNode *next;  /**< 拉链中的下一节点（NULL 表示链尾）   */
} HashNode;

/* -----------------------------------------------------------------------
 * 哈希表结构体
 * ---------------------------------------------------------------------- */

/**
 * @struct HashTable
 * @brief  拉链法哈希表。
 *
 * buckets 是一个 HashNode* 数组，每个元素是一条链表的头节点。
 */
typedef struct HashTable {
    HashNode **buckets;   /**< 槽位数组（长度为 capacity）              */
    int        capacity;  /**< 槽位总数（建议质数）                      */
    int        size;      /**< 当前存储的记录总数                        */
} HashTable;

/* -----------------------------------------------------------------------
 * 哈希函数
 * ---------------------------------------------------------------------- */

/*
 * @brief  FNV-1a 哈希函数，比 DJB2 在短字符串场景分布更均匀。
 *
 * @param  key  非 NULL 的 C 字符串。
 * @return 无符号散列值（不含取模）。
 */
unsigned long hash_fnv1a(const char *key);

/* -----------------------------------------------------------------------
 * 初始化 / 销毁
 * ---------------------------------------------------------------------- */

/**
 * @brief  初始化哈希表，分配槽位数组并清零。
 *
 * @param  ht        指向待初始化的 HashTable（非 NULL）。
 * @param  capacity  槽位数量（传 0 则使用 HASH_DEFAULT_CAPACITY）。
 * @return 0 表示成功；-1 表示内存分配失败。
 */
int hash_init(HashTable *ht, int capacity);

/**
 * @brief  销毁哈希表：释放所有 HashNode 链表节点及槽位数组。
 *         注意：不释放 HashNode.data 指向的 CourseRecord（由 AVL 树负责）。
 *
 * @param  ht  指向已初始化的 HashTable（非 NULL）。
 */
void hash_destroy(HashTable *ht);

/* -----------------------------------------------------------------------
 * 核心操作接口
 * ---------------------------------------------------------------------- */

/**
 * @brief  向哈希表插入一条记录（若 key 已存在则拒绝）。
 *
 * @param  ht      已初始化的 HashTable 指针。
 * @param  record  指向 CourseRecord 的指针（共享所有权，不拷贝）。
 * @return  0 表示插入成功；
 *          1 表示 key 已存在（不重复插入）；
 *         -1 表示内存分配失败。
 */
int hash_insert(HashTable *ht, CourseRecord *record);

/**
 * @brief  从哈希表中删除指定 key 的节点（只释放 HashNode，不释放 CourseRecord）。
 *
 * @param  ht   已初始化的 HashTable 指针。
 * @param  key  要删除的唯一主键字符串。
 * @return  0 表示删除成功；-1 表示 key 不存在。
 */
int hash_delete(HashTable *ht, const char *key);

/**
 * @brief  在哈希表中按 key 精确查找一条记录（O(1) 均摊）。
 *
 * @param  ht   已初始化的 HashTable 指针。
 * @param  key  要查找的唯一主键字符串。
 * @return 找到时返回对应 CourseRecord 指针；未找到返回 NULL。
 */
CourseRecord *hash_search(HashTable *ht, const char *key);

/**
 * @brief  更新哈希表中 key 对应节点的 data 指针（用于记录修改后更新索引）。
 *
 * @param  ht      已初始化的 HashTable 指针。
 * @param  key     要更新的唯一主键字符串。
 * @param  record  新的 CourseRecord 指针。
 * @return  0 表示更新成功；-1 表示 key 不存在。
 */
int hash_update(HashTable *ht, const char *key, CourseRecord *record);

/**
 * @brief  打印哈希表各槽位的填充情况（用于调试/分析分布均匀性）。
 * @param  ht  已初始化的 HashTable 指针。
 */
void hash_print_stats(const HashTable *ht);

#endif /* HASH_H */
