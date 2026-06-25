/**
 * @file    util.c
 * @brief   文件 I/O、JSON 序列化/反序列化、排序与统计实现
 *          —— 校园选课记录检索与大数据分析系统
 *
 * 实现要点：
 *   - save_to_json: 中序遍历 AVL 树，逐行序列化为 JSON Lines 写入文件。
 *   - load_from_json: 按行读取，手写解析 JSON 键值对（不依赖外部库），
 *     同步插入 AVL 树和哈希表双索引。
 *   - sort_records: 手写三路快速排序，通过 SortKey 枚举支持多关键字。
 *   - filter_records: 按 FilterCriteria 多字段联合筛选。
 *   - 统计函数：成绩分布、均值计算。
 *
 * @author  鹿智明
 * @date    2026-06-25
 * @version 1.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/util.h"
#include "../include/record.h"
#include "../include/avl.h"
#include "../include/hash.h"

/* -----------------------------------------------------------------------
 * 内部：JSON 序列化单条记录
 * ---------------------------------------------------------------------- */

int record_to_json(const CourseRecord *record, char *buf, int buf_sz) {
    if (!record || !buf || buf_sz <= 0) return -1;

    /* 用 snprintf 一次性拼装完整 JSON 对象行 */
    int n = snprintf(buf, (size_t)buf_sz,
        "{\"sid\":\"%s\",\"name\":\"%s\",\"college\":\"%s\","
        "\"cid\":\"%s\",\"cname\":\"%s\",\"credit\":%.1f,"
        "\"semester\":\"%s\",\"date\":\"%s\",\"score\":%d}",
        record->student_id,
        record->name,
        record->college,
        record->course_id,
        record->course_name,
        record->credit,
        record->semester,
        record->date,
        record->score);

    /* snprintf 返回值 >= buf_sz 表示缓冲区不足（被截断） */
    if (n < 0 || n >= buf_sz) return -1;
    return n; /* 实际写入字节数（不含 '\0'） */
}

/* -----------------------------------------------------------------------
 * 内部辅助：从 JSON 字符串中提取 "key":"value" 或 "key":number
 * ---------------------------------------------------------------------- */

/**
 * @brief  在 json 字符串中查找 "field" 后面的值，写入 out_buf
 * @return 0 成功；-1 未找到该字段
 *
 * 支持的值类型：字符串 ("...") 和整数/浮点数（裸数字）
 * 字符串值中的转义引号 \" 不做特殊处理（测试数据不含转义）
 */
static int json_extract(const char *json, const char *field,
                        char *out_buf, int out_sz) {
    /* 构造搜索模式："field" */
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", field);

    const char *p = strstr(json, pattern);
    if (!p) return -1;

    /* 跳过 "field" 部分 */
    p += strlen(pattern);

    /* 跳过空白和冒号 */
    while (*p && (isspace((unsigned char)*p) || *p == ':')) p++;
    if (!*p) return -1;

    if (*p == '"') {
        /* 字符串值：提取到下一个未转义的 '"' */
        p++; /* 跳过开头的 '"' */
        int i = 0;
        while (*p && *p != '"' && i < out_sz - 1) {
            /* 处理简单转义：\" -> " */
            if (*p == '\\' && *(p + 1) == '"') {
                if (i < out_sz - 1) out_buf[i++] = '"';
                p += 2;
            } else {
                out_buf[i++] = *p++;
            }
        }
        out_buf[i] = '\0';
        return 0;
    } else {
        /* 数值类型：提取连续的数字、小数点、负号 */
        int i = 0;
        while (*p && !isspace((unsigned char)*p) && *p != ',' && *p != '}' && i < out_sz - 1) {
            out_buf[i++] = *p++;
        }
        out_buf[i] = '\0';
        return 0;
    }
}

/* -----------------------------------------------------------------------
 * 内部：JSON 反序列化一行为 CourseRecord
 * ---------------------------------------------------------------------- */

int json_to_record(const char *json, CourseRecord *out) {
    if (!json || !out) return -1;

    /* 清零结构体，确保没有残留数据 */
    memset(out, 0, sizeof(CourseRecord));

    char buf[256]; /* 临时缓冲区，足够容纳任何单个字段值 */

    /* 提取各字段（字段名与 record_to_json 中的 key 保持一致） */
    if (json_extract(json, "sid", buf, sizeof(buf)) != 0) return -1;
    util_strncpy(out->student_id, buf, STUDENT_ID_LEN);

    if (json_extract(json, "name", buf, sizeof(buf)) != 0) return -1;
    util_strncpy(out->name, buf, NAME_LEN);

    if (json_extract(json, "college", buf, sizeof(buf)) != 0) return -1;
    util_strncpy(out->college, buf, COLLEGE_LEN);

    if (json_extract(json, "cid", buf, sizeof(buf)) != 0) return -1;
    util_strncpy(out->course_id, buf, COURSE_ID_LEN);

    if (json_extract(json, "cname", buf, sizeof(buf)) != 0) return -1;
    util_strncpy(out->course_name, buf, COURSE_NAME_LEN);

    if (json_extract(json, "credit", buf, sizeof(buf)) != 0) return -1;
    out->credit = (float)atof(buf);

    if (json_extract(json, "semester", buf, sizeof(buf)) != 0) return -1;
    util_strncpy(out->semester, buf, SEMESTER_LEN);

    if (json_extract(json, "date", buf, sizeof(buf)) != 0) return -1;
    util_strncpy(out->date, buf, DATE_LEN);

    if (json_extract(json, "score", buf, sizeof(buf)) != 0) return -1;
    out->score = atoi(buf);

    /* 自动生成唯一主键 */
    record_make_key(out);

    return 0;
}

/* -----------------------------------------------------------------------
 * record.h 中声明的辅助函数实现
 * ---------------------------------------------------------------------- */

void record_make_key(CourseRecord *record) {
    /* 格式：学号_课程号 */
    snprintf(record->key, KEY_LEN, "%s_%s", record->student_id, record->course_id);
}

void record_print(const CourseRecord *record) {
    printf("+-----------------------+------------------------------+\n");
    printf("| %-21s | %-28s |\n", "字段", "值");
    printf("+-----------------------+------------------------------+\n");
    printf("| %-21s | %-28s |\n", "唯一主键",   record->key);
    printf("| %-21s | %-28s |\n", "学号",       record->student_id);
    printf("| %-21s | %-28s |\n", "姓名",       record->name);
    printf("| %-21s | %-28s |\n", "学院",       record->college);
    printf("| %-21s | %-28s |\n", "课程编号",   record->course_id);
    printf("| %-21s | %-28s |\n", "课程名称",   record->course_name);
    printf("| %-21s | %-28.1f |\n", "学分",     record->credit);
    printf("| %-21s | %-28s |\n", "选课学期",   record->semester);
    printf("| %-21s | %-28s |\n", "选课日期",   record->date);
    printf("| %-21s | %-28d |\n", "成绩",       record->score);
    printf("+-----------------------+------------------------------+\n");
}

int record_compare_key(const CourseRecord *a, const CourseRecord *b) {
    return strcmp(a->key, b->key);
}

/* -----------------------------------------------------------------------
 * 文件 I/O
 * ---------------------------------------------------------------------- */

int save_to_json(const char *filename, AVLNode *root) {
    if (!filename) return -1;

    FILE *fp = fopen(filename, "w");
    if (!fp) return -1;

    /* 中序遍历收集所有记录指针 */
    int count = avl_count(root);
    if (count > 0) {
        /* 分分指针数组 */
        CourseRecord **arr = (CourseRecord **)malloc((size_t)count * sizeof(CourseRecord *));
        if (!arr) {
            fclose(fp);
            return -1;
        }

        int idx = 0;
        avl_inorder(root, arr, &idx);

        /* 逐行序列化写入文件 */
        char line[512];
        for (int i = 0; i < count; i++) {
            int len = record_to_json(arr[i], line, sizeof(line));
            if (len < 0) {
                /* 单条序列化失败，跳过（不中断整体写入） */
                continue;
            }
            fputs(line, fp);
            fputc('\n', fp);
        }

        free(arr);
    }

    fclose(fp);
    return 0;
}

int load_from_json(const char *filename, HashTable *ht, AVLNode **root) {
    if (!filename || !ht || !root) return -1;

    FILE *fp = fopen(filename, "r");
    if (!fp) return 0; /* 文件不存在视为空数据，返回 0 */

    char line[512];
    int loaded = 0;

    while (fgets(line, sizeof(line), fp)) {
        /* 去除行尾换行符 */
        util_trim(line);

        /* 跳过空行和注释行 */
        if (line[0] == '\0' || line[0] == '#') continue;

        /* 分配 CourseRecord 堆内存 */
        CourseRecord *record = (CourseRecord *)calloc(1, sizeof(CourseRecord));
        if (!record) {
            fclose(fp);
            return -1;
        }

        /* 解析 JSON 行 */
        if (json_to_record(line, record) != 0) {
            free(record);
            continue; /* 跳过格式错误的行 */
        }

        /* 先插入 AVL 树（获得唯一所有权） */
        *root = avl_insert(*root, record);

        /* 检查 AVL 树是否接受了该记录（key 重复时 avl_insert 返回原根） */
        /* 通过哈希表插入来同步；如果哈希表报告 key 重复，说明 AVL 也拒绝了 */
        int hash_ret = hash_insert(ht, record);
        if (hash_ret == 1) {
            /* key 已存在，AVL 树也未接受，需要释放这条记录 */
            /* 但 AVL 可能已经接受了（如果哈希表先插入的话）……
               实际上 avl_insert 重复 key 时返回原根且不修改树，
               所以 record 没有被 AVL 持有，需要我们释放 */
            free(record);
        } else if (hash_ret == 0) {
            loaded++;
        } else {
            /* hash_insert 返回 -1（内存分配失败） */
            free(record);
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return loaded;
}

/* -----------------------------------------------------------------------
 * 排序（三路快速排序）
 * ---------------------------------------------------------------------- */

/**
 * @brief  根据 SortKey 比较两条记录的大小
 * @return 负数 a<b；0 a==b；正数 a>b
 */
static int compare_records(const CourseRecord *a, const CourseRecord *b, SortKey key) {
    switch (key) {
        case SORT_BY_KEY:
            return strcmp(a->key, b->key);

        case SORT_BY_STUDENT_ID:
            /* 先按学号，学号相同再按课程号（保证稳定排序） */
            {
                int cmp = strcmp(a->student_id, b->student_id);
                return cmp != 0 ? cmp : strcmp(a->course_id, b->course_id);
            }

        case SORT_BY_COURSE_ID:
            {
                int cmp = strcmp(a->course_id, b->course_id);
                return cmp != 0 ? cmp : strcmp(a->student_id, b->student_id);
            }

        case SORT_BY_SCORE_ASC:
            /* 成绩升序；相同成绩按 key 排序 */
            {
                int diff = a->score - b->score;
                return diff != 0 ? diff : strcmp(a->key, b->key);
            }

        case SORT_BY_SCORE_DESC:
            /* 成绩降序；相同成绩按 key 排序 */
            {
                int diff = b->score - a->score;
                return diff != 0 ? diff : strcmp(a->key, b->key);
            }

        case SORT_BY_CREDIT_ASC:
            {
                float diff = a->credit - b->credit;
                if (diff < 0) return -1;
                if (diff > 0) return 1;
                return strcmp(a->key, b->key);
            }

        case SORT_BY_SEMESTER:
            {
                int cmp = strcmp(a->semester, b->semester);
                return cmp != 0 ? cmp : strcmp(a->key, b->key);
            }

        case SORT_BY_DATE:
            {
                int cmp = strcmp(a->date, b->date);
                return cmp != 0 ? cmp : strcmp(a->key, b->key);
            }

        default:
            return strcmp(a->key, b->key);
    }
}

/**
 * @brief  三路快速排序递归实现
 * @details 将数组分为 <pivot / ==pivot / >pivot 三段，
 *          只递归排序 < 和 > 两段，对大量重复元素有更好性能。
 */
static void qsort_records(CourseRecord **arr, int lo, int hi, SortKey key) {
    if (lo >= hi) return;

    /* 选取枢轴（取中间元素，避免对已排序数组的最坏退化） */
    int mid = lo + (hi - lo) / 2;
    CourseRecord *pivot = arr[mid];

    /* 三路分区：Dutch National Flag 算法 */
    int lt = lo;     /* arr[lo..lt-1] < pivot */
    int gt = hi;     /* arr[gt+1..hi] > pivot */
    int i   = lo;    /* 扫描指针 */

    while (i <= gt) {
        int cmp = compare_records(arr[i], pivot, key);
        if (cmp < 0) {
            /* 交换 arr[i] 和 arr[lt]，i 和 lt 都前进 */
            CourseRecord *tmp = arr[i];
            arr[i]  = arr[lt];
            arr[lt] = tmp;
            i++;
            lt++;
        } else if (cmp > 0) {
            /* 交换 arr[i] 和 arr[gt]，只 gt 后退（换来的元素还没检查） */
            CourseRecord *tmp = arr[i];
            arr[i]  = arr[gt];
            arr[gt] = tmp;
            gt--;
        } else {
            /* == pivot，只前进 i */
            i++;
        }
    }

    /* 递归排序 < pivot 和 > pivot 的分区 */
    qsort_records(arr, lo, lt - 1, key);
    qsort_records(arr, gt + 1, hi, key);
}

void sort_records(CourseRecord **arr, int n, SortKey key) {
    if (!arr || n <= 1) return;
    qsort_records(arr, 0, n - 1, key);
}

/* -----------------------------------------------------------------------
 * 筛选
 * ---------------------------------------------------------------------- */

int filter_records(CourseRecord **in,  int in_cnt,
                   CourseRecord **out, const FilterCriteria *fc) {
    if (!in || !out || !fc) return 0;

    int out_cnt = 0;

    for (int i = 0; i < in_cnt; i++) {
        CourseRecord *r = in[i];
        if (!r) continue;

        /* 学号：非空则检查前缀匹配（支持模糊查询） */
        if (fc->student_id[0] != '\0') {
            if (strncmp(r->student_id, fc->student_id,
                        strlen(fc->student_id)) != 0)
                continue;
        }

        /* 学院：非空则要求精确匹配 */
        if (fc->college[0] != '\0') {
            if (strcmp(r->college, fc->college) != 0)
                continue;
        }

        /* 课程编号：非空则要求精确匹配 */
        if (fc->course_id[0] != '\0') {
            if (strcmp(r->course_id, fc->course_id) != 0)
                continue;
        }

        /* 学期：非空则要求精确匹配 */
        if (fc->semester[0] != '\0') {
            if (strcmp(r->semester, fc->semester) != 0)
                continue;
        }

        /* 成绩下限：score_min >= 0 时要求 score >= score_min
           （score == -1 的未录入项不满足任何成绩条件） */
        if (fc->score_min >= 0) {
            if (r->score < 0 || r->score < fc->score_min)
                continue;
        }

        /* 成绩上限：score_max >= 0 时要求 score <= score_max */
        if (fc->score_max >= 0) {
            if (r->score < 0 || r->score > fc->score_max)
                continue;
        }

        /* 学分下限：credit_min >= 0 时要求 credit >= credit_min */
        if (fc->credit_min >= 0) {
            if (r->credit < fc->credit_min)
                continue;
        }

        /* 全部条件通过，写入输出数组 */
        out[out_cnt++] = r;
    }

    return out_cnt;
}

/* -----------------------------------------------------------------------
 * 统计分析
 * ---------------------------------------------------------------------- */

void stat_score_distribution(CourseRecord **arr, int n, int dist[5]) {
    if (!arr || !dist) return;

    /* 清零分段计数 */
    for (int i = 0; i < 5; i++) dist[i] = 0;

    for (int i = 0; i < n; i++) {
        if (!arr[i]) continue;

        int score = arr[i]->score;
        if (score < 0) continue; /* 跳过未录入成绩 */

        if (score < 60)
            dist[0]++;           /* 不及格 [0, 60)  */
        else if (score < 70)
            dist[1]++;           /* 及格   [60, 70) */
        else if (score < 80)
            dist[2]++;           /* 中等   [70, 80) */
        else if (score < 90)
            dist[3]++;           /* 良好   [80, 90) */
        else
            dist[4]++;           /* 优秀   [90, 100] */
    }
}

float stat_average_score(CourseRecord **arr, int n) {
    if (!arr || n <= 0) return -1.0f;

    long long sum = 0; /* 使用 long long 防止大数溢出 */
    int valid = 0;

    for (int i = 0; i < n; i++) {
        if (!arr[i]) continue;
        if (arr[i]->score >= 0) {
            sum += arr[i]->score;
            valid++;
        }
    }

    if (valid == 0) return -1.0f;
    return (float)((double)sum / valid);
}

/* -----------------------------------------------------------------------
 * 字符串工具
 * ---------------------------------------------------------------------- */

int util_strncpy(char *dst, const char *src, int size) {
    int len = 0;
    if (!dst || !src || size <= 0) return 0;
    while (len < size - 1 && src[len]) {
        dst[len] = src[len];
        len++;
    }
    dst[len] = '\0';
    /* 返回源字符串总长度（类似 strlcpy） */
    while (src[len]) len++;
    return len;
}

void util_trim(char *s) {
    if (!s) return;
    /* 去除尾部空白 */
    int len = (int)strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    /* 去除头部空白（原地左移） */
    int start = 0;
    while (s[start] && isspace((unsigned char)s[start])) start++;
    if (start > 0) {
        int i = 0;
        while (s[start]) s[i++] = s[start++];
        s[i] = '\0';
    }
}
