#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "record.h"
#include "avl.h"
#include "hash.h"

static int pass = 0, fail = 0;
#define CHECK(cond, msg) do { \
    if (cond) { pass++; printf("  [PASS] %s\n", msg); } \
    else { fail++; printf("  [FAIL] %s\n", msg); } \
} while(0)

static CourseRecord *make_rec(const char *sid, const char *name,
        const char *college, const char *cid, const char *cname,
        float credit, const char *sem, const char *date, int score) {
    CourseRecord *r = (CourseRecord *)calloc(1, sizeof(CourseRecord));
    strncpy(r->student_id, sid, STUDENT_ID_LEN - 1);
    strncpy(r->name, name, NAME_LEN - 1);
    strncpy(r->college, college, COLLEGE_LEN - 1);
    strncpy(r->course_id, cid, COURSE_ID_LEN - 1);
    strncpy(r->course_name, cname, COURSE_NAME_LEN - 1);
    r->credit = credit;
    strncpy(r->semester, sem, SEMESTER_LEN - 1);
    strncpy(r->date, date, DATE_LEN - 1);
    r->score = score;
    record_make_key(r);
    return r;
}

int main(void) {
    printf("=== util.c 功能测试 ===\n\n");

    /* 测试 1: JSON 序列化/反序列化往返 */
    printf("测试1: JSON 序列化/反序列化往返\n");
    {
        CourseRecord *r = make_rec("202401010001", "张三", "计算机学院",
            "CS300101", "数据结构", 3.5f, "2024-2025-1", "2024-09-01", 88);
        char json[512];
        int len = record_to_json(r, json, sizeof(json));
        CHECK(len > 0, "record_to_json 返回正长度");

        CourseRecord r2;
        int ret = json_to_record(json, &r2);
        CHECK(ret == 0, "json_to_record 返回 0");
        CHECK(strcmp(r2.student_id, "202401010001") == 0, "学号一致");
        CHECK(strcmp(r2.name, "张三") == 0, "姓名一致");
        CHECK(strcmp(r2.college, "计算机学院") == 0, "学院一致");
        CHECK(strcmp(r2.course_id, "CS300101") == 0, "课程编号一致");
        CHECK(strcmp(r2.course_name, "数据结构") == 0, "课程名称一致");
        CHECK(r2.credit == 3.5f, "学分一致");
        CHECK(strcmp(r2.semester, "2024-2025-1") == 0, "学期一致");
        CHECK(strcmp(r2.date, "2024-09-01") == 0, "日期一致");
        CHECK(r2.score == 88, "成绩一致");
        CHECK(strcmp(r2.key, "202401010001_CS300101") == 0, "主键自动生成正确");
        free(r);
    }
    printf("\n");

    /* 测试 2: save_to_json / load_from_json 文件往返 */
    printf("测试2: save_to_json / load_from_json 文件往返\n");
    {
        AVLNode *root = avl_create();
        HashTable ht;
        hash_init(&ht, 0);

        CourseRecord *r1 = make_rec("202401010001", "张三", "计算机学院",
            "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 85);
        CourseRecord *r2 = make_rec("202401010002", "李四", "软件学院",
            "CS300102", "操作系统", 4.0f, "2024-2025-1", "2024-09-01", 92);
        CourseRecord *r3 = make_rec("202401010003", "王五", "计算机学院",
            "CS300101", "数据结构", 3.0f, "2024-2025-2", "2025-02-15", 55);
        root = avl_insert(root, r1);
        root = avl_insert(root, r2);
        root = avl_insert(root, r3);
        hash_insert(&ht, r1);
        hash_insert(&ht, r2);
        hash_insert(&ht, r3);

        int sv = save_to_json("/tmp/test_records.jsonl", root);
        CHECK(sv == 0, "save_to_json 返回 0");

        avl_destroy(root);
        hash_destroy(&ht);
        hash_init(&ht, 0);
        root = avl_create();

        int loaded = load_from_json("/tmp/test_records.jsonl", &ht, &root);
        CHECK(loaded == 3, "load_from_json 加载 3 条");
        CHECK(avl_count(root) == 3, "AVL 树节点数 = 3");
        CHECK(ht.size == 3, "哈希表记录数 = 3");

        CourseRecord *found = hash_search(&ht, "202401010002_CS300102");
        CHECK(found != NULL, "哈希表查找 202401010002_CS300102");
        CHECK(found && found->score == 92, "李四成绩 = 92");
        CHECK(found && strcmp(found->name, "李四") == 0, "李四姓名正确");

        avl_destroy(root);
        hash_destroy(&ht);
    }
    printf("\n");

    /* 测试 3: sort_records 多关键字排序 */
    printf("测试3: sort_records 多关键字排序\n");
    {
        CourseRecord *arr[6];
        arr[0] = make_rec("202401010003", "王五", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 55);
        arr[1] = make_rec("202401010001", "张三", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 85);
        arr[2] = make_rec("202401010002", "李四", "软件学院",   "CS300102", "操作系统", 4.0f, "2024-2025-1", "2024-09-01", 92);
        arr[3] = make_rec("202401010005", "赵六", "计算机学院", "CS300103", "计算机网络", 3.0f, "2024-2025-2", "2025-02-15", 78);
        arr[4] = make_rec("202401010004", "钱七", "软件学院",   "CS300102", "操作系统", 4.0f, "2024-2025-1", "2024-09-01", 92);
        arr[5] = make_rec("202401010006", "孙八", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", -1);

        sort_records(arr, 6, SORT_BY_SCORE_ASC);
        CHECK(arr[0]->score == -1 || arr[0]->score <= arr[1]->score, "成绩升序: 第一个 <= 第二个");
        CHECK(arr[5]->score == 92, "成绩升序: 最后一个 = 92");

        sort_records(arr, 6, SORT_BY_SCORE_DESC);
        CHECK(arr[0]->score == 92, "成绩降序: 第一个 = 92");

        sort_records(arr, 6, SORT_BY_STUDENT_ID);
        CHECK(strcmp(arr[0]->student_id, "202401010001") == 0, "学号升序: 第一个 = 202401010001");
        CHECK(strcmp(arr[5]->student_id, "202401010006") == 0, "学号升序: 最后一个 = 202401010006");

        sort_records(arr, 6, SORT_BY_CREDIT_ASC);
        CHECK(arr[0]->credit == 3.0f, "学分升序: 第一个 credit = 3.0");
        CHECK(arr[5]->credit == 4.0f, "学分升序: 最后一个 credit = 4.0");

        for (int i = 0; i < 6; i++) free(arr[i]);
    }
    printf("\n");

    /* 测试 4: filter_records 多字段筛选 */
    printf("测试4: filter_records 多字段筛选\n");
    {
        CourseRecord *in[6];
        in[0] = make_rec("202401010001", "张三", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 85);
        in[1] = make_rec("202401010002", "李四", "软件学院",   "CS300102", "操作系统", 4.0f, "2024-2025-1", "2024-09-01", 92);
        in[2] = make_rec("202401010003", "王五", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-2", "2025-02-15", 55);
        in[3] = make_rec("202401010004", "赵六", "计算机学院", "CS300103", "计算机网络", 3.0f, "2024-2025-1", "2024-09-01", 78);
        in[4] = make_rec("202401010005", "钱七", "软件学院",   "CS300102", "操作系统", 4.0f, "2024-2025-2", "2025-02-15", 92);
        in[5] = make_rec("202401010006", "孙八", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", -1);

        CourseRecord *out[6];

        /* 初始化 FilterCriteria 的正确方式：score_min/max 设为 -1 表示不设限 */
        #define INIT_FC(fc) do { \
            memset(&(fc), 0, sizeof(fc)); \
            (fc).score_min = -1; \
            (fc).score_max = -1; \
            (fc).credit_min = -1.0f; \
        } while(0)

        FilterCriteria fc1;
        INIT_FC(fc1);
        strcpy(fc1.college, "计算机学院");
        strcpy(fc1.semester, "2024-2025-1");
        int cnt1 = filter_records(in, 6, out, &fc1);
        CHECK(cnt1 == 3, "计算机学院+2024-2025-1 -> 3条");

        FilterCriteria fc2;
        INIT_FC(fc2);
        fc2.score_min = 80;
        fc2.score_max = 100;
        int cnt2 = filter_records(in, 6, out, &fc2);
        CHECK(cnt2 == 3, "成绩80-100 -> 3条 (85,92,92; score=-1被排除)");

        FilterCriteria fc3;
        INIT_FC(fc3);
        fc3.credit_min = 4.0f;
        int cnt3 = filter_records(in, 6, out, &fc3);
        CHECK(cnt3 == 2, "学分>=4.0 -> 2条");

        FilterCriteria fc4;
        INIT_FC(fc4);
        strcpy(fc4.student_id, "2024010100");
        int cnt4 = filter_records(in, 6, out, &fc4);
        CHECK(cnt4 == 6, "学号前缀2024010100 -> 6条");

        FilterCriteria fc5;
        INIT_FC(fc5);
        int cnt5 = filter_records(in, 6, out, &fc5);
        CHECK(cnt5 == 6, "无条件 -> 6条");

        for (int i = 0; i < 6; i++) free(in[i]);
    }
    printf("\n");

    /* 测试 5: 统计函数 */
    printf("测试5: stat_score_distribution / stat_average_score\n");
    {
        CourseRecord *arr[8];
        arr[0] = make_rec("202401010001", "A", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 95);
        arr[1] = make_rec("202401010002", "B", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 85);
        arr[2] = make_rec("202401010003", "C", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 75);
        arr[3] = make_rec("202401010004", "D", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 65);
        arr[4] = make_rec("202401010005", "E", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 55);
        arr[5] = make_rec("202401010006", "F", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", -1);
        arr[6] = make_rec("202401010007", "G", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 100);
        arr[7] = make_rec("202401010008", "H", "计算机学院", "CS300101", "数据结构", 3.0f, "2024-2025-1", "2024-09-01", 90);

        int dist[5];
        stat_score_distribution(arr, 8, dist);
        CHECK(dist[0] == 1, "不及格 [0,60) = 1人 (55分)");
        CHECK(dist[1] == 1, "及格 [60,70) = 1人 (65分)");
        CHECK(dist[2] == 1, "中等 [70,80) = 1人 (75分)");
        CHECK(dist[3] == 1, "良好 [80,90) = 1人 (85分)");
        CHECK(dist[4] == 3, "优秀 [90,100] = 3人 (95,100,90)");

        float avg = stat_average_score(arr, 8);
        CHECK(avg > 80.0f && avg < 81.0f, "平均成绩约80.71 (忽略-1)");

        for (int i = 0; i < 8; i++) free(arr[i]);
    }
    printf("\n");

    /* 测试 6: JSON 边界情况 */
    printf("测试6: JSON 边界情况\n");
    {
        CourseRecord r;
        const char *json = "{\"sid\":\"202401010001\",\"name\":\"O\\\"Brien\",\"college\":\"Math Dept\","
            "\"cid\":\"MA100101\",\"cname\":\"Calculus I\",\"credit\":2.0,"
            "\"semester\":\"2024-2025-1\",\"date\":\"2024-09-01\",\"score\":100}";
        int ret = json_to_record(json, &r);
        CHECK(ret == 0, "解析含转义引号的JSON");
        CHECK(strcmp(r.name, "O\"Brien") == 0, "姓名含转义引号正确解析");

        CourseRecord *r2 = make_rec("202401010001", "测试学生", "计算机学院",
            "CS300101", "数据结构导论", 3.5f, "2024-2025-1", "2024-09-01", 88);
        char tiny[8];
        int len = record_to_json(r2, tiny, sizeof(tiny));
        CHECK(len == -1, "缓冲区不足返回 -1");
        free(r2);
    }
    printf("\n");

    printf("=== 测试结果: %d passed, %d failed ===\n", pass, fail);
    return fail > 0 ? 1 : 0;
}
