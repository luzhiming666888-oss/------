/**
 * @file    record.h
 * @brief   选课记录数据结构定义 —— 校园选课记录检索与大数据分析系统
 *
 * 本头文件定义核心业务实体 CourseRecord（选课记录），
 * 以及由学号和课程编号拼接而成的唯一主键（Key）格式。
 *
 * 唯一主键格式：<学号>_<课程编号>，例如：202401011234_CS300102
 *
 * @author  （作者姓名 / 学号，待填写）
 * @date    2026-06-23
 * @version 1.0
 */

#ifndef RECORD_H
#define RECORD_H

/* -----------------------------------------------------------------------
 * 字段长度宏定义
 * ---------------------------------------------------------------------- */
#define STUDENT_ID_LEN   13   /* 学号：12位字符 + '\0' */
#define NAME_LEN         32   /* 姓名：最多 31 个字节（兼容中文 UTF-8） + '\0' */
#define COLLEGE_LEN      64   /* 学院名称：最多 63 字节 + '\0' */
#define COURSE_ID_LEN    9    /* 课程编号：8位字符 + '\0' */
#define COURSE_NAME_LEN  64   /* 课程名称：最多 63 字节 + '\0' */
#define SEMESTER_LEN     16   /* 选课学期：格式 "2024-2025-1" + '\0' */
#define DATE_LEN         11   /* 选课日期：格式 "YYYY-MM-DD" + '\0' */
#define KEY_LEN          (STUDENT_ID_LEN + COURSE_ID_LEN + 1)
                              /* 唯一主键：学号(12) + '_' + 课程编号(8) + '\0' = 22 */

/* -----------------------------------------------------------------------
 * 核心业务结构体
 * ---------------------------------------------------------------------- */

/**
 * @struct CourseRecord
 * @brief  一条选课记录，对应数据文件中的一个 JSON 对象行。
 */
typedef struct CourseRecord {
    char  student_id[STUDENT_ID_LEN];   /**< 学号，12位数字字符串           */
    char  name[NAME_LEN];               /**< 学生姓名                        */
    char  college[COLLEGE_LEN];         /**< 所属学院                        */
    char  course_id[COURSE_ID_LEN];     /**< 课程编号，8位字母数字字符串     */
    char  course_name[COURSE_NAME_LEN]; /**< 课程名称                        */
    float credit;                       /**< 学分（如 2.0、3.5）             */
    char  semester[SEMESTER_LEN];       /**< 选课学期，格式 "YYYY-YYYY-N"    */
    char  date[DATE_LEN];               /**< 选课日期，格式 "YYYY-MM-DD"     */
    int   score;                        /**< 成绩，取值范围 0 ~ 100；
                                             -1 表示尚未录入                 */
    char  key[KEY_LEN];                 /**< 唯一主键（由程序自动生成，勿手填）*/
} CourseRecord;

/* -----------------------------------------------------------------------
 * 辅助函数声明
 * ---------------------------------------------------------------------- */

/**
 * @brief  根据学号和课程编号生成唯一主键，写入 record->key。
 * @param  record  指向待填充主键的 CourseRecord 指针（非 NULL）。
 */
void record_make_key(CourseRecord *record);

/**
 * @brief  打印一条选课记录的格式化信息到标准输出。
 * @param  record  指向要打印的 CourseRecord 指针（非 NULL）。
 */
void record_print(const CourseRecord *record);

/**
 * @brief  比较两条记录的唯一主键（字典序）。
 * @return 负数：a < b；0：a == b；正数：a > b。
 */
int  record_compare_key(const CourseRecord *a, const CourseRecord *b);

#endif /* RECORD_H */
