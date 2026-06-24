from __future__ import annotations

import math
from pathlib import Path
from typing import Iterable

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent
FONT_REGULAR = "C:/Windows/Fonts/msyh.ttc"
FONT_BOLD = "C:/Windows/Fonts/msyhbd.ttc"


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(FONT_BOLD if bold else FONT_REGULAR, size=size)


def text_size(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.ImageFont) -> tuple[int, int]:
    box = draw.multiline_textbbox((0, 0), text, font=fnt, spacing=4, align="center")
    return box[2] - box[0], box[3] - box[1]


def wrap_text(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.ImageFont, max_width: int) -> str:
    lines: list[str] = []
    for raw in text.split("\n"):
        line = ""
        for ch in raw:
            candidate = line + ch
            if text_size(draw, candidate, fnt)[0] <= max_width or not line:
                line = candidate
            else:
                lines.append(line)
                line = ch
        lines.append(line)
    return "\n".join(lines)


def centered_text(
    draw: ImageDraw.ImageDraw,
    box: tuple[int, int, int, int],
    text: str,
    fnt: ImageFont.ImageFont,
    fill: str = "black",
    spacing: int = 4,
) -> None:
    x1, y1, x2, y2 = box
    wrapped = wrap_text(draw, text, fnt, x2 - x1 - 16)
    w, h = text_size(draw, wrapped, fnt)
    draw.multiline_text(
        (x1 + (x2 - x1 - w) / 2, y1 + (y2 - y1 - h) / 2),
        wrapped,
        font=fnt,
        fill=fill,
        spacing=spacing,
        align="center",
    )


def fitted_multiline_text(
    draw: ImageDraw.ImageDraw,
    box: tuple[int, int, int, int],
    text: str,
    max_size: int,
    min_size: int,
    fill: str,
    spacing: int = 3,
    bold: bool = False,
) -> None:
    x1, y1, x2, y2 = box
    for size in range(max_size, min_size - 1, -1):
        fnt = font(size, bold)
        wrapped = wrap_text(draw, text, fnt, x2 - x1)
        _, h = text_size(draw, wrapped, fnt)
        if h <= (y2 - y1):
            draw.multiline_text((x1, y1), wrapped, font=fnt, fill=fill, spacing=spacing)
            return

    fnt = font(min_size, bold)
    wrapped = wrap_text(draw, text, fnt, x2 - x1)
    draw.multiline_text((x1, y1), wrapped, font=fnt, fill=fill, spacing=spacing)


def rounded_box(
    draw: ImageDraw.ImageDraw,
    box: tuple[int, int, int, int],
    fill: str,
    outline: str | None = None,
    radius: int = 12,
    width: int = 2,
) -> None:
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def arrow_down(draw: ImageDraw.ImageDraw, cx: int, y: int, scale: float = 1.0) -> None:
    w = int(78 * scale)
    h = int(44 * scale)
    pts = [
        (cx - w // 2, y),
        (cx, y + h),
        (cx + w // 2, y),
        (cx + w // 2, y + int(15 * scale)),
        (cx, y + h + int(17 * scale)),
        (cx - w // 2, y + int(15 * scale)),
    ]
    draw.polygon(pts, fill="#1fd2d4", outline="#e6ffff")


def line_arrow(draw: ImageDraw.ImageDraw, start: tuple[int, int], end: tuple[int, int], fill: str = "white", width: int = 3) -> None:
    draw.line([start, end], fill=fill, width=width)
    angle = math.atan2(end[1] - start[1], end[0] - start[0])
    size = 12
    p1 = (end[0] - size * math.cos(angle - math.pi / 6), end[1] - size * math.sin(angle - math.pi / 6))
    p2 = (end[0] - size * math.cos(angle + math.pi / 6), end[1] - size * math.sin(angle + math.pi / 6))
    draw.polygon([end, p1, p2], fill=fill)


def polyline_arrow(draw: ImageDraw.ImageDraw, points: list[tuple[int, int]], fill: str, width: int = 3) -> None:
    draw.line(points, fill=fill, width=width, joint="curve")
    if len(points) < 2:
        return
    start = points[-2]
    end = points[-1]
    angle = math.atan2(end[1] - start[1], end[0] - start[0])
    size = 12
    p1 = (end[0] - size * math.cos(angle - math.pi / 6), end[1] - size * math.sin(angle - math.pi / 6))
    p2 = (end[0] - size * math.cos(angle + math.pi / 6), end[1] - size * math.sin(angle + math.pi / 6))
    draw.polygon([end, p1, p2], fill=fill)


def draw_card(
    draw: ImageDraw.ImageDraw,
    x: int,
    y: int,
    w: int,
    h: int,
    title: str,
    subtitle: str,
    fill: str,
    title_size: int = 23,
    sub_size: int = 15,
) -> None:
    rounded_box(draw, (x, y, x + w, y + h), fill=fill, radius=12)
    centered_text(draw, (x + 6, y + 8, x + w - 6, y + h // 2 + 8), title, font(title_size), "black")
    centered_text(draw, (x + 6, y + h // 2 + 2, x + w - 6, y + h - 8), subtitle, font(sub_size), "black")


def architecture_image() -> None:
    img = Image.new("RGB", (1800, 950), "white")
    draw = ImageDraw.Draw(img)
    left = 75
    right = 1725
    label_w = 250
    layer_h = 130
    gap = 55
    y = 55
    layers = [
        ("应用层\n(APP)", "#8d6aad", "#ffd966", [
            ("app_camera", "摄像头\nOV2640"),
            ("app_oled", "OLED显示\nI2C模拟"),
            ("app_DHT11", "温湿度\nGPIO"),
            ("app_mq2", "烟雾/气体\nADC1_CH0"),
            ("app_sw180", "振动检测\nGPIO"),
            ("app_rc522", "RFID刷卡\nSPI2"),
            ("app_servo", "门锁舵机\nLEDC PWM"),
            ("app_Buzzer", "蜂鸣器\nGPIO"),
        ]),
        ("板级支持包\n(BSP)", "#92d050", "#d8e8f7", [
            ("bsp_platform", "HAL兼容桥接"),
            ("bsp_gpio", "GPIO读写"),
            ("bsp_adc", "ADC采样"),
            ("bsp_timer", "PWM/定时"),
            ("bsp_uart", "串口/printf"),
            ("app_oled", "OLED驱动/字库"),
        ]),
        ("核心层\n(Core/RTOS)", "#9dc3e6", "#f2f2f2", [
            ("task_state", "运行态状态中心"),
            ("task_*", "采样/控制/显示"),
            ("FreeRTOS", "Task/Queue/Semaphore"),
            ("MQTT/HTTP", "联网与上传"),
            ("main.c", "启动编排"),
        ]),
        ("驱动层\n(Driver)", "#2f79b8", "#70e5e0", [
            ("ESP-IDF Drivers", "GPIO/ADC/UART/SPI\nLEDC"),
            ("Network Stack", "Wi-Fi/MQTT/HTTP/TLS"),
            ("esp32-camera", "Camera驱动"),
            ("cJSON/mbedTLS", "JSON/安全通信"),
        ]),
        ("硬件层\n(Hardware)", "#ff9f13", "#f3f3f3", [
            ("ESP32-S3", "主控芯片"),
            ("GPIO1", "MQ-2 ADC"),
            ("GPIO2", "DHT11"),
            ("GPIO35", "Servo"),
            ("GPIO36", "Buzzer"),
            ("GPIO37", "SW180"),
            ("GPIO38-42/14", "RC522 SPI2"),
            ("GPIO21/47", "OLED I2C"),
            ("GPIO3-18", "Camera"),
        ]),
    ]
    title_f = font(29)
    for idx, (label, color, card_color, cards) in enumerate(layers):
        draw.rectangle((left, y, right, y + layer_h), fill=color)
        draw.line((left + label_w, y, left + label_w, y + layer_h), fill="black", width=3)
        centered_text(draw, (left, y, left + label_w, y + layer_h), label, title_f)
        start_x = left + label_w + 34
        available = right - start_x - 35
        card_w = min(180, int((available - 26 * (len(cards) - 1)) / len(cards)))
        if len(cards) <= 5:
            card_w = 200
        step = (available - card_w) / max(1, len(cards) - 1)
        for i, (name, sub) in enumerate(cards):
            x = int(start_x + i * step)
            draw_card(draw, x, y + 19, card_w, 92, name, sub, card_color, 18, 14)
        if idx < len(layers) - 1:
            arrow_down(draw, (left + right) // 2, y + layer_h + 3, 0.9)
        y += layer_h + gap
    img.save(ROOT / "savebox_architecture_layers.png")


def startup_image() -> None:
    img = Image.new("RGB", (1800, 760), "white")
    draw = ImageDraw.Draw(img)

    draw.text((70, 54), "Savebox 系统启动流程", font=font(34, True), fill="#122033")
    draw.text((72, 102), "从 ESP32-S3 上电到 FreeRTOS 多任务持续运行", font=font(18), fill="#64748b")

    line_y = 268
    draw.line((190, line_y, 1610, line_y), fill="#cbd5e1", width=6)

    steps = [
        ("系统上电", "ESP32-S3 启动\n进入 app_main()"),
        ("芯片信息输出", "打印芯片型号\nFlash / Heap"),
        ("BSP 初始化", "BSP_Init()\nTimer / OLED / 平台"),
        ("APP 初始化", "APP_Init()\nCamera 等外设"),
        ("状态中心初始化", "task_state_init()\n创建互斥锁\n默认上锁"),
        ("创建业务任务", "TASK_StartAll()\n采集 / 控制 / 显示"),
        ("启动联网服务", "MQTT Start\n联网通信"),
        ("持续运行", "FreeRTOS 调度\n本地监控 + 云端交互"),
    ]

    colors = ["#2563eb", "#0891b2", "#16a34a", "#65a30d", "#ca8a04", "#ea580c", "#dc2626", "#7c3aed"]
    start_x = 130
    step_w = 205
    card_w = 178
    for i, (title, body) in enumerate(steps):
        cx = start_x + i * step_w
        circle = (cx - 27, line_y - 27, cx + 27, line_y + 27)
        draw.ellipse(circle, fill=colors[i], outline="white", width=4)
        centered_text(draw, circle, str(i + 1), font(21, True), "white")

        card_y = 330 if i % 2 == 0 else 410
        shadow = (cx - card_w // 2 + 5, card_y + 6, cx + card_w // 2 + 5, card_y + 152)
        rounded_box(draw, shadow, "#e2e8f0", radius=16)
        box = (cx - card_w // 2, card_y, cx + card_w // 2, card_y + 146)
        rounded_box(draw, box, "white", outline="#cbd5e1", radius=16, width=2)
        draw.rectangle((box[0], box[1], box[2], box[1] + 10), fill=colors[i])
        centered_text(draw, (box[0] + 14, box[1] + 18, box[2] - 14, box[1] + 54), title, font(18, True), "#111827")
        centered_text(draw, (box[0] + 12, box[1] + 62, box[2] - 12, box[3] - 14), body, font(14), "#334155")

        line_arrow(draw, (cx, line_y + 31), (cx, card_y - 4), "#94a3b8", 2)

    note = (115, 635, 1685, 705)
    rounded_box(draw, note, "#f8fafc", outline="#cbd5e1", radius=16, width=2)
    centered_text(
        draw,
        note,
        "启动主线：先完成板级与外设初始化，再创建业务任务，最后启动联网服务；传感器采集、门锁控制、OLED 刷新和云端通信在 FreeRTOS 下并行运行。",
        font(18),
        "#334155",
    )
    img.save(ROOT / "savebox_startup_flow.png")


def flow_node(draw: ImageDraw.ImageDraw, center: tuple[int, int], size: tuple[int, int], text: str, shape: str = "rect") -> tuple[int, int, int, int]:
    cx, cy = center
    w, h = size
    box = (cx - w // 2, cy - h // 2, cx + w // 2, cy + h // 2)
    if shape == "ellipse":
        draw.ellipse(box, fill="white", outline="#222222", width=2)
    elif shape == "diamond":
        pts = [(cx, cy - h // 2), (cx + w // 2, cy), (cx, cy + h // 2), (cx - w // 2, cy)]
        draw.polygon(pts, fill="white", outline="#222222")
        draw.line([pts[0], pts[1], pts[2], pts[3], pts[0]], fill="#222222", width=2)
    else:
        draw.rectangle(box, fill="white", outline="#222222", width=2)
    centered_text(draw, box, text, font(18), "#111111")
    return box


def business_flow_image_legacy() -> None:
    img = Image.new("RGB", (1800, 1250), "white")
    draw = ImageDraw.Draw(img)
    ink = "#172033"
    muted = "#64748b"
    blue = "#2563eb"
    green = "#16a34a"
    orange = "#ea580c"
    purple = "#7c3aed"
    red = "#dc2626"

    draw.text((70, 48), "Savebox 业务运行流程", font=font(34, True), fill=ink)
    draw.text((72, 96), "传感监控、门锁控制、告警抓拍、状态展示与云端上报的主流程", font=font(18), fill=muted)

    def section(box: tuple[int, int, int, int], title: str, fill: str, stroke: str) -> None:
        rounded_box(draw, box, fill, outline=stroke, radius=22, width=2)
        draw.text((box[0] + 24, box[1] + 18), title, font=font(22, True), fill=stroke)

    def node(box: tuple[int, int, int, int], title: str, body: str = "", color: str = blue) -> tuple[int, int, int, int]:
        rounded_box(draw, box, "white", outline="#cbd5e1", radius=16, width=2)
        draw.rectangle((box[0], box[1], box[0] + 9, box[3]), fill=color)
        draw.text((box[0] + 24, box[1] + 16), title, font=font(18, True), fill=ink)
        if body:
            draw.multiline_text((box[0] + 24, box[1] + 50), body, font=font(14), fill="#475569", spacing=4)
        return box

    def decision(center: tuple[int, int], text: str, color: str) -> tuple[int, int, int, int]:
        cx, cy = center
        w, h = 160, 94
        pts = [(cx, cy - h // 2), (cx + w // 2, cy), (cx, cy + h // 2), (cx - w // 2, cy)]
        draw.polygon(pts, fill="white")
        draw.line([pts[0], pts[1], pts[2], pts[3], pts[0]], fill=color, width=3)
        centered_text(draw, (cx - 58, cy - 30, cx + 58, cy + 30), text, font(17, True), ink)
        return (cx - w // 2, cy - h // 2, cx + w // 2, cy + h // 2)

    def connect(start: tuple[int, int], end: tuple[int, int], color: str = "#475569", label: str = "") -> None:
        line_arrow(draw, start, end, color, 3)
        if label:
            mx = (start[0] + end[0]) // 2
            my = (start[1] + end[1]) // 2
            rounded_box(draw, (mx - 36, my - 15, mx + 36, my + 15), "white", outline="#e2e8f0", radius=8, width=1)
            centered_text(draw, (mx - 36, my - 15, mx + 36, my + 15), label, font(13), color)

    section((60, 150, 1740, 300), "1. 启动与任务创建", "#f8fafc", "#334155")
    n_boot = node((160, 205, 355, 270), "系统启动", "app_main()", blue)
    n_init = node((450, 205, 675, 270), "初始化", "BSP_Init / APP_Init", blue)
    n_state = node((770, 205, 1005, 270), "状态中心", "task_state_init()", blue)
    n_tasks = node((1100, 205, 1380, 270), "创建并行任务", "Camera / DHT11\nMQ2 / SW180 / OLED", blue)
    n_net = node((1480, 205, 1680, 270), "联网服务", "Wi-Fi / MQTT / HTTP", blue)
    connect((355, 238), (450, 238), blue)
    connect((675, 238), (770, 238), blue)
    connect((1005, 238), (1100, 238), blue)
    connect((1380, 238), (1480, 238), blue)

    section((60, 340, 880, 760), "2. 传感监控与告警", "#f0fdf4", green)
    dht = node((105, 420, 300, 500), "DHT11", "温湿度采集\n更新状态中心", green)
    mq2 = node((355, 420, 550, 500), "MQ-2", "气体 ADC 采样\n计算增量阈值", green)
    sw = node((605, 420, 800, 500), "SW180", "振动中断 + 去抖\n判断报警状态", green)
    alarm = decision((455, 610), "是否报警", red)
    buzzer = node((150, 660, 330, 730), "蜂鸣器报警", "task_buzzer_beep()", red)
    camera = node((390, 660, 620, 730), "摄像头抓拍", "gas / vibration\nHTTP 上传告警帧", red)
    normal = node((660, 660, 825, 730), "正常运行", "继续周期采样", green)
    connect((300, 460), (380, 575), green)
    connect((455, 500), (455, 563), green)
    connect((700, 500), (530, 575), green)
    connect((390, 640), (300, 660), red, "是")
    connect((455, 657), (505, 660), red, "是")
    connect((535, 610), (660, 690), green, "否")

    section((920, 340, 1740, 760), "3. 门锁控制", "#fff7ed", orange)
    card = node((970, 420, 1165, 500), "用户刷卡", "RC522 读取 UID", orange)
    verify = decision((1285, 460), "白名单验证", orange)
    reject = node((1485, 420, 1665, 500), "拒绝开锁", "未知卡或冷却中", red)
    lock_state = decision((1285, 610), "当前锁状态", purple)
    unlock = node((1015, 665, 1210, 735), "执行开锁", "舵机 10°\n更新 lock=false", purple)
    lock = node((1360, 665, 1555, 735), "执行关锁", "舵机 90°\n更新 lock=true", purple)
    remote = node((1485, 550, 1685, 630), "远程命令", "MQTT / UART\nLOCK / UNLOCK", orange)
    connect((1165, 460), (1205, 460), orange)
    connect((1365, 460), (1485, 460), red, "失败")
    connect((1285, 507), (1285, 563), orange, "成功")
    connect((1205, 610), (1110, 665), purple, "已锁")
    connect((1365, 610), (1455, 665), purple, "未锁")
    connect((1485, 590), (1360, 610), orange)

    section((60, 815, 1740, 1125), "4. 状态展示与云端同步", "#f5f3ff", purple)
    state = node((150, 900, 390, 995), "统一状态中心", "温湿度、气体、振动\n卡号、锁状态、摄像头状态", purple)
    oled = node((515, 900, 735, 995), "OLED 显示", "LOCK / TEMP / HUMI\nMQ2 / VIB / CARD", purple)
    mqtt = node((860, 900, 1110, 995), "MQTT 周期上报", "设备属性、事件\n云端命令响应", purple)
    http = node((1235, 900, 1485, 995), "HTTP 上传", "最新画面、告警帧\n状态快照", purple)
    cloud = node((1550, 900, 1710, 995), "云端服务", "IoTDA Broker\n图片服务", purple)
    connect((390, 948), (515, 948), purple)
    connect((735, 948), (860, 948), purple)
    connect((1110, 948), (1235, 948), purple)
    connect((1485, 948), (1550, 948), purple)

    rounded_box(draw, (655, 785, 1150, 835), "#ffffff", outline="#cbd5e1", radius=14, width=2)
    centered_text(draw, (655, 785, 1150, 835), "各业务任务统一写入 task_state，OLED、MQTT、HTTP 从同一份运行态读取", font(16), "#475569")

    img.save(ROOT / "savebox_business_flow.png")


def business_flow_image() -> None:
    img = Image.new("RGB", (1800, 900), "white")
    draw = ImageDraw.Draw(img)

    ink = "#142033"
    body = "#475569"
    muted = "#64748b"
    border = "#cbd5e1"
    blue = "#2563eb"
    green = "#16a34a"
    orange = "#ea580c"
    purple = "#7c3aed"
    red = "#dc2626"

    title_font = font(36, True)
    subtitle_font = font(18)
    section_font = font(24, True)
    node_title_font = font(19, True)
    label_font = font(14)

    draw.text((70, 34), "Savebox 业务运行流程", font=title_font, fill=ink)
    draw.text((72, 84), "传感监控、门锁控制、告警抓拍、状态展示与云端上报", font=subtitle_font, fill=muted)

    def section(box: tuple[int, int, int, int], title: str, fill: str, color: str) -> None:
        rounded_box(draw, box, fill, outline=color, radius=24, width=2)
        draw.text((box[0] + 26, box[1] + 20), title, font=section_font, fill=color)

    def box_node(
        box: tuple[int, int, int, int],
        title: str,
        desc: str,
        color: str,
        fill: str = "white",
    ) -> tuple[int, int, int, int]:
        rounded_box(draw, box, fill, outline=border, radius=16, width=2)
        draw.rectangle((box[0], box[1], box[0] + 10, box[3]), fill=color)
        draw.text((box[0] + 24, box[1] + 15), title, font=node_title_font, fill=ink)
        if desc:
            fitted_multiline_text(
                draw,
                (box[0] + 24, box[1] + 52, box[2] - 16, box[3] - 12),
                desc,
                14,
                11,
                body,
                spacing=3,
            )
        return box

    def diamond(center: tuple[int, int], title: str, color: str) -> tuple[int, int, int, int]:
        cx, cy = center
        w, h = 168, 96
        pts = [(cx, cy - h // 2), (cx + w // 2, cy), (cx, cy + h // 2), (cx - w // 2, cy)]
        draw.polygon(pts, fill="white")
        draw.line([pts[0], pts[1], pts[2], pts[3], pts[0]], fill=color, width=3)
        centered_text(draw, (cx - 60, cy - 28, cx + 60, cy + 28), title, node_title_font, ink)
        return (cx - w // 2, cy - h // 2, cx + w // 2, cy + h // 2)

    def connect(start: tuple[int, int], end: tuple[int, int], color: str, label: str = "") -> None:
        line_arrow(draw, start, end, color, 3)
        if label:
            mx = (start[0] + end[0]) // 2
            my = (start[1] + end[1]) // 2
            tw, th = text_size(draw, label, label_font)
            tag = (mx - tw // 2 - 14, my - th // 2 - 8, mx + tw // 2 + 14, my + th // 2 + 8)
            rounded_box(draw, tag, "white", outline="#e2e8f0", radius=8, width=1)
            centered_text(draw, tag, label, label_font, color)

    # 1. Start path
    section((60, 125, 1740, 255), "1. 启动与任务创建", "#f8fafc", "#334155")
    y = 177
    n1 = box_node((160, y, 350, y + 78), "系统启动", "进入主程序", blue)
    n2 = box_node((455, y, 665, y + 78), "初始化", "BSP + APP", blue)
    n3 = box_node((770, y, 1000, y + 78), "状态中心", "创建互斥锁", blue)
    n4 = box_node((1105, y, 1380, y + 78), "业务任务", "采集 / 控制 / 显示", blue)
    n5 = box_node((1485, y, 1680, y + 78), "联网服务", "Wi-Fi / MQTT / HTTP", blue)
    connect((350, y + 39), (455, y + 39), blue)
    connect((665, y + 39), (770, y + 39), blue)
    connect((1000, y + 39), (1105, y + 39), blue)
    connect((1380, y + 39), (1485, y + 39), blue)

    # 2. Sensor monitoring
    section((60, 285, 880, 640), "2. 传感监控与告警", "#f0fdf4", green)
    box_node((105, 355, 305, 455), "DHT11", "温湿度采集\n写入状态中心", green)
    box_node((360, 355, 560, 455), "MQ-2", "气体 ADC 采样\n增量阈值判断", green)
    box_node((615, 355, 815, 455), "SW180", "振动中断去抖\n判断报警状态", green)
    diamond((460, 535), "是否报警", red)
    box_node((115, 590, 305, 655), "蜂鸣器报警", "蜂鸣器鸣叫 2 秒", red)
    box_node((365, 590, 555, 655), "摄像头抓拍", "上传告警图像", red)
    box_node((665, 515, 835, 590), "正常运行", "继续周期采样", green)

    bus_y = 490
    draw.line((205, bus_y, 715, bus_y), fill=green, width=3)
    line_arrow(draw, (205, 455), (205, bus_y), green, 3)
    line_arrow(draw, (460, 455), (460, bus_y), green, 3)
    line_arrow(draw, (715, 455), (715, bus_y), green, 3)
    line_arrow(draw, (460, bus_y), (460, 487), green, 3)

    polyline_arrow(draw, [(430, 570), (430, 580), (210, 580), (210, 590)], red, 3)
    polyline_arrow(draw, [(460, 583), (460, 590)], red, 3)
    centered_text(draw, (320, 560, 360, 590), "是", label_font, red)
    centered_text(draw, (475, 570, 515, 600), "是", label_font, red)
    connect((540, 535), (665, 552), green, "否")

    # 3. Lock control
    section((920, 285, 1740, 640), "3. 门锁控制", "#fff7ed", orange)
    box_node((970, 360, 1170, 450), "用户刷卡", "RC522 读取卡号", orange)
    diamond((1290, 405), "白名单验证", orange)
    box_node((1495, 360, 1670, 450), "拒绝开锁", "未授权或冷却中", red)
    box_node((1495, 475, 1690, 555), "远程命令", "MQTT / UART\n锁定或解锁", orange)
    diamond((1290, 545), "当前锁状态", purple)
    box_node((1015, 585, 1210, 655), "执行开锁", "舵机 10 度", purple)
    box_node((1375, 585, 1570, 655), "执行关锁", "舵机 90 度", purple)
    connect((1170, 405), (1206, 405), orange)
    connect((1374, 405), (1495, 405), red, "失败")
    connect((1290, 453), (1290, 497), orange, "成功")
    connect((1495, 515), (1370, 545), orange)
    connect((1210, 545), (1110, 585), purple, "已锁")
    connect((1370, 545), (1470, 585), purple, "未锁")

    # 4. State display and cloud sync
    section((60, 680, 1740, 850), "4. 状态展示与云端同步", "#f5f3ff", purple)
    box_node((150, 735, 390, 825), "统一状态中心", "温湿度、气体、振动\n卡号、锁状态、摄像头", purple)
    box_node((520, 735, 735, 825), "OLED 显示", "锁状态 / 温湿度\n气体 / 振动 / 卡号", purple)
    box_node((865, 735, 1110, 825), "MQTT 上报", "周期发布状态\n响应云端命令", purple)
    box_node((1235, 735, 1485, 825), "HTTP 上传", "最新画面、告警帧\n状态快照", purple)
    box_node((1550, 735, 1710, 825), "云端服务", "IoTDA Broker\n图片服务", purple)
    connect((390, 780), (520, 780), purple)
    connect((735, 780), (865, 780), purple)
    connect((1110, 780), (1235, 780), purple)
    connect((1485, 780), (1550, 780), purple)

    img.save(ROOT / "savebox_business_flow.png")


def main() -> None:
    architecture_image()
    startup_image()
    business_flow_image()


if __name__ == "__main__":
    main()
