from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent
FONT_REGULAR = "C:/Windows/Fonts/msyh.ttc"
FONT_BOLD = "C:/Windows/Fonts/msyhbd.ttc"


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(FONT_BOLD if bold else FONT_REGULAR, size=size)


def round_box(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], fill, outline, radius=18, width=2) -> None:
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def text_size(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.ImageFont) -> tuple[int, int]:
    box = draw.multiline_textbbox((0, 0), text, font=fnt, spacing=4)
    return box[2] - box[0], box[3] - box[1]


def wrap_text(draw: ImageDraw.ImageDraw, text: str, fnt: ImageFont.ImageFont, max_width: int) -> str:
    result: list[str] = []
    for raw in text.split("\n"):
        line = ""
        for ch in raw:
            candidate = line + ch
            if text_size(draw, candidate, fnt)[0] <= max_width or not line:
                line = candidate
            else:
                result.append(line)
                line = ch
        result.append(line)
    return "\n".join(result)


def fitted_text(
    draw: ImageDraw.ImageDraw,
    xy: tuple[int, int],
    text: str,
    max_width: int,
    max_height: int,
    max_size: int,
    min_size: int,
    fill: str,
    bold: bool = False,
) -> None:
    x, y = xy
    for size in range(max_size, min_size - 1, -1):
        fnt = font(size, bold)
        wrapped = wrap_text(draw, text, fnt, max_width)
        _, h = text_size(draw, wrapped, fnt)
        if h <= max_height:
            draw.multiline_text((x, y), wrapped, font=fnt, fill=fill, spacing=4)
            return
    fnt = font(min_size, bold)
    draw.multiline_text((x, y), wrap_text(draw, text, fnt, max_width), font=fnt, fill=fill, spacing=3)


def arrow_down(draw: ImageDraw.ImageDraw, x: int, y1: int, y2: int, color: str) -> None:
    draw.line((x, y1, x, y2), fill=color, width=4)
    draw.polygon([(x, y2), (x - 8, y2 - 13), (x + 8, y2 - 13)], fill=color)


def arrow_right(draw: ImageDraw.ImageDraw, x1: int, y: int, x2: int, color: str) -> None:
    draw.line((x1, y, x2, y), fill=color, width=4)
    draw.polygon([(x2, y), (x2 - 13, y - 8), (x2 - 13, y + 8)], fill=color)


def draw_title(draw: ImageDraw.ImageDraw, text: str, x: int, y: int, orange: str) -> None:
    draw.text((x, y), text, font=font(30, True), fill=orange)
    draw.line((x, y + 44, x + 138, y + 44), fill=orange, width=4)


def startup_image() -> None:
    img = Image.new("RGBA", (760, 430), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)
    orange = "#ff7a1a"
    dark = "#263241"
    muted = "#607083"
    blue = "#2f6fed"
    stroke = "#cbd5e1"
    fill = (255, 255, 255, 215)

    draw_title(draw, "启动流程", 20, 18, orange)
    draw.text((176, 26), "app_main 到 FreeRTOS 多任务运行", font=font(16), fill=muted)

    steps = [
        ("01", "系统入口", "app_main() 启动系统"),
        ("02", "板级初始化", "BSP_Init() 初始化平台、定时器和 OLED"),
        ("03", "外设初始化", "APP_Init() 初始化 Camera 等功能模块"),
        ("04", "状态中心", "task_state_init() 创建运行态状态中心"),
        ("05", "任务创建", "TASK_StartAll() 启动采集、控制、显示任务"),
        ("06", "联网通信", "savebox_mqtt_start() 启动 Wi-Fi / MQTT / HTTP"),
    ]

    x0, y0 = 38, 86
    card_w, card_h, gap = 650, 40, 10
    for i, (num, title, desc) in enumerate(steps):
        y = y0 + i * (card_h + gap)
        round_box(draw, (x0 + 42, y, x0 + 42 + card_w, y + card_h), fill, stroke, radius=14, width=2)
        draw.ellipse((x0, y + 3, x0 + 34, y + 37), fill=orange if i in (0, 5) else blue)
        draw.text((x0 + 6, y + 9), num, font=font(12, True), fill="white")
        draw.text((x0 + 66, y + 8), title, font=font(17, True), fill=dark)
        fitted_text(draw, (x0 + 188, y + 11), desc, 430, 21, 13, 10, muted)
        if i < len(steps) - 1:
            arrow_down(draw, x0 + 17, y + 38, y + card_h + gap - 3, "#9aa8b8")

    round_box(draw, (38, 395, 728, 422), (255, 247, 237, 230), "#ffd4b3", radius=12, width=1)
    draw.text((58, 400), "顺序特点：先底层与外设初始化，再创建任务，最后启动联网能力。", font=font(13), fill=dark)

    img.save(ROOT / "slide21_startup_flow.png")


def task_division_image() -> None:
    img = Image.new("RGBA", (820, 430), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)
    orange = "#ff7a1a"
    dark = "#263241"
    muted = "#607083"
    stroke = "#cbd5e1"
    green = "#16a34a"
    blue = "#2563eb"
    purple = "#7c3aed"
    red = "#dc2626"

    draw_title(draw, "任务划分", 20, 18, orange)
    draw.text((166, 26), "FreeRTOS 多任务解耦采集、控制、显示和通信", font=font(16), fill=muted)

    groups = [
        ("感知采集", green, [
            ("task_dht11", "温湿度周期采集"),
            ("task_mq2", "气体/烟雾阈值判断"),
            ("task_sw180", "振动检测与异常触发"),
            ("task_rc522", "RFID 刷卡识别"),
        ]),
        ("执行控制", red, [
            ("task_servo", "舵机门锁开关控制"),
            ("task_buzzer", "蜂鸣器报警队列"),
        ]),
        ("显示与通信", blue, [
            ("task_oled", "本地状态刷新显示"),
            ("task_camera", "画面上传与报警抓拍"),
            ("savebox_mqtt", "状态上报与命令订阅"),
        ]),
    ]

    col_x = [28, 294, 560]
    col_w = 232
    top = 92
    for idx, (name, color, items) in enumerate(groups):
        x = col_x[idx]
        h = 294
        round_box(draw, (x, top, x + col_w, top + h), (255, 255, 255, 218), stroke, radius=18, width=2)
        draw.rectangle((x, top, x + col_w, top + 8), fill=color)
        draw.text((x + 18, top + 20), name, font=font(21, True), fill=color)

        y = top + 62
        for task, desc in items:
            round_box(draw, (x + 16, y, x + col_w - 16, y + 48), (248, 250, 252, 235), "#e2e8f0", radius=10, width=1)
            draw.text((x + 28, y + 8), task, font=font(14, True), fill=dark)
            fitted_text(draw, (x + 28, y + 28), desc, col_w - 62, 17, 11, 9, muted)
            y += 56

    # Shared runtime state center.
    round_box(draw, (150, 394, 670, 422), (255, 247, 237, 230), "#ffd4b3", radius=12, width=1)
    draw.text((190, 400), "共享状态中心：task_state 统一保存传感器、门锁、卡号和摄像头状态", font=font(12), fill=dark)

    img.save(ROOT / "slide21_task_division.png")


def main() -> None:
    startup_image()
    task_division_image()
    print(ROOT / "slide21_startup_flow.png")
    print(ROOT / "slide21_task_division.png")


if __name__ == "__main__":
    main()
