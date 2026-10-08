from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets'
OUT.mkdir(exist_ok=True)
FONT = '/System/Library/Fonts/STHeiti Medium.ttc'
INK = '#172C41'
MUTED = '#52677A'
BLUE = '#215D8C'
TEAL = '#16827B'
PALE = '#EAF2F8'
LINE = '#CFDBE4'


def font(size):
    return ImageFont.truetype(FONT, size)


def canvas(width, height):
    im = Image.new('RGB', (width, height), 'white')
    return im, ImageDraw.Draw(im)


def text(draw, xy, value, size=30, fill=INK, anchor=None):
    draw.text(xy, value, font=font(size), fill=fill, anchor=anchor)


def arrow(draw, a, b, fill=BLUE, width=5):
    draw.line([a, b], fill=fill, width=width)
    import math
    angle = math.atan2(b[1] - a[1], b[0] - a[0])
    length = 16
    points = [b]
    for delta in (-0.5, 0.5):
        points.append((b[0] - length * math.cos(angle + delta),
                       b[1] - length * math.sin(angle + delta)))
    draw.polygon(points, fill=fill)


def box(draw, rect, title, subtitle, fill=PALE, title_size=36):
    draw.rounded_rectangle(rect, radius=14, fill=fill, outline=LINE, width=2)
    x = (rect[0] + rect[2]) / 2
    y = (rect[1] + rect[3]) / 2
    text(draw, (x, y - 30), title, title_size, anchor='mm')
    for i, line in enumerate(subtitle.split('\n')):
        text(draw, (x, y + 23 + i * 36), line, 27, MUTED, 'mm')


im, d = canvas(1600, 390)
text(d, (45, 28), '一次上传经过哪里', 40)
nodes = [(40, 120, 335, 285), (450, 120, 745, 285),
         (865, 120, 1160, 285), (1280, 120, 1575, 285)]
for rect, title, sub in zip(nodes, ['手机 UE', '基站 gNB', '5G 核心网', '应用服务器'],
                            ['数据变为无线信号', '无线信号还原为数据', '继续转送数据', '接收上传的内容']):
    box(d, rect, title, sub)
for i in range(3):
    arrow(d, (nodes[i][2] + 12, 200), (nodes[i+1][0] - 12, 200))
text(d, (390, 98), 'PUSCH', 26, TEAL, 'mm')
text(d, (805, 98), '后续网络', 24, MUTED, 'mm')
text(d, (1220, 98), '后续网络', 24, MUTED, 'mm')
text(d, (800, 342), '本文重点是 UE 与 gNB 之间的上行空口', 28, MUTED, 'mm')
im.save(OUT / 'network.png')


im, d = canvas(1600, 740)
text(d, (45, 25), '一个 PRB 的资源网格', 40)
text(d, (45, 82), '算例：普通 CP，14 符号时隙，PUSCH 使用 0 至 11', 29, MUTED)
x0, y0, cw, ch = 185, 155, 91, 33
for k in range(12):
    yy = y0 + (11-k) * ch
    text(d, (158, yy+ch/2), str(k), 23, MUTED, 'rm')
    for l in range(14):
        fill = '#EFF2F4' if l >= 12 else ('#16827B' if l == 2 and k % 2 == 0 else '#B8D3EA')
        d.rectangle((x0+l*cw, yy, x0+(l+1)*cw, yy+ch), fill=fill, outline='white', width=2)
        if l == 2 and k % 2 == 0:
            text(d, (x0+l*cw+cw/2, yy+ch/2), 'R', 23, 'white', 'mm')
for l in range(14):
    text(d, (x0+l*cw+cw/2, y0+12*ch+25), str(l), 25, MUTED, 'mm')
d.rectangle((x0+6*cw, y0+8*ch, x0+7*cw, y0+9*ch), outline=INK, width=4)
text(d, (x0+7*cw+14, y0+8.5*ch), '1 RE', 23)
text(d, (46, 190), '频率 ↑', 27)
text(d, (46, 239), '12 条', 27)
text(d, (46, 280), '子载波', 27)
text(d, (820, 615), 'OFDM 符号索引   时间 →', 29, INK, 'mm')
for x, fill, label in [(195, '#B8D3EA', '数据 RE'), (590, '#16827B', 'DM-RS RE'),
                       (1040, '#EFF2F4', '本次未分配')]:
    d.rectangle((x, 673, x+34, 707), fill=fill, outline=LINE)
    text(d, (x+50, 688), label, 28, INK, 'lm')
im.save(OUT / 'grid.png')


im, d = canvas(1600, 980)
text(d, (45, 28), '一包数据的 PUSCH 发送主线', 40)
groups = [
    ('先处理比特', [('TB 与 CRC', '检查整包'), ('基图与码块', '选择结构并分段'),
                  ('LDPC 编码', '增加纠错冗余'), ('速率匹配与拼接', '适应本次容量')]),
    ('再变成调制符号', [('UCI 复用', '按配置加入控制信息'), ('加扰', '使用约定扰码'),
                         ('调制', '比特变成复数符号'), ('层映射', '分配到数据层')]),
    ('最后形成无线信号', [('变换预编码', '可选 DFT 操作'), ('空间预编码', '层映射到端口'),
                           ('资源网格', '映射数据与参考信号'), ('OFDM 与射频', '生成波形并发射')])
]
for row, (label, entries) in enumerate(groups):
    yy = 118 + row*252
    text(d, (50, yy), f'{row+1}  {label}', 29, MUTED)
    for col, (title, sub) in enumerate(entries):
        xx = 48 + col*398
        box(d, (xx, yy+53, xx+310, yy+183), title, sub, title_size=31)
        if col < 3:
            arrow(d, (xx+322, yy+118), (xx+385, yy+118))
    if row < 2:
        text(d, (1488, yy+221), '接下一行 ↓', 25, TEAL, 'mm')
d.line((50, 866, 1550, 866), fill=LINE, width=2)
text(d, (800, 908), '基站接收：同步与信道估计 → 检测解调 → 速率恢复与译码 → CRC 检查', 28, INK, 'mm')
im.save(OUT / 'chain.png')


im, d = canvas(1600, 630)
text(d, (45, 25), '更多星座点可以装更多比特', 40)
text(d, (45, 82), '教学图：未显示比特标签，两个星座按相同平均能量归一化', 28, MUTED)
import math
for cx, title, qm, vals in [(420, 'QPSK  4 个点', '2 编码比特 / 调制符号', [-1/math.sqrt(2), 1/math.sqrt(2)]),
                            (1180, '16QAM  16 个点', '4 编码比特 / 调制符号', [-3/math.sqrt(10), -1/math.sqrt(10), 1/math.sqrt(10), 3/math.sqrt(10)])]:
    cy, scale = 345, 150
    arrow(d, (cx-235, cy), (cx+235, cy), MUTED, 3)
    arrow(d, (cx, cy+190), (cx, cy-190), MUTED, 3)
    text(d, (cx+252, cy), 'I', 29, MUTED, 'mm')
    text(d, (cx, cy-210), 'Q', 29, MUTED, 'mm')
    for a in vals:
        for b in vals:
            x, y = cx+a*scale, cy-b*scale
            d.ellipse((x-11, y-11, x+11, y+11), fill=BLUE)
    text(d, (cx, 554), title, 31, INK, 'mm')
    text(d, (cx, 598), qm, 27, MUTED, 'mm')
im.save(OUT / 'constellation.png')


im, d = canvas(1600, 830)
text(d, (45, 25), '分清上行重传与下行接收反馈', 40)
for left, title in [(45, '上行 TB 解码失败后重传'), (850, 'UE 反馈下行接收结果')]:
    text(d, (left, 112), title, 32)
    xue, xgnb = left+75, left+600
    text(d, (xue, 185), 'UE', 32, INK, 'mm')
    text(d, (xgnb, 185), 'gNB', 32, INK, 'mm')
    for x in [xue, xgnb]:
        d.line((x, 220, x, 720), fill=LINE, width=4)
    if left == 45:
        events = [(265, xgnb, xue, '上行授权'), (360, xue, xgnb, 'PUSCH 发送 TB'),
                  (520, xgnb, xue, '安排同一进程的重传'), (625, xue, xgnb, '同一 TB 的相应 RV')]
        text(d, (left+605, 427), 'CRC 失败', 25, TEAL, 'mm')
        text(d, (left+605, 710), '软信息合并', 25, TEAL, 'mm')
    else:
        events = [(300, xgnb, xue, 'PDSCH 下行数据'), (580, xue, xgnb, 'PUCCH 或 PUSCH 上的 HARQ-ACK')]
        text(d, (left+85, 425), 'UE 检查下行接收', 25, TEAL, 'mm')
    for y, xa, xb, label in events:
        arrow(d, (xa, y), (xb, y))
        text(d, ((xa+xb)/2, y-34), label, 26, INK, 'mm')
d.line((800, 110, 800, 740), fill=LINE, width=2)
text(d, (800, 792), '教学时序省略具体等待时间；右侧反馈不是在确认左侧的本次上传', 28, MUTED, 'mm')
im.save(OUT / 'harq.png')

im, d = canvas(1600, 1030)
text(d, (45, 25), '用五份规范理解一次 PUSCH 发送', 40)
text(d, (45, 82), '控制与数据过程决定条件；比特与信号处理完成发送；测量提供信息', 28, MUTED)
box(d, (60, 150, 740, 330), '38.213  控制过程', '功率控制 · UCI 报告 · 控制接收', title_size=35)
box(d, (860, 150, 1540, 330), '38.214  数据过程', '发送方案 · 资源 · MCS · TBS', title_size=35)
arrow(d, (400, 343), (400, 426), TEAL)
arrow(d, (1200, 343), (1200, 426), TEAL)
text(d, (800, 393), '提供本次发送的参数和条件', 28, MUTED, 'mm')
box(d, (60, 445, 710, 640), '38.212  比特处理', 'CRC · LDPC · 速率匹配 · UCI 复用', title_size=35)
box(d, (890, 445, 1540, 640), '38.211  物理信号', '调制 · 预编码 · 映射 · 参考信号 · 波形', title_size=35)
arrow(d, (725, 542), (875, 542))
text(d, (800, 502), '编码比特', 26, BLUE, 'mm')
box(d, (300, 755, 1300, 935), '38.215  物理层测量', '定义 UE 与 NG-RAN 的测量量；测量结果可供后续控制和调度使用', fill='#ECF6F3', title_size=35)
d.line([(300, 845), (30, 845), (30, 235)], fill=TEAL, width=4)
arrow(d, (30, 235), (60, 235), TEAL)
arrow(d, (1300, 845), (1555, 845), TEAL)
d.line([(1555, 845), (1570, 845), (1570, 235)], fill=TEAL, width=4)
arrow(d, (1570, 235), (1540, 235), TEAL)
text(d, (800, 986), '功能关系教学图；38.215 不是编码链中的下一个串行处理步骤', 28, MUTED, 'mm')
im.save(OUT / 'standards.png')


im, d = canvas(1600, 980)
text(d, (45, 25), '接收方向不同 测量用途也不同', 40)
text(d, (45, 82), '把 UE 的下行测量与 gNB 的上行测量分开理解', 28, MUTED)
box(d, (60, 150, 390, 285), 'UE 接收', '下行参考信号', title_size=35)
box(d, (1210, 150, 1540, 285), 'gNB 发送', 'SSB / CSI-RS', title_size=35)
arrow(d, (1190, 215), (410, 215))
text(d, (800, 177), '下行', 29, BLUE, 'mm')
box(d, (85, 350, 750, 545), 'UE：RSRP 等测量', '38.215 §5.1\n匹配参考信号 → PL → PUSCH 功率', fill='#ECF6F3', title_size=34)
text(d, (1150, 415), '功率控制：38.213 §7.1', 32, INK, 'mm')
text(d, (1150, 468), '测量值与参考发射功率要匹配', 27, MUTED, 'mm')
arrow(d, (770, 445), (885, 445), TEAL)
box(d, (60, 615, 390, 750), 'UE 发送', 'SRS / PUSCH', title_size=35)
box(d, (1210, 615, 1540, 750), 'gNB 接收', '测量与译码', title_size=35)
arrow(d, (410, 680), (1190, 680))
text(d, (800, 642), '上行', 29, BLUE, 'mm')
text(d, (800, 810), 'gNB：UL SRS-RSRP 等测量（38.215 §5.2）与接收结果', 30, INK, 'mm')
text(d, (800, 862), '可供调度参考；UE 按 38.214 解释资源、发送方案、MCS 与 TBS', 28, MUTED, 'mm')
d.line((55, 914, 1545, 914), fill=LINE, width=2)
text(d, (800, 945), '测量对象与 PUSCH 接收条件可能不同；标准没有固定的“RSRP → MCS”算法', 27, MUTED, 'mm')
im.save(OUT / 'measurements.png')
print('Created 7 original diagrams in', OUT)
