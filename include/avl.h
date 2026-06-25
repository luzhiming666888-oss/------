/**
 * @file    avl.h
 * @brief   双向平衡二叉搜索树（AVL Tree）接口声明 —— 校园选课记录检索与大数据分析系统
 *
 * 本模块实现以 CourseRecord.key（学号_课程号）为键的 AVL 树，
 * 支持 O(log n) 的精确增删改查，并提供中序遍历（有序输出）能力。
 *
 * 旋转命名约定（以失衡节点 z 为基准）：
 *   - LL（右旋）  ：z 的左子的左子插入导致失衡
 *   - RR（左旋）  ：z 的右子的右子插入导致失衡
 *   - LR（左-右旋）：z 的左子的右子插入导致失衡
 *   - RL（右-左旋）：z 的右子的左子插入导致失衡
 *
 * @author  （作者姓名 / 学号，待填写）
 * @date    2026-06-23
 * @version 1.0
 */

#ifndef AVL_H
#define AVL_H

#include "record.h"

/* -----------------------------------------------------------------------
 * AVL 树节点结构体
 * ---------------------------------------------------------------------- */

/**
 * @struct AVLNode
 * @brief  AVL 树的单个节点，存储一条选课记录指针及平衡信息。
 *
 * 使用指针而非内联结构体，便于与哈希表共享同一份数据，避免冗余拷贝。
 */
typedef struct AVLNode {
    CourseRecord  *data;        /**< 指向堆上 CourseRecord 的指针（唯一所有权）*/
    int            height;      /**< 以本节点为根的子树高度（叶节点高度 = 1）  */
    struct AVLNode *left;       /**< 左子节点                                    */
    struct AVLNode *right;      /**< 右子节点                                    */
} AVLNode;

/* -----------------------------------------------------------------------
 * 宏：获取节点高度（安全处理 NULL）
 * ---------------------------------------------------------------------- */
#define AVL_HEIGHT(node)  ((node) ? (node)->height : 0)

/**
 * @brief  更新节点高度（根据左右子树高度重新计算）。
 *         仅作内联使用；在旋转函数内部调用。
 */
#define AVL_UPDATE_HEIGHT(node) \
    do { \
        int _lh = AVL_HEIGHT((node)->left); \
        int _rh = AVL_HEIGHT((node)->right); \
        (node)->height = (_lh > _rh ? _lh : _rh) + 1; \
    } while (0)

/* -----------------------------------------------------------------------
 * 初始化 / 销毁
 * ---------------------------------------------------------------------- */

/**
 * @brief  返回一棵空 AVL 树的根（即 NULL）。
 *         AVL 树用 NULL 指针表示空树，无需额外初始化结构。
 * @return NULL（表示空树根）。
 */
AVLNode *avl_create(void);

/**
 * @brief  销毁整棵 AVL 树，释放所有节点及其持有的 CourseRecord 内存。
 * @param  root  树的根节点指针（可为 NULL）。
 */
void avl_destroy(AVLNode *root);

/* -----------------------------------------------------------------------
 * 平衡旋转（内部函数，供 avl.c 实现使用；头文件声明以便单元测试）
 * ---------------------------------------------------------------------- */

/**
 * @brief  LL 型失衡修复（右旋）。
 * @param  z  失衡节点（非 NULL）。
 * @return 旋转后的新子树根节点。
 */
AVLNode *avl_rotate_ll(AVLNode *z);

/**
 * @brief  RR 型失衡修复（左旋）。
 * @param  z  失衡节点（非 NULL）。
 * @return 旋转后的新子树根节点。
 */
AVLNode *avl_rotate_rr(AVLNode *z);

/**
 * @brief  LR 型失衡修复（先左旋左子，再右旋 z）。
 * @param  z  失衡节点（非 NULL）。
 * @return 旋转后的新子树根节点。
 */
AVLNode *avl_rotate_lr(AVLNode *z);

/**
 * @brief  RL 型失衡修复（先右旋右子，再左旋 z）。
 * @param  z  失衡节点（非 NULL）。
 * @return 旋转后的新子树根节点。
 */
AVLNode *avl_rotate_rl(AVLNode *z);

/* -----------------------------------------------------------------------
 * 核心操作接口
 * ---------------------------------------------------------------------- */

/**
 * @brief  向 AVL 树插入一条选课记录（若 key 已存在则拒绝插入）。
 *
 * @param  root    当前树的根节点指针（可为 NULL 表示空树）。
 * @param  record  指向已分配堆内存的 CourseRecord（所有权转移给树）。
 * @return 插入并重新平衡后的新根节点指针。
 *         若 key 重复，返回原根且不修改树，调用者需自行释放 record。
 */
AVLNode *avl_insert(AVLNode *root, CourseRecord *record);

/**
 * @brief  从 AVL 树中删除指定 key 的节点，并释放对应的 CourseRecord 内存。
 *
 * @param  root  当前树的根节点指针。
 * @param  key   要删除的唯一主键字符串（格式：学号_课程号）。
 * @return 删除并重新平衡后的新根节点指针（若树变空则返回 NULL）。
 */
AVLNode *avl_delete(AVLNode *root, const char *key);

/**
 * @brief  在 AVL 树中按 key 精确查找一条记录（O(log n)）。
 *
 * @param  root  当前树的根节点指针。
 * @param  key   要查找的唯一主键字符串。
 * @return 找到时返回对应 AVLNode 指针；未找到返回 NULL。
 */
AVLNode *avl_search(AVLNode *root, const char *key);

/**
 * @brief  中序遍历 AVL 树，将所有记录指针按 key 升序填入数组。
 *
 * @param  root    当前树的根节点指针。
 * @param  out     输出数组，调用者需预先分配足够空间（至少 count 个元素）。
 * @param  idx     当前填充索引（初始调用时传入 0 的地址）。
 */
void avl_inorder(AVLNode *root, CourseRecord **out, int *idx);

/**
 * @brief  返回 AVL 树中节点总数（O(n)）。
 * @param  root  当前树的根节点指针。
 * @return 节点数量。
 */
int avl_count(AVLNode *root);

/**
 * @brief  获取平衡因子（左子高度 - 右子高度）。
 * @param  node  树节点指针（可为 NULL，返回 0）。
 * @return 平衡因子值。
 */
int avl_balance_factor(AVLNode *node);

#endif /* AVL_H */
