from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parent
OUT = ROOT / "slide8_system_link_transparent.png"
FONT_REGULAR = "C:/Windows/Fonts/msyh.ttc"
FONT_BOLD = "C:/Windows/Fonts/msyhbd.ttc"


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(FONT_BOLD if bold else FONT_REGULAR, size=size)


def text_center(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], text: str, size: int, fill: str, bold: bool = False) -> None:
    fnt = font(size, bold)
    bbox = draw.multiline_textbbox((0, 0), text, font=fnt, spacing=4, align="center")
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]
    x = box[0] + (box[2] - box[0] - w) / 2
    y = box[1] + (box[3] - box[1] - h) / 2
    draw.multiline_text((x, y), text, font=fnt, fill=fill, spacing=4, align="center")


def round_box(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], fill, outline: str, width: int = 2, radius: int = 18) -> None:
    draw.rounded_rectangle(box, radius=radius, fill=fill, outline=outline, width=width)


def arrow(draw: ImageDraw.ImageDraw, start: tuple[int, int], end: tuple[int, int], color: str) -> None:
    draw.line([start, end], fill=color, width=5)
    x, y = end
    draw.polygon([(x, y), (x - 10, y - 16), (x + 10, y - 16)], fill=color)


def chip_icon(draw: ImageDraw.ImageDraw, x: int, y: int, color: str) -> None:
    draw.rounded_rectangle((x, y, x + 34, y + 34), radius=6, fill=color)
    for i in range(5):
        yy = y + 5 + i * 6
        draw.line((x - 6, yy, x, yy), fill=color, width=2)
        draw.line((x + 34, yy, x + 40, yy), fill=color, width=2)


def wifi_icon(draw: ImageDraw.ImageDraw, x: int, y: int, color: str) -> None:
    draw.arc((x, y, x + 50, y + 42), 205, 335, fill=color, width=4)
    draw.arc((x + 10, y + 11, x + 40, y + 42), 210, 330, fill=color, width=4)
    draw.ellipse((x + 22, y + 34, x + 28, y + 40), fill=color)


def cloud_icon(draw: ImageDraw.ImageDraw, x: int, y: int, color: str) -> None:
    draw.ellipse((x + 8, y + 15, x + 34, y + 41), outline=color, width=4)
    draw.ellipse((x + 28, y + 8, x + 58, y + 41), outline=color, width=4)
    draw.ellipse((x + 50, y + 17, x + 76, y + 43), outline=color, width=4)
    draw.line((x + 15, y + 41, x + 69, y + 41), fill=color, width=4)


def ai_icon(draw: ImageDraw.ImageDraw, x: int, y: int, color: str) -> None:
    draw.arc((x, y, x + 42, y + 46), 82, 292, fill=color, width=4)
    draw.line((x + 18, y + 42, x + 18, y + 50), fill=color, width=4)
    draw.text((x + 48, y + 12), "AI", font=font(22, True), fill=color)


def main() -> None:
    img = Image.new("RGBA", (690, 500), (255, 255, 255, 0))
    draw = ImageDraw.Draw(img)

    orange = "#ff5a00"
    dark = "#202938"
    muted = "#4b5563"
    stroke = "#ff9b63"
    label_fill = (255, 247, 237, 220)

    draw.text((36, 26), "系统链路图", font=font(30, True), fill=dark)
    draw.text((38, 68), "从本地安防到多端云端管理", font=font(16), fill=muted)

    cards = [
        ((62, 112, 626, 172), "1", "本地设备端", "ESP32-S3、传感器、门锁、蜂鸣器、摄像头", "chip"),
        ((62, 190, 626, 250), "2", "运行状态中心", "task_state 汇总温湿度、气体、振动、卡号、锁状态", "chip"),
        ((62, 268, 626, 328), "3", "通信传输端", "Wi-Fi、MQTT 上报/命令下发、HTTP 图片上传", "wifi"),
        ((62, 346, 626, 406), "4", "平台与应用端", "华为云 IoTDA、后端服务、Web 看板、小程序", "cloud"),
        ((62, 424, 626, 474), "5", "智能端", "AI 风险分析、预警建议", "ai"),
    ]

    cx = 344
    for index, (box, num, title, desc, icon) in enumerate(cards):
        round_box(draw, box, (255, 255, 255, 0), stroke, width=2, radius=14)
        draw.ellipse((box[0] + 18, box[1] + 14, box[0] + 54, box[1] + 50), fill=orange)
        text_center(draw, (box[0] + 18, box[1] + 14, box[0] + 54, box[1] + 50), num, 18, "white", True)

        draw.text((box[0] + 74, box[1] + 10), title, font=font(21, True), fill=orange)
        draw.text((box[0] + 74, box[1] + 40), desc, font=font(13), fill=dark)

        if icon == "chip":
            chip_icon(draw, box[2] - 90, box[1] + 14, orange)
        elif icon == "wifi":
            wifi_icon(draw, box[2] - 100, box[1] + 12, orange)
        elif icon == "cloud":
            cloud_icon(draw, box[2] - 118, box[1] + 8, orange)
        else:
            ai_icon(draw, box[2] - 120, box[1] + 5, orange)

        if index < len(cards) - 1:
            arrow(draw, (cx, box[3] + 2), (cx, box[3] + 24), orange)

    labels = [
        (392, 176, "状态/告警/图片"),
        (392, 254, "MQTT/HTTP"),
        (392, 332, "云端分发"),
        (392, 410, "风险分析"),
    ]
    for x, y, text in labels:
        draw.rounded_rectangle((x, y, x + 126, y + 24), radius=8, fill=label_fill, outline=stroke)
        text_center(draw, (x, y, x + 126, y + 24), text, 12, orange, True)

    img.save(OUT)
    print(OUT)


if __name__ == "__main__":
    main()
