#!/usr/bin/env python3
"""生成 records.jsonl 测试数据文件 - 校园选课记录系统"""
import random
import json
import os

random.seed(42)  # 固定随机种子保证可复现

# 数据池
colleges = ["计算机学院", "软件学院", "信息学院", "数学学院"]
students = []
for i in range(1, 31):  # 30个学生
    sid = f"20240101{i:04d}"
    names = ["张三","李四","王五","赵六","钱七","孙八","周九","吴十",
             "郑明","王芳","李娜","刘洋","陈杰","杨帆","赵磊","黄丽",
             "周涛","吴婷","郑超","王强","李静","刘辉","陈晨","杨光",
             "赵敏","黄磊","周琳","吴鹏","郑雪","王磊"]
    college = colleges[i % 4]
    students.append((sid, names[i-1], college))

courses = [
    ("CS300101", "数据结构", 3.0),
    ("CS300102", "操作系统", 4.0),
    ("CS300103", "计算机网络", 3.0),
    ("CS300104", "数据库原理", 3.5),
    ("CS300105", "算法设计与分析", 3.0),
    ("MA200101", "高等数学", 5.0),
    ("MA200102", "线性代数", 3.0),
    ("EN100101", "大学英语", 2.0),
]

semesters = ["2024-2025-1", "2024-2025-2"]
dates_by_sem = {
    "2024-2025-1": "2024-09-01",
    "2024-2025-2": "2025-02-15"
}

records = []
used_keys = set()

# 每个学生选 3-5 门课
for sid, name, college in students:
    n_courses = random.randint(3, 5)
    chosen = random.sample(courses, n_courses)
    for cid, cname, credit in chosen:
        sem = random.choice(semesters)
        # 成绩分布：80%有成绩，20%未录入(-1)
        if random.random() < 0.2:
            score = -1
        else:
            # 成绩正态分布，均值78，标准差12，截断到[0,100]
            score = max(0, min(100, int(random.gauss(78, 12))))

        key = f"{sid}_{cid}"
        if key in used_keys:
            continue
        used_keys.add(key)

        records.append({
            "sid": sid,
            "name": name,
            "college": college,
            "cid": cid,
            "cname": cname,
            "credit": credit,
            "semester": sem,
            "date": dates_by_sem[sem],
            "score": score
        })

# 确保覆盖所有成绩分段：手动添加几条边界数据
# 不及格
records.append({"sid":"202401019991","name":"测试不及格","college":"计算机学院",
    "cid":"CS300101","cname":"数据结构","credit":3.0,"semester":"2024-2025-1",
    "date":"2024-09-01","score":45})
# 满分
records.append({"sid":"202401019992","name":"测试满分","college":"软件学院",
    "cid":"CS300102","cname":"操作系统","credit":4.0,"semester":"2024-2025-1",
    "date":"2024-09-01","score":100})
# 刚好60分
records.append({"sid":"202401019993","name":"测试及格线","college":"数学学院",
    "cid":"MA200101","cname":"高等数学","credit":5.0,"semester":"2024-2025-2",
    "date":"2025-02-15","score":60})
# 刚好90分
records.append({"sid":"202401019994","name":"测试优秀线","college":"信息学院",
    "cid":"CS300104","cname":"数据库原理","credit":3.5,"semester":"2024-2025-1",
    "date":"2024-09-01","score":90})

# 按 key 排序输出
records.sort(key=lambda r: f"{r['sid']}_{r['cid']}")

# 写入文件
out_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")
os.makedirs(out_dir, exist_ok=True)
out_path = os.path.join(out_dir, "records.jsonl")

with open(out_path, "w", encoding="utf-8") as f:
    for r in records:
        f.write(json.dumps(r, ensure_ascii=False) + "\n")

print(f"已生成 {len(records)} 条记录到 {out_path}")

# 统计信息
scores = [r["score"] for r in records if r["score"] >= 0]
dist = [0]*5
for s in scores:
    if s < 60: dist[0]+=1
    elif s < 70: dist[1]+=1
    elif s < 80: dist[2]+=1
    elif s < 90: dist[3]+=1
    else: dist[4]+=1

print(f"成绩分布: 不及格={dist[0]} 及格={dist[1]} 中等={dist[2]} 良好={dist[3]} 优秀={dist[4]}")
print(f"未录入: {len(records)-len(scores)} 条")
print(f"平均成绩: {sum(scores)/len(scores):.1f}")
print(f"学院分布: ", {c: sum(1 for r in records if r["college"]==c) for c in colleges})
