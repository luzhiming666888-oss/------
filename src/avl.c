/**
 * @file    avl.c
 * @brief   双向平衡二叉搜索树（AVL Tree）实现 —— 校园选课记录检索与大数据分析系统
 *
 * 实现要点：
 *   - 以 CourseRecord.key（学号_课程号）作为 BST 排序键，使用 strcmp 比较。
 *   - 每次 insert / delete 操作后，沿回溯路径检查平衡因子并执行旋转修复。
 *   - 删除采用"前驱替换"策略（用左子树最大节点的 data 替换，再删除前驱节点）。
 *   - 所有节点内存（AVLNode + CourseRecord）均通过 malloc/free 管理。
 *
 * @author  鹿智明
 * @date    2026-06-25
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/avl.h"
#include "../include/record.h"

/* -----------------------------------------------------------------------
 * 内部辅助：创建新节点
 * ---------------------------------------------------------------------- */

/**
 * @brief   分配并初始化一个 AVL 树叶节点
 * @details 新节点高度初始化为 1（叶节点），左右子指针为 NULL，
 *          data 指针接管调用者传入的 CourseRecord 堆内存所有权。
 */
static AVLNode *avl_new_node(CourseRecord *record) {
    AVLNode *node = (AVLNode *)malloc(sizeof(AVLNode));
    if (node == NULL) {
        return NULL; /* 内存分配失败 */
    }
    node->data   = record;
    node->height = 1; /* 叶节点高度 = 1 */
    node->left   = NULL;
    node->right  = NULL;
    return node;
}

/* -----------------------------------------------------------------------
 * 初始化 / 销毁
 * ---------------------------------------------------------------------- */

AVLNode *avl_create(void) {
    return NULL; /* 空树即 NULL 根 */
}

/**
 * @brief   后序遍历销毁整棵 AVL 树
 * @details 先递归释放左右子树，再释放当前节点持有的 CourseRecord 内存，
 *          最后释放 AVLNode 节点本身。保证不会出现内存泄漏。
 */
void avl_destroy(AVLNode *root) {
    if (root == NULL) return;

    avl_destroy(root->left);   /* 递归销毁左子树 */
    avl_destroy(root->right);  /* 递归销毁右子树 */

    free(root->data); /* 释放 CourseRecord 堆内存（唯一所有权） */
    free(root);       /* 释放 AVLNode 节点本身 */
}

/* -----------------------------------------------------------------------
 * 平衡因子
 * ---------------------------------------------------------------------- */

int avl_balance_factor(AVLNode *node) {
    if (!node) return 0;
    return AVL_HEIGHT(node->left) - AVL_HEIGHT(node->right);
}

/* -----------------------------------------------------------------------
 * 旋转操作
 * ---------------------------------------------------------------------- */

/**
 * @brief   LL 型失衡修复 —— 右旋
 * @details z 的左子树的左子树插入导致失衡。
 *          将 z 的左子 l 提升为新子树根，z 成为 l 的右子。
 *
 * 旋转前:        z               旋转后:      l
 *               / \                         /   \
 *              l   T4                     T1     z
 *             / \                                / \
 *            T1  T2                            T2  T4
 */
AVLNode *avl_rotate_ll(AVLNode *z) {
    AVLNode *l = z->left;     /* l 为 z 的左子节点，旋转后成为新根 */

    z->left   = l->right;     /* l 的右子树 T2 挂到 z 的左侧 */
    l->right  = z;            /* z 成为 l 的右子 */

    AVL_UPDATE_HEIGHT(z);     /* 先更新下层节点 z 的高度 */
    AVL_UPDATE_HEIGHT(l);     /* 再更新新根 l 的高度 */

    return l;                 /* 返回新子树根 */
}

/**
 * @brief   RR 型失衡修复 —— 左旋
 * @details z 的右子树的右子树插入导致失衡。
 *          将 z 的右子 r 提升为新子树根，z 成为 r 的左子。
 *
 * 旋转前:    z                 旋转后:      r
 *           / \                           /   \
 *          T1  r                         z     T4
 *             / \                       / \
 *            T2  T3                    T1  T2
 */
AVLNode *avl_rotate_rr(AVLNode *z) {
    AVLNode *r = z->right;    /* r 为 z 的右子节点，旋转后成为新根 */

    z->right  = r->left;      /* r 的左子树 T2 挂到 z 的右侧 */
    r->left   = z;            /* z 成为 r 的左子 */

    AVL_UPDATE_HEIGHT(z);     /* 先更新下层节点 z 的高度 */
    AVL_UPDATE_HEIGHT(r);     /* 再更新新根 r 的高度 */

    return r;                 /* 返回新子树根 */
}

/**
 * @brief   LR 型失衡修复 —— 先左旋左子，再右旋 z
 * @details z 的左子树的右子树插入导致失衡。
 *          先对 z->left 执行 RR 左旋（变为 LL 型），再对 z 执行 LL 右旋。
 */
AVLNode *avl_rotate_lr(AVLNode *z) {
    z->left = avl_rotate_rr(z->left); /* 先对左子做左旋，转化为 LL 型 */
    return avl_rotate_ll(z);          /* 再对 z 做右旋完成修复 */
}

/**
 * @brief   RL 型失衡修复 —— 先右旋右子，再左旋 z
 * @details z 的右子树的左子树插入导致失衡。
 *          先对 z->right 执行 LL 右旋（变为 RR 型），再对 z 执行 RR 左旋。
 */
AVLNode *avl_rotate_rl(AVLNode *z) {
    z->right = avl_rotate_ll(z->right); /* 先对右子做右旋，转化为 RR 型 */
    return avl_rotate_rr(z);            /* 再对 z 做左旋完成修复 */
}

/* -----------------------------------------------------------------------
 * 内部：平衡修复（供 insert / delete 回溯时调用）
 * ---------------------------------------------------------------------- */

/**
 * @brief   检查节点平衡因子，若 |BF| = 2 则执行对应旋转
 * @param   node  当前需检查的节点（调用前高度已更新）
 * @return  旋转后的新子树根；若无需旋转则返回原 node
 */
static AVLNode *avl_rebalance(AVLNode *node) {
    int bf = avl_balance_factor(node);

    if (bf > 1) {
        /* 左子树偏高（BF = +2）*/
        if (avl_balance_factor(node->left) >= 0) {
            /* 左子的左子树偏高 → LL 型 */
            return avl_rotate_ll(node);
        } else {
            /* 左子的右子树偏高 → LR 型 */
            return avl_rotate_lr(node);
        }
    }

    if (bf < -1) {
        /* 右子树偏高（BF = -2）*/
        if (avl_balance_factor(node->right) <= 0) {
            /* 右子的右子树偏高 → RR 型 */
            return avl_rotate_rr(node);
        } else {
            /* 右子的左子树偏高 → RL 型 */
            return avl_rotate_rl(node);
        }
    }

    return node; /* |BF| <= 1，无需旋转 */
}

/* -----------------------------------------------------------------------
 * 插入
 * ---------------------------------------------------------------------- */

/**
 * @brief   递归 BST 插入 + 回溯平衡修复
 * @details 按字典序递归定位插入位置，创建新叶节点后，
 *          沿回溯路径逐层更新高度并执行 rebalance。
 *          若 key 已存在则拒绝插入（返回原根）。
 */
AVLNode *avl_insert(AVLNode *root, CourseRecord *record) {
    /* 1. 递归到空位 → 创建新叶节点 */
    if (root == NULL) {
        return avl_new_node(record);
    }

    /* 2. 按 BST 规则递归向下 */
    int cmp = strcmp(record->key, root->data->key);
    if (cmp < 0) {
        root->left = avl_insert(root->left, record);
    } else if (cmp > 0) {
        root->right = avl_insert(root->right, record);
    } else {
        /* key 已存在，拒绝重复插入 */
        return root;
    }

    /* 3. 回溯阶段：更新高度 + 平衡修复 */
    AVL_UPDATE_HEIGHT(root);
    return avl_rebalance(root);
}

/* -----------------------------------------------------------------------
 * 删除
 * ---------------------------------------------------------------------- */

/**
 * @brief   递归 BST 删除 + 回溯平衡修复
 * @details 采用"前驱替换"策略处理双子节点删除：
 *          用左子树最大节点的 data 替换目标节点的 data，
 *          然后递归删除前驱节点（此时前驱至多有一个右子）。
 *          沿回溯路径逐层更新高度并执行 rebalance。
 *
 * 内存安全说明：
 *   前驱节点的 data 指针转移给目标节点后，将前驱的 data 置 NULL，
 *   防止递归删除前驱时 free(NULL) 之外的双重释放。
 *   （C 标准保证 free(NULL) 是安全的空操作）
 */
AVLNode *avl_delete(AVLNode *root, const char *key) {
    if (root == NULL) {
        return NULL; /* 空树，无需删除 */
    }

    int cmp = strcmp(key, root->data->key);
    if (cmp < 0) {
        /* 目标在左子树 */
        root->left = avl_delete(root->left, key);
    } else if (cmp > 0) {
        /* 目标在右子树 */
        root->right = avl_delete(root->right, key);
    } else {
        /* 找到目标节点 */
        if (root->left == NULL || root->right == NULL) {
            /* 至多一个子节点：用子节点替换当前节点 */
            AVLNode *child = root->left ? root->left : root->right;

            free(root->data); /* 释放当前节点的 CourseRecord */
            free(root);       /* 释放当前 AVLNode */

            return child;     /* child 可能为 NULL（叶子节点删除后变空） */
        } else {
            /* 两个子节点：前驱替换策略 */
            /* 1. 找左子树中的最大节点（前驱） */
            AVLNode *predecessor = root->left;
            while (predecessor->right != NULL) {
                predecessor = predecessor->right;
            }

            /* 2. 保存前驱的 key（因为 data 转移后无法再读取） */
            char tmp_key[KEY_LEN];
            strcpy(tmp_key, predecessor->data->key);

            /* 3. 释放当前节点的旧 data，用前驱的 data 替换 */
            free(root->data);
            root->data = predecessor->data;

            /* 4. 前驱的 data 置 NULL，防止后续递归删除时双重释放 */
            predecessor->data = NULL;

            /* 5. 递归删除前驱节点（此时前驱 data == NULL，至多有一个左子） */
            root->left = avl_delete(root->left, tmp_key);
        }
    }

    /* 回溯阶段：更新高度 + 平衡修复 */
    AVL_UPDATE_HEIGHT(root);
    return avl_rebalance(root);
}

/* -----------------------------------------------------------------------
 * 精确查找
 * ---------------------------------------------------------------------- */

/**
 * @brief   迭代 BST 查找（避免递归栈开销）
 * @details 从根节点出发，按 strcmp 结果决定走向左/右子树，
 *          最坏 O(log n) 次比较即可定位或确认不存在。
 */
AVLNode *avl_search(AVLNode *root, const char *key) {
    while (root != NULL) {
        int cmp = strcmp(key, root->data->key);
        if (cmp == 0) {
            return root;      /* 命中 */
        } else if (cmp < 0) {
            root = root->left;  /* key 小于当前节点，走向左子树 */
        } else {
            root = root->right; /* key 大于当前节点，走向右子树 */
        }
    }
    return NULL; /* 未找到 */
}

/* -----------------------------------------------------------------------
 * 中序遍历（有序输出到数组）
 * ---------------------------------------------------------------------- */

/**
 * @brief   递归中序遍历，按 key 升序填充输出数组
 * @details 中序遍历顺序：左子树 → 当前节点 → 右子树，
 *          保证输出数组按 key 字典序严格升序排列。
 */
void avl_inorder(AVLNode *root, CourseRecord **out, int *idx) {
    if (root == NULL) {
        return;
    }

    avl_inorder(root->left, out, idx);  /* 先遍历左子树 */
    out[(*idx)++] = root->data;         /* 访问当前节点 */
    avl_inorder(root->right, out, idx); /* 再遍历右子树 */
}

/* -----------------------------------------------------------------------
 * 节点计数
 * ---------------------------------------------------------------------- */

int avl_count(AVLNode *root) {
    if (!root) return 0;
    return 1 + avl_count(root->left) + avl_count(root->right);
}
