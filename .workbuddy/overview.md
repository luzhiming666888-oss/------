# 项目实现进度

## 《校园选课记录检索与大数据分析系统》

**技术栈**：纯 C 语言 · CLI · AVL 树 · 拉链法哈希表 · JSON Lines

---

## 实现进度

| 文件 | 状态 | 说明 |
|------|------|------|
| `src/hash.c` | ✅ 完成 | FNV-1a 哈希，拉链法，鹿智明 2026-06-24 |
| `src/avl.c` | ✅ 完成 | 11 个函数全实现，7 项测试通过，鹿智明 2026-06-25 |
| `src/util.c` | ✅ 完成 | 8 个函数全实现，40 项测试通过，鹿智明 2026-06-25 |
| `src/main.c` | ⬜ 待实现 | CLI 菜单交互逻辑 |
| `data/records.jsonl` | ✅ 已生成 | 119 条测试数据，4 学院 8 课程 2 学期 |
| `report_draft.md` | ✅ 已更新 | AVL/哈希章节填充完成，DJB2→FNV-1a 修正 |

---

## 目录结构

```
E:\数据结构实训\
├── include/
│   ├── record.h   ← 数据结构定义
│   ├── avl.h      ← AVL 树接口
│   ├── hash.h     ← 哈希表接口
│   └── util.h     ← I/O、排序、统计接口
├── src/
│   ├── main.c     ← CLI 菜单入口（待实现）
│   ├── avl.c      ← AVL 树实现 ✅
│   ├── hash.c     ← 哈希表实现 ✅
│   └── util.c     ← 工具函数实现 ✅
├── data/
│   └── records.jsonl  ← 119 条测试数据 ✅
├── tests/
│   └── test_util.c    ← util.c 功能测试（40 项全通过）
├── scripts/
│   └── gen_test_data.py  ← 测试数据生成脚本
└── report_draft.md   ← 实验报告草稿
```

---

## 关键设计

### 双索引架构
- **AVL 树**：唯一所有权，`avl_destroy` 负责 `free(data)`，支持 O(log n) 有序遍历
- **哈希表**：共享指针，`hash_destroy` 只释放链表节点不 free data，O(1) 均摊查找

### JSON Lines 格式
```json
{"sid":"202401010001","name":"张三","college":"计算机学院","cid":"CS300101","cname":"数据结构","credit":3.0,"semester":"2024-2025-1","date":"2024-09-01","score":88}
```

### 排序与筛选
- 三路快速排序（Dutch National Flag），支持 8 种 SortKey
- 多字段联合筛选（学号前缀/学院/课程/学期/成绩范围/学分下限）
- `FilterCriteria` 初始化：score_min=-1, score_max=-1, credit_min=-1 表示不设限

---

## 下一步

1. **实现 `main.c`**：CLI 菜单交互（加载数据 → 增删改查 → 排序筛选 → 统计 → 保存退出）
2. 全项目编译：`gcc -Wall -Wextra -std=c99 src/*.c -I include -o campus_course`
