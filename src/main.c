/**
 * @file    main.c
 * @brief   程序入口与 CLI 菜单交互 —— 校园选课记录检索与大数据分析系统
 *
 * 负责：
 *   - 初始化 AVL 树和哈希表双索引
 *   - 从 JSON Lines 文件加载持久化数据
 *   - 渲染多级 CLI 菜单，调度各子模块功能
 *   - 退出前将内存数据保存到文件并释放所有资源
 *
 * @author  鹿智明
 * @date    2026-06-25
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/record.h"
#include "../include/avl.h"
#include "../include/hash.h"
#include "../include/util.h"

/* -----------------------------------------------------------------------
 * 数据文件路径
 * ---------------------------------------------------------------------- */
#define DATA_FILE  "data/records.jsonl"

/* -----------------------------------------------------------------------
 * 全局双索引（main.c 局部，不对外暴露）
 * ---------------------------------------------------------------------- */
static AVLNode  *g_avl_root = NULL;
static HashTable g_hash_table;

/* -----------------------------------------------------------------------
 * 前向声明（菜单函数）
 * ---------------------------------------------------------------------- */
static void menu_main(void);
static void menu_insert(void);
static void menu_delete(void);
static void menu_search(void);
static void menu_update(void);
static void menu_list(void);
static void menu_stats(void);

/* -----------------------------------------------------------------------
 * 统一输入函数：全部基于 fgets，避免 scanf/fgets 混用导致缓冲区错位
 * ---------------------------------------------------------------------- */

/**
 * @brief  读取一行字符串（去除首尾空白，包括 \r\n）
 */
/**
 * @brief  读取一行字符串到 buf（去除首尾空白，包括 \r\n）
 * @note   如果一行内容超过 buf_size-1 个字符，剩余部分会被自动消耗，
 *         避免污染下一次读取。
 */
static void read_line(char *buf, int size) {
    if (!fgets(buf, size, stdin)) {
        buf[0] = '\0';
        return;
    }

    /* 检查 fgets 是否读到了完整的一行（以 \n 结尾） */
    int len = (int)strlen(buf);
    int has_newline = (len > 0 && buf[len - 1] == '\n');

    /* 如果没读到换行符，说明该行还有剩余字符在 stdin 中，
       需要消耗掉，避免污染下一次读取 */
    if (!has_newline) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) { }
    }

    /* 去除首尾空白（包括 \r\n） */
    util_trim(buf);
}

/**
 * @brief  读取一行并转为整数
 */
static int read_int_line(void) {
    char buf[64];
    read_line(buf, sizeof(buf));
    return atoi(buf);
}

/**
 * @brief  读取一行并转为浮点数
 */
static float read_float_line(void) {
    char buf[64];
    read_line(buf, sizeof(buf));
    return (float)atof(buf);
}

/* -----------------------------------------------------------------------
 * 辅助：分页打印记录数组
 * ---------------------------------------------------------------------- */
static void print_records_paged(CourseRecord **arr, int n) {
    if (n <= 0) {
        printf("  （无记录）\n");
        return;
    }

    const int page_size = 10;
    int shown = 0;

    while (shown < n) {
        int end = shown + page_size;
        if (end > n) end = n;

        for (int i = shown; i < end; i++) {
            printf("  [%d] %-13s | %-6s | %-10s | %-8s | 学分%.1f | %s | 成绩",
                   i + 1,
                   arr[i]->student_id,
                   arr[i]->name,
                   arr[i]->course_id,
                   arr[i]->course_name,
                   arr[i]->credit,
                   arr[i]->semester);
            if (arr[i]->score >= 0)
                printf("%d\n", arr[i]->score);
            else
                printf("（未录入）\n");
        }

        shown = end;

        if (shown < n) {
            printf("\n  --- 第 %d-%d 条 / 共 %d 条 ---\n", shown - page_size + 1, shown, n);
            printf("  按回车继续，输入 q 返回：");
            char buf[8];
            read_line(buf, sizeof(buf));
            if (buf[0] == 'q' || buf[0] == 'Q') break;
            printf("\n");
        } else {
            printf("\n  --- 共 %d 条记录 ---\n\n", n);
        }
    }
}

/* -----------------------------------------------------------------------
 * main()
 * ---------------------------------------------------------------------- */
int main(void) {
    /* 1. 初始化哈希表 */
    if (hash_init(&g_hash_table, 0) != 0) {
        fprintf(stderr, "[ERROR] 哈希表初始化失败，程序退出。\n");
        return EXIT_FAILURE;
    }

    /* 2. 加载持久化数据，同时构建双索引 */
    int loaded = load_from_json(DATA_FILE, &g_hash_table, &g_avl_root);
    if (loaded < 0) {
        fprintf(stderr, "[WARN]  数据文件解析出错，将以空库启动。\n");
    } else {
        printf("[INFO]  成功加载 %d 条选课记录。\n", loaded);
    }

    /* 3. 进入主菜单循环 */
    menu_main();

    /* 4. 退出前保存数据并释放资源 */
    if (save_to_json(DATA_FILE, g_avl_root) != 0) {
        fprintf(stderr, "[ERROR] 数据保存失败，请检查 data/ 目录权限。\n");
    }
    hash_destroy(&g_hash_table);
    avl_destroy(g_avl_root);

    printf("[INFO]  程序已退出，数据已保存。\n");
    return EXIT_SUCCESS;
}

/* -----------------------------------------------------------------------
 * 主菜单
 * ---------------------------------------------------------------------- */
static void menu_main(void) {
    char buf[16];
    while (1) {
        printf("\n============================\n");
        printf("  校园选课记录检索与分析系统\n");
        printf("============================\n");
        printf("  1. 新增选课记录\n");
        printf("  2. 删除选课记录\n");
        printf("  3. 精确查询（按主键）\n");
        printf("  4. 修改选课记录\n");
        printf("  5. 列表查询（排序/筛选）\n");
        printf("  6. 统计分析\n");
        printf("  0. 保存并退出\n");
        printf("----------------------------\n");
        printf("请输入选项：");

        read_line(buf, sizeof(buf));

        /* 检测 EOF（管道输入耗尽时优雅退出） */
        if (feof(stdin)) {
            printf("\n[INFO] 检测到输入结束，自动退出。\n");
            return;
        }

        int choice = atoi(buf);
        switch (choice) {
            case 1: menu_insert(); break;
            case 2: menu_delete(); break;
            case 3: menu_search(); break;
            case 4: menu_update(); break;
            case 5: menu_list();   break;
            case 6: menu_stats();  break;
            case 0: return;
            default:
                if (buf[0] != '\0')
                    printf("[WARN] 无效选项，请重新输入。\n");
                break;
        }
    }
}

/* -----------------------------------------------------------------------
 * 子菜单：新增选课记录
 * ---------------------------------------------------------------------- */
static void menu_insert(void) {
    CourseRecord *record = (CourseRecord *)calloc(1, sizeof(CourseRecord));
    if (!record) {
        printf("[ERROR] 内存分配失败，无法新增。\n");
        return;
    }

    printf("\n--- 新增选课记录 ---\n");

    printf("学号（12位）：");
    read_line(record->student_id, STUDENT_ID_LEN);

    printf("姓名：");
    read_line(record->name, NAME_LEN);

    printf("学院：");
    read_line(record->college, COLLEGE_LEN);

    printf("课程编号（8位）：");
    read_line(record->course_id, COURSE_ID_LEN);

    printf("课程名称：");
    read_line(record->course_name, COURSE_NAME_LEN);

    printf("学分：");
    record->credit = read_float_line();

    printf("选课学期（如 2024-2025-1）：");
    read_line(record->semester, SEMESTER_LEN);

    printf("选课日期（如 2024-09-01）：");
    read_line(record->date, DATE_LEN);

    printf("成绩（0-100，-1表示未录入）：");
    record->score = read_int_line();

    /* 生成唯一主键 */
    record_make_key(record);

    /* 先检查哈希表中是否已存在该 key */
    if (hash_search(&g_hash_table, record->key) != NULL) {
        printf("[ERROR] 主键 [%s] 已存在，拒绝重复插入。\n", record->key);
        free(record);
        return;
    }

    /* 插入 AVL 树（获得唯一所有权） */
    g_avl_root = avl_insert(g_avl_root, record);

    /* 插入哈希表（共享指针） */
    if (hash_insert(&g_hash_table, record) != 0) {
        printf("[ERROR] 哈希表插入失败（内存不足）。\n");
        /* AVL 已持有 record，不需要 free */
        return;
    }

    printf("[OK] 新增成功！主键：%s\n", record->key);
}

/* -----------------------------------------------------------------------
 * 子菜单：删除选课记录
 * ---------------------------------------------------------------------- */
static void menu_delete(void) {
    printf("\n--- 删除选课记录 ---\n");

    char sid[STUDENT_ID_LEN];
    char cid[COURSE_ID_LEN];

    printf("学号（12位）：");
    read_line(sid, STUDENT_ID_LEN);

    printf("课程编号（8位）：");
    read_line(cid, COURSE_ID_LEN);

    /* 拼接主键 */
    char key[KEY_LEN];
    snprintf(key, KEY_LEN, "%s_%s", sid, cid);

    /* 先检查是否存在 */
    if (hash_search(&g_hash_table, key) == NULL) {
        printf("[WARN] 未找到主键 [%s] 的记录，无法删除。\n", key);
        return;
    }

    /* 从哈希表删除（只释放 HashNode，不释放 CourseRecord） */
    hash_delete(&g_hash_table, key);

    /* 从 AVL 树删除（释放 CourseRecord 和 AVLNode） */
    g_avl_root = avl_delete(g_avl_root, key);

    printf("[OK] 删除成功！主键：%s\n", key);
}

/* -----------------------------------------------------------------------
 * 子菜单：精确查询（按主键）
 * ---------------------------------------------------------------------- */
static void menu_search(void) {
    printf("\n--- 精确查询 ---\n");

    char sid[STUDENT_ID_LEN];
    char cid[COURSE_ID_LEN];

    printf("学号（12位）：");
    read_line(sid, STUDENT_ID_LEN);

    printf("课程编号（8位）：");
    read_line(cid, COURSE_ID_LEN);

    char key[KEY_LEN];
    snprintf(key, KEY_LEN, "%s_%s", sid, cid);

    /* 优先使用哈希表 O(1) 查找 */
    CourseRecord *record = hash_search(&g_hash_table, key);
    if (record == NULL) {
        printf("[WARN] 未找到主键 [%s] 的记录。\n", key);
        return;
    }

    printf("\n查询结果：\n");
    record_print(record);
}

/* -----------------------------------------------------------------------
 * 子菜单：修改选课记录
 * ---------------------------------------------------------------------- */
static void menu_update(void) {
    printf("\n--- 修改选课记录 ---\n");

    char sid[STUDENT_ID_LEN];
    char cid[COURSE_ID_LEN];

    printf("学号（12位）：");
    read_line(sid, STUDENT_ID_LEN);

    printf("课程编号（8位）：");
    read_line(cid, COURSE_ID_LEN);

    char key[KEY_LEN];
    snprintf(key, KEY_LEN, "%s_%s", sid, cid);

    /* 用哈希表快速定位 */
    CourseRecord *old = hash_search(&g_hash_table, key);
    if (old == NULL) {
        printf("[WARN] 未找到主键 [%s] 的记录，无法修改。\n", key);
        return;
    }

    printf("\n当前记录：\n");
    record_print(old);

    printf("\n--- 输入新值（直接回车保持原值不变）---\n");

    char buf[256];

    printf("姓名 [%s]：", old->name);
    read_line(buf, NAME_LEN);
    if (buf[0] != '\0') util_strncpy(old->name, buf, NAME_LEN);

    printf("学院 [%s]：", old->college);
    read_line(buf, COLLEGE_LEN);
    if (buf[0] != '\0') util_strncpy(old->college, buf, COLLEGE_LEN);

    printf("课程名称 [%s]：", old->course_name);
    read_line(buf, COURSE_NAME_LEN);
    if (buf[0] != '\0') util_strncpy(old->course_name, buf, COURSE_NAME_LEN);

    printf("学分 [%.1f]：", old->credit);
    read_line(buf, sizeof(buf));
    if (buf[0] != '\0') old->credit = (float)atof(buf);

    printf("选课学期 [%s]：", old->semester);
    read_line(buf, SEMESTER_LEN);
    if (buf[0] != '\0') util_strncpy(old->semester, buf, SEMESTER_LEN);

    printf("选课日期 [%s]：", old->date);
    read_line(buf, DATE_LEN);
    if (buf[0] != '\0') util_strncpy(old->date, buf, DATE_LEN);

    printf("成绩 [%d]：", old->score);
    read_line(buf, sizeof(buf));
    if (buf[0] != '\0') old->score = atoi(buf);

    /* 学号和课程编号不可修改（修改主键会破坏索引一致性） */
    /* 主键未变，AVL 树和哈希表中的指针仍然指向同一块内存，无需更新索引 */

    printf("\n[OK] 修改成功！更新后的记录：\n");
    record_print(old);
}

/* -----------------------------------------------------------------------
 * 子菜单：列表查询（排序/筛选）
 * ---------------------------------------------------------------------- */
static void menu_list(void) {
    printf("\n--- 列表查询 ---\n");
    printf("  1. 全部记录（按主键排序）\n");
    printf("  2. 按条件筛选\n");
    printf("  0. 返回\n");
    printf("请选择：");

    int sub = read_int_line();
    if (sub == 0) return;

    /* 收集 AVL 树中的所有记录 */
    int total = avl_count(g_avl_root);
    if (total <= 0) {
        printf("[INFO] 当前无任何记录。\n");
        return;
    }

    CourseRecord **all = (CourseRecord **)malloc((size_t)total * sizeof(CourseRecord *));
    if (!all) {
        printf("[ERROR] 内存分配失败。\n");
        return;
    }

    int idx = 0;
    avl_inorder(g_avl_root, all, &idx);

    int result_count = total;

    /* 如果选择筛选 */
    if (sub == 2) {
        FilterCriteria fc;
        memset(&fc, 0, sizeof(fc));
        fc.score_min  = -1;
        fc.score_max  = -1;
        fc.credit_min = -1.0f;

        char buf[256];

        printf("\n--- 筛选条件（直接回车跳过该条件）---\n");

        printf("学号前缀：");
        read_line(buf, STUDENT_ID_LEN);
        if (buf[0] != '\0') util_strncpy(fc.student_id, buf, STUDENT_ID_LEN);

        printf("学院名称：");
        read_line(buf, COLLEGE_LEN);
        if (buf[0] != '\0') util_strncpy(fc.college, buf, COLLEGE_LEN);

        printf("课程编号：");
        read_line(buf, COURSE_ID_LEN);
        if (buf[0] != '\0') util_strncpy(fc.course_id, buf, COURSE_ID_LEN);

        printf("学期（如 2024-2025-1）：");
        read_line(buf, SEMESTER_LEN);
        if (buf[0] != '\0') util_strncpy(fc.semester, buf, SEMESTER_LEN);

        printf("成绩下限（-1 不限）：");
        read_line(buf, sizeof(buf));
        if (buf[0] != '\0') fc.score_min = atoi(buf);

        printf("成绩上限（-1 不限）：");
        read_line(buf, sizeof(buf));
        if (buf[0] != '\0') fc.score_max = atoi(buf);

        printf("学分下限（-1 不限）：");
        read_line(buf, sizeof(buf));
        if (buf[0] != '\0') fc.credit_min = (float)atof(buf);

        /* 分配输出数组 */
        CourseRecord **filtered = (CourseRecord **)malloc((size_t)total * sizeof(CourseRecord *));
        if (!filtered) {
            printf("[ERROR] 内存分配失败。\n");
            free(all);
            return;
        }

        result_count = filter_records(all, total, filtered, &fc);

        /* 选择排序方式 */
        printf("\n--- 排序方式 ---\n");
        printf("  0. 按主键（默认）     1. 按学号        2. 按课程编号\n");
        printf("  3. 按成绩升序          4. 按成绩降序     5. 按学分升序\n");
        printf("  6. 按学期              7. 按日期\n");
        printf("请选择排序方式：");

        int sort_choice = read_int_line();
        if (sort_choice >= 0 && sort_choice <= 7) {
            sort_records(filtered, result_count, (SortKey)sort_choice);
        }

        print_records_paged(filtered, result_count);

        free(filtered);
    } else {
        /* 全部记录，选择排序方式 */
        printf("\n--- 排序方式 ---\n");
        printf("  0. 按主键（默认）     1. 按学号        2. 按课程编号\n");
        printf("  3. 按成绩升序          4. 按成绩降序     5. 按学分升序\n");
        printf("  6. 按学期              7. 按日期\n");
        printf("请选择排序方式：");

        int sort_choice = read_int_line();
        if (sort_choice >= 0 && sort_choice <= 7) {
            sort_records(all, result_count, (SortKey)sort_choice);
        }

        print_records_paged(all, result_count);
    }

    free(all);
}

/* -----------------------------------------------------------------------
 * 子菜单：统计分析
 * ---------------------------------------------------------------------- */
static void menu_stats(void) {
    printf("\n--- 统计分析 ---\n");

    int total = avl_count(g_avl_root);
    if (total <= 0) {
        printf("[INFO] 当前无任何记录，无法统计。\n");
        return;
    }

    /* 收集所有记录 */
    CourseRecord **all = (CourseRecord **)malloc((size_t)total * sizeof(CourseRecord *));
    if (!all) {
        printf("[ERROR] 内存分配失败。\n");
        return;
    }

    int idx = 0;
    avl_inorder(g_avl_root, all, &idx);

    /* 成绩分布 */
    int dist[5];
    stat_score_distribution(all, total, dist);

    int valid_scores = dist[0] + dist[1] + dist[2] + dist[3] + dist[4];

    printf("\n========== 成绩分布统计 ==========\n");
    printf("  总记录数：     %d\n", total);
    printf("  已录入成绩：   %d\n", valid_scores);
    printf("  未录入成绩：   %d\n", total - valid_scores);
    printf("  --------------------------------\n");
    printf("  不及格 [0,60) : %d 人\n", dist[0]);
    printf("  及格  [60,70) : %d 人\n", dist[1]);
    printf("  中等  [70,80) : %d 人\n", dist[2]);
    printf("  良好  [80,90) : %d 人\n", dist[3]);
    printf("  优秀  [90,100]: %d 人\n", dist[4]);
    printf("  --------------------------------\n");

    /* 平均成绩 */
    float avg = stat_average_score(all, total);
    if (avg < 0) {
        printf("  平均成绩：     （无有效成绩）\n");
    } else {
        printf("  平均成绩：     %.2f\n", avg);
    }

    printf("==================================\n");

    /* 哈希表统计 */
    hash_print_stats(&g_hash_table);

    free(all);
}
