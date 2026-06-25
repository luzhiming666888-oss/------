/**
 * @file    util.h
 * @brief   文件 I/O、排序与筛选辅助接口声明 —— 校园选课记录检索与大数据分析系统
 *
 * 本模块提供三类能力：
 *   1. JSON Lines 格式的文件读写（手写轻量序列化/反序列化，不依赖外部库）
 *   2. 针对 CourseRecord 指针数组的多关键字快速排序
 *   3. 多维筛选（按学院、学期、成绩段、课程等条件过滤）
 *
 * 文件格式约定（JSON Lines，每行一个对象）：
 * {"sid":"202401011234","name":"张三","college":"计算机学院","cid":"CS300102",
 *  "cname":"数据结构","credit":3.0,"semester":"2024-2025-1",
 *  "date":"2024-09-01","score":88}
 *
 * @author  （作者姓名 / 学号，待填写）
 * @date    2026-06-23
 * @version 1.0
 */

#ifndef UTIL_H
#define UTIL_H

#include "record.h"
#include "avl.h"
#include "hash.h"

/* -----------------------------------------------------------------------
 * 文件 I/O 接口
 * ---------------------------------------------------------------------- */

/**
 * @brief  将 AVL 树中的所有选课记录以 JSON Lines 格式保存到文件。
 *
 * 采用 AVL 中序遍历（按 key 升序），每条记录写为一行 JSON 对象，
 * 末尾以 '\n' 结束；文件写入完成后自动关闭。
 *
 * @param  filename  目标文件路径（如 "data/records.jsonl"）。
 * @param  root      AVL 树根节点（可为 NULL，则输出空文件）。
 * @return  0 表示成功；-1 表示文件打开失败。
 */
int save_to_json(const char *filename, AVLNode *root);

/**
 * @brief  从 JSON Lines 文件加载选课记录，同时构建 AVL 树和哈希表双索引。
 *
 * 逐行读取文件，手工解析 JSON 键值对，为每条记录分配 CourseRecord 堆内存，
 * 并同步插入到 *root（AVL 树）和 ht（哈希表）中。
 *
 * @param  filename  源文件路径。
 * @param  ht        已初始化的 HashTable 指针（非 NULL）。
 * @param  root      AVL 树根节点的二级指针（初始 *root 可为 NULL）。
 * @return 成功加载的记录条数；文件不存在时返回 0；解析错误时返回 -1。
 */
int load_from_json(const char *filename, HashTable *ht, AVLNode **root);

/* -----------------------------------------------------------------------
 * 手写 JSON 序列化 / 反序列化辅助（util.c 内部使用，也供单元测试）
 * ---------------------------------------------------------------------- */

/**
 * @brief  将一条 CourseRecord 序列化为 JSON 字符串（写入 buf）。
 *
 * @param  record  源记录指针（非 NULL）。
 * @param  buf     输出缓冲区。
 * @param  buf_sz  缓冲区字节数。
 * @return 实际写入字节数（不含 '\0'）；缓冲区不足返回 -1。
 */
int  record_to_json(const CourseRecord *record, char *buf, int buf_sz);

/**
 * @brief  将一行 JSON 字符串反序列化为 CourseRecord（调用者提供已分配内存）。
 *
 * @param  json    一行 JSON 字符串（以 '\0' 结尾，不含换行符）。
 * @param  out     指向已分配的 CourseRecord 内存（非 NULL）。
 * @return  0 表示解析成功；-1 表示格式错误或字段缺失。
 */
int  json_to_record(const char *json, CourseRecord *out);

/* -----------------------------------------------------------------------
 * 排序接口（快速排序，支持多关键字比较）
 * ---------------------------------------------------------------------- */

/**
 * @brief  排序方式枚举，传入 sort_records() 指定排序关键字。
 */
typedef enum SortKey {
    SORT_BY_KEY        = 0,  /**< 按唯一主键（学号_课程号）升序  */
    SORT_BY_STUDENT_ID = 1,  /**< 按学号升序                      */
    SORT_BY_COURSE_ID  = 2,  /**< 按课程编号升序                  */
    SORT_BY_SCORE_ASC  = 3,  /**< 按成绩升序                      */
    SORT_BY_SCORE_DESC = 4,  /**< 按成绩降序                      */
    SORT_BY_CREDIT_ASC = 5,  /**< 按学分升序                      */
    SORT_BY_SEMESTER   = 6,  /**< 按学期字符串升序                */
    SORT_BY_DATE       = 7,  /**< 按选课日期升序                  */
} SortKey;

/**
 * @brief  对 CourseRecord 指针数组执行快速排序（原地修改）。
 *
 * @param  arr   CourseRecord 指针数组（非 NULL）。
 * @param  n     数组元素个数。
 * @param  key   排序关键字（见 SortKey 枚举）。
 */
void sort_records(CourseRecord **arr, int n, SortKey key);

/* -----------------------------------------------------------------------
 * 筛选接口
 * ---------------------------------------------------------------------- */

/**
 * @brief  筛选条件结构体（各字段均为可选，空字符串或特殊值表示不过滤）。
 */
typedef struct FilterCriteria {
    char  student_id[STUDENT_ID_LEN]; /**< 按学号前缀/精确匹配；空字符串跳过  */
    char  college[COLLEGE_LEN];       /**< 按学院精确匹配；空字符串跳过        */
    char  course_id[COURSE_ID_LEN];   /**< 按课程编号精确匹配；空字符串跳过    */
    char  semester[SEMESTER_LEN];     /**< 按学期精确匹配；空字符串跳过        */
    int   score_min;                  /**< 成绩下限（-1 表示不设下限）         */
    int   score_max;                  /**< 成绩上限（-1 表示不设上限）         */
    float credit_min;                 /**< 学分下限（< 0 表示不设下限）        */
} FilterCriteria;

/**
 * @brief  从记录指针数组中筛选满足条件的记录，结果写入 out 数组。
 *
 * @param  in       输入记录指针数组（非 NULL）。
 * @param  in_cnt   输入数组长度。
 * @param  out      输出数组，调用者需预分配至少 in_cnt 个指针的空间。
 * @param  fc       筛选条件指针（非 NULL）。
 * @return 实际满足条件的记录数量。
 */
int filter_records(CourseRecord **in,  int in_cnt,
                   CourseRecord **out, const FilterCriteria *fc);

/* -----------------------------------------------------------------------
 * 统计分析接口
 * ---------------------------------------------------------------------- */

/**
 * @brief  统计记录数组的成绩分布（各分段人数）。
 *
 * 分段：[0,60) 不及格  [60,70) 及格  [70,80) 中等  [80,90) 良好  [90,100] 优秀
 *
 * @param  arr    CourseRecord 指针数组。
 * @param  n      数组元素个数。
 * @param  dist   输出数组，长度 5，依次对应上述 5 个分段计数。
 */
void stat_score_distribution(CourseRecord **arr, int n, int dist[5]);

/**
 * @brief  计算记录数组中成绩的平均值（忽略 score == -1 的未录入项）。
 *
 * @param  arr  CourseRecord 指针数组。
 * @param  n    数组元素个数。
 * @return 平均成绩；若全部未录入则返回 -1.0。
 */
float stat_average_score(CourseRecord **arr, int n);

/* -----------------------------------------------------------------------
 * 字符串工具
 * ---------------------------------------------------------------------- */

/**
 * @brief  安全的字符串截断复制（保证结果以 '\0' 结尾，类似 strlcpy）。
 *
 * @param  dst   目标缓冲区。
 * @param  src   源字符串。
 * @param  size  目标缓冲区字节数（含 '\0'）。
 * @return 源字符串的原始长度。
 */
int util_strncpy(char *dst, const char *src, int size);

/**
 * @brief  去除字符串首尾的空白字符（原地修改）。
 * @param  s  待处理字符串（非 NULL）。
 */
void util_trim(char *s);

#endif /* UTIL_H */
