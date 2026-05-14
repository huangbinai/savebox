#include "app_rc522.h"
#include "string.h"


/*
 * 函数功能：通过 SPI 接口收发 1 个字节
 * 参数说明：
 *   tx_data：要发送的数据
 * 返回值：
 *   从 RC522 读到的 1 字节数据
 */
u8 RC522_SPI_ReadWriteOneByte(u8 tx_data)
{
    u8 rx_data = 0;
    HAL_SPI_TransmitReceive(&hspi1, &tx_data, &rx_data, 1, 100);
    return rx_data;
}


/*
 * 功能描述：根据卡序列号选择卡片，并读取卡容量
 * 参数说明：
 *   serNum：传入的卡序列号（4 字节）
 * 返回值：
 *   成功时返回卡容量；失败返回 0
 */
u8 RC522_MFRC522_SelectTag(u8 *serNum)
{
    u8 i;
    u8 status;
    u8 size;
    u8 recvBits;
    u8 buffer[9];

    buffer[0] = PICC_ANTICOLL1;  // 防冲突指令
    buffer[1] = 0x70;
    buffer[6] = 0x00;
    for (i = 0; i < 4; i++)
    {
        buffer[i + 2] = *(serNum + i);  // buffer[2]~buffer[5] 为卡序列号
        buffer[6] ^= *(serNum + i);     // 校验和
    }

    RC522_CalulateCRC(buffer, 7, &buffer[7]); // buffer[7]~buffer[8] 为 CRC 校验码
    RC522_ClearBitMask(Status2Reg, 0x08);
    status = RC522_PcdComMF522(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);

    if ((status == MI_OK) && (recvBits == 0x18)) size = buffer[0];
    else size = 0;

    return size;
}


/*
 * 函数功能：RC522 芯片初始化
 */
void RC522_Init(void)
{
    RC522_PcdReset();          // 复位 RC522
    RC522_PcdAntennaOff();     // 关闭天线
    delay_ms(2);               // 延时 2 ms
    RC522_PcdAntennaOn();      // 开启天线
    M500PcdConfigISOType('A'); // 配置为 ISO14443A 工作方式
}


/*
 * 函数功能：重新初始化 RC522
 */
void RC522_Reset(void)
{
    RC522_PcdReset();      // 复位 RC522
    RC522_PcdAntennaOff(); // 关闭天线
    delay_ms(2);           // 延时 2 ms
    RC522_PcdAntennaOn();  // 开启天线
}


/*
 * 功能：寻卡
 * 参数说明：
 *   req_code[IN]：寻卡模式
 *       0x52 = 寻找当前感应区内所有符合 14443A 标准的卡
 *       0x26 = 寻找未进入休眠状态的卡
 *   pTagType[OUT]：返回的卡片类型代码
 *       0x4400 = Mifare_UltraLight
 *       0x0400 = Mifare_One(S50)
 *       0x0200 = Mifare_One(S70)
 *       0x0800 = Mifare_Pro(X)
 *       0x4403 = Mifare_DESFire
 * 返回值：
 *   成功返回 MI_OK，失败返回 MI_ERR
 */
char RC522_PcdRequest(u8 req_code, u8 *pTagType)
{
    char status;
    u8 unLen;
    u8 ucComMF522Buf[MAXRLEN]; // MAXRLEN = 18

    RC522_ClearBitMask(Status2Reg, 0x08);    // 清除 Status2Reg 中的 MFCryptOn 位
    RC522_WriteRawRC(BitFramingReg, 0x07);   // 设置发送和接收数据的位数
    RC522_SetBitMask(TxControlReg, 0x03);    // 打开发送天线

    ucComMF522Buf[0] = req_code;             // 寻卡命令

    // 通过 RC522 与 ISO14443 卡片通信
    status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 1, ucComMF522Buf, &unLen);

    if ((status == MI_OK) && (unLen == 0x10))
    {
        *pTagType = ucComMF522Buf[0];
        *(pTagType + 1) = ucComMF522Buf[1];
    }
    else status = MI_ERR;

    return status;
}


/*
 * 功能：防冲突，读取卡序列号
 * 参数说明：
 *   pSnr[OUT]：卡序列号，4 字节
 * 返回值：
 *   成功返回 MI_OK，失败返回 MI_ERR
 */
char RC522_PcdAnticoll(u8 *pSnr)
{
    char status;
    u8 i, snr_check = 0;
    u8 unLen;
    u8 ucComMF522Buf[MAXRLEN];

    RC522_ClearBitMask(Status2Reg, 0x08);  // 清除 MFCryptOn
    RC522_WriteRawRC(BitFramingReg, 0x00); // 全字节对齐
    RC522_ClearBitMask(CollReg, 0x80);     // 允许位冲突检测

    ucComMF522Buf[0] = PICC_ANTICOLL1;     // 0x93
    ucComMF522Buf[1] = 0x20;

    /*
     * 通过 RC522 与 ISO14443 卡片通信
     * PCD_TRANSCEIVE：发送并接收数据
     * InLenByte = 2：向卡片发送 2 字节数据
     * ucComMF522Buf：发送和接收缓冲区
     * unLen：接收到的数据位数
     */
    status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 2, ucComMF522Buf, &unLen);

    if (status == MI_OK)
    {
        for (i = 0; i < 4; i++)
        {
            *(pSnr + i) = ucComMF522Buf[i]; // 将读出的序列号复制到 pSnr
            snr_check ^= ucComMF522Buf[i];
        }
        if (snr_check != ucComMF522Buf[i]) status = MI_ERR;
    }
    RC522_SetBitMask(CollReg, 0x80); // 重新启用冲突检测
    return status;
}


/*
 * 功能：选定卡片
 * 参数说明：
 *   pSnr[IN]：卡片序列号，4 字节
 * 返回值：
 *   成功返回 MI_OK，失败返回 MI_ERR
 */
char RC522_PcdSelect(u8 *pSnr)
{
    char status;
    u8 i;
    u8 unLen;
    u8 ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_ANTICOLL1;
    ucComMF522Buf[1] = 0x70;
    ucComMF522Buf[6] = 0;

    for (i = 0; i < 4; i++)
    {
        ucComMF522Buf[i + 2] = *(pSnr + i);
        ucComMF522Buf[6] ^= *(pSnr + i);
    }

    // 使用 RC522 计算 CRC16 校验
    RC522_CalulateCRC(ucComMF522Buf, 7, &ucComMF522Buf[7]);
    RC522_ClearBitMask(Status2Reg, 0x08); // 清除 MFCryptOn

    status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 9, ucComMF522Buf, &unLen);
    if ((status == MI_OK) && (unLen == 0x18)) status = MI_OK;
    else status = MI_ERR;

    return status;
}


/*
 * 功能：验证卡片密码
 * 参数说明：
 *   auth_mode[IN]：验证模式
 *       0x60 = 验证 A 密钥
 *       0x61 = 验证 B 密钥
 *   addr[IN]     ：块地址
 *   pKey[IN]     ：扇区密钥 6 字节
 *   pSnr[IN]     ：卡片序列号 4 字节
 * 返回值：
 *   成功返回 MI_OK，失败返回 MI_ERR
 */
char RC522_PcdAuthState(u8 auth_mode, u8 addr, u8 *pKey, u8 *pSnr)
{
    char status;
    u8 unLen;
    u8 ucComMF522Buf[MAXRLEN]; // MAXRLEN = 18

    // 验证命令 + 块地址 + 扇区密钥 + 卡序列号
    ucComMF522Buf[0] = auth_mode;
    ucComMF522Buf[1] = addr;
    memcpy(&ucComMF522Buf[2], pKey, 6);
    memcpy(&ucComMF522Buf[8], pSnr, 4);

    status = RC522_PcdComMF522(PCD_AUTHENT, ucComMF522Buf, 12, ucComMF522Buf, &unLen);
    if ((status != MI_OK) || (!(RC522_ReadRawRC(Status2Reg) & 0x08))) status = MI_ERR;
    return status;
}


/*
 * 功能：读取 M1 卡一个块的数据
 * 参数说明：
 *   addr[IN]：块地址
 *   p[OUT]  ：读取到的数据缓冲区，长度 16 字节
 * 返回值：
 *   成功返回 MI_OK，失败返回 MI_ERR
 */
char RC522_PcdRead(u8 addr, u8 *p)
{
    char status;
    u8 unLen;
    u8 i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_READ;
    ucComMF522Buf[1] = addr;
    RC522_CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    // 通过 RC522 与 M1 卡通信
    status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    if ((status == MI_OK) && (unLen == 0x90))
    {
        for (i = 0; i < 16; i++) *(p + i) = ucComMF522Buf[i];
    }
    else status = MI_ERR;

    return status;
}


/*
 * 功能：向 M1 卡指定块写入 16 字节数据
 * 参数说明：
 *   addr[IN]：块地址
 *   p[IN]   ：要写入的数据缓冲区，长度 16 字节
 * 返回值：
 *   成功返回 MI_OK，失败返回 MI_ERR
 */
char RC522_PcdWrite(u8 addr, u8 *p)
{
    char status;
    u8 unLen;
    u8 i, ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_WRITE; // 0xA0，写块命令
    ucComMF522Buf[1] = addr;       // 块地址
    RC522_CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);

    status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A)) status = MI_ERR;

    if (status == MI_OK)
    {
        // 向 FIFO 写入 16 字节数据
        for (i = 0; i < 16; i++) ucComMF522Buf[i] = *(p + i);
        RC522_CalulateCRC(ucComMF522Buf, 16, &ucComMF522Buf[16]);
        status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 18, ucComMF522Buf, &unLen);
        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A)) status = MI_ERR;
    }
    return status;
}


/*
 * 功能：命令卡片进入休眠状态
 * 返回值：
 *   成功返回 MI_OK，失败返回 MI_ERR
 */
char RC522_PcdHalt()
{
    u8 status = MI_ERR;
    u8 unLen;
    u8 ucComMF522Buf[MAXRLEN];

    ucComMF522Buf[0] = PICC_HALT;
    ucComMF522Buf[1] = 0;
    RC522_CalulateCRC(ucComMF522Buf, 2, &ucComMF522Buf[2]);
    status = RC522_PcdComMF522(PCD_TRANSCEIVE, ucComMF522Buf, 4, ucComMF522Buf, &unLen);
    return status;
}


/*
 * 功能：使用 RC522 自带的 CRC 计算单元计算 CRC16
 * 参数说明：
 *   pIn [IN] ：输入数据缓冲区
 *   len [IN] ：输入数据长度（字节）
 *   pOut[OUT]：输出 CRC 结果，两字节
 */
void RC522_CalulateCRC(u8 *pIn, u8 len, u8 *pOut)
{
    u8 i, n;
    RC522_ClearBitMask(DivIrqReg, 0x04); // 清除 CRCIrq 标志
    RC522_WriteRawRC(CommandReg, PCD_IDLE);
    RC522_SetBitMask(FIFOLevelReg, 0x80); // 清 FIFO 指针

    // 将数据写入 FIFO
    for (i = 0; i < len; i++) RC522_WriteRawRC(FIFODataReg, *(pIn + i));

    // 启动 CRC 计算
    RC522_WriteRawRC(CommandReg, PCD_CALCCRC);
    i = 0xFF;
    do
    {
        n = RC522_ReadRawRC(DivIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x04)); // 等待 CRCIrq 置位

    // 读取 CRC 计算结果
    pOut[0] = RC522_ReadRawRC(CRCResultRegL);
    pOut[1] = RC522_ReadRawRC(CRCResultRegM);
}


/*
 * 功能：硬件复位 RC522
 * 返回值：
 *   成功返回 MI_OK
 */
char RC522_PcdReset()
{
    if (RC522_RST_Pin != GPIO_NUM_NC)
    {
        RC522_RST(1);
        delay_ms(10);
        RC522_RST(0);
        delay_us(1);
        RC522_RST(1);
        delay_us(1);
    }
    else
    {
        delay_ms(10);
    }

    // 复位命令
    RC522_WriteRawRC(CommandReg, PCD_RESETPHASE);
    RC522_WriteRawRC(CommandReg, PCD_RESETPHASE);
    delay_us(1);

    // 和 Mifare 卡通信时的一些默认配置
    RC522_WriteRawRC(ModeReg, 0x3D);
    RC522_WriteRawRC(TReloadRegL, 30);
    RC522_WriteRawRC(TReloadRegH, 0);
    RC522_WriteRawRC(TModeReg, 0x8D);
    RC522_WriteRawRC(TPrescalerReg, 0x3E);

    RC522_WriteRawRC(TxAutoReg, 0x40); // 必须设置
    return MI_OK;
}


/*
 * 函数功能：配置 RC522 的工作模式（ISO14443 类型）
 * 参数说明：
 *   type：'A' 表示 ISO14443_A
 * 返回值：
 *   成功返回 MI_OK，失败返回 1
 */
char M500PcdConfigISOType(u8 type)
{
    if (type == 'A') // ISO14443_A
    {
        RC522_ClearBitMask(Status2Reg, 0x08);
        RC522_WriteRawRC(ModeReg, 0x3D);
        RC522_WriteRawRC(RxSelReg, 0x86);
        RC522_WriteRawRC(RFCfgReg, 0x7F);
        RC522_WriteRawRC(TReloadRegL, 30);
        RC522_WriteRawRC(TReloadRegH, 0);
        RC522_WriteRawRC(TModeReg, 0x8D);
        RC522_WriteRawRC(TPrescalerReg, 0x3E);
        delay_us(1);
        RC522_PcdAntennaOn(); // 开启天线
    }
    else return 1; // 失败

    return MI_OK;  // 成功
}


/*
 * 功能：读取 RC522 寄存器
 * 参数说明：
 *   Address[IN]：寄存器地址
 * 返回值：
 *   读到的寄存器值
 */
u8 RC522_ReadRawRC(u8 Address)
{
    u8 ucAddr;
    u8 ucResult = 0;
    RC522_CS(0); // 片选 RC522
    ucAddr = ((Address << 1) & 0x7E) | 0x80;
    RC522_SPI_ReadWriteOneByte(ucAddr);       // 发送地址命令
    ucResult = RC522_SPI_ReadWriteOneByte(0); // 读取返回数据
    RC522_CS(1);                              // 释放片选
    return ucResult;
}


/*
 * 功能：向 RC522 寄存器写入数据
 * 参数说明：
 *   Address[IN]：寄存器地址
 *   value  [IN]：要写入的值
 */
void RC522_WriteRawRC(u8 Address, u8 value)
{
    u8 ucAddr;
    RC522_CS(0); // 片选
    ucAddr = ((Address << 1) & 0x7E);
    RC522_SPI_ReadWriteOneByte(ucAddr); // 发送地址
    RC522_SPI_ReadWriteOneByte(value);  // 发送数据
    RC522_CS(1);                        // 释放片选
}


/*
 * 功能：对 RC522 寄存器设置指定位（置 1）
 * 参数说明：
 *   reg [IN]：寄存器地址
 *   mask[IN]：需要置 1 的位掩码
 */
void RC522_SetBitMask(u8 reg, u8 mask)
{
    char tmp = 0x0;
    tmp = RC522_ReadRawRC(reg);
    RC522_WriteRawRC(reg, tmp | mask);
}


/*
 * 功能：对 RC522 寄存器清除指定位（置 0）
 * 参数说明：
 *   reg [IN]：寄存器地址
 *   mask[IN]：需要清零的位掩码
 */
void RC522_ClearBitMask(u8 reg, u8 mask)
{
    char tmp = 0x0;
    tmp = RC522_ReadRawRC(reg);
    RC522_WriteRawRC(reg, tmp & ~mask);
}


/*
 * 功能：通过 RC522 与 ISO14443 卡片进行通信
 * 参数说明：
 *   Command      [IN]：RC522 命令字
 *   pIn          [IN]：发送到卡片的数据缓冲区
 *   InLenByte    [IN]：发送数据的字节数
 *   pOut         [OUT]：接收到的卡片返回数据缓冲区
 *   pOutLenBit   [OUT]：返回数据的位长度
 * 返回值：
 *   MI_OK / MI_ERR / MI_NOTAGERR
 */
char RC522_PcdComMF522(u8 Command, u8 *pIn, u8 InLenByte, u8 *pOut, u8 *pOutLenBit)
{
    char status = MI_ERR;
    u8 irqEn = 0x00;
    u8 waitFor = 0x00;
    u8 lastBits;
    u8 n;
    u16 i;

    switch (Command)
    {
    case PCD_AUTHENT:    // 验证密钥
        irqEn = 0x12;
        waitFor = 0x10;
        break;
    case PCD_TRANSCEIVE: // 发送并接收数据
        irqEn = 0x77;
        waitFor = 0x30;
        break;
    default:
        break;
    }
    RC522_WriteRawRC(ComIEnReg, irqEn | 0x80);
    RC522_ClearBitMask(ComIrqReg, 0x80);   // 清除全部中断标志
    RC522_WriteRawRC(CommandReg, PCD_IDLE);
    RC522_SetBitMask(FIFOLevelReg, 0x80);  // 清 FIFO

    // 将数据写入 FIFO
    for (i = 0; i < InLenByte; i++)
    {
        RC522_WriteRawRC(FIFODataReg, pIn[i]);
    }

    // 执行命令
    RC522_WriteRawRC(CommandReg, Command);
    if (Command == PCD_TRANSCEIVE) RC522_SetBitMask(BitFramingReg, 0x80); // 启动发送

    // 轮询等待命令完成
    // i 的取值可根据时钟频率调整，此处约 5ms 超时
    i = 2000;
    do
    {
        n = RC522_ReadRawRC(ComIrqReg);
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitFor));

    RC522_ClearBitMask(BitFramingReg, 0x80);

    if (i != 0)
    {
        if (!(RC522_ReadRawRC(ErrorReg) & 0x1B))
        {
            status = MI_OK;
            if (n & irqEn & 0x01) status = MI_NOTAGERR;
            if (Command == PCD_TRANSCEIVE)
            {
                n = RC522_ReadRawRC(FIFOLevelReg);
                lastBits = RC522_ReadRawRC(ControlReg) & 0x07;
                if (lastBits) *pOutLenBit = (n - 1) * 8 + lastBits;
                else *pOutLenBit = n * 8;

                if (n == 0) n = 1;
                if (n > MAXRLEN) n = MAXRLEN;
                for (i = 0; i < n; i++) pOut[i] = RC522_ReadRawRC(FIFODataReg);
            }
        }
        else status = MI_ERR;
    }
    RC522_SetBitMask(ControlReg, 0x80); // 停止定时器
    RC522_WriteRawRC(CommandReg, PCD_IDLE);
    return status;
}


/*
 * 函数功能：打开天线
 * 说明：每次打开或关闭天线之间应保证至少 1ms 间隔
 */
void RC522_PcdAntennaOn()
{
    u8 i;
    i = RC522_ReadRawRC(TxControlReg);
    if (!(i & 0x03)) RC522_SetBitMask(TxControlReg, 0x03);
}


/*
 * 函数功能：关闭天线
 * 说明：每次打开或关闭天线之间应保证至少 1ms 间隔
 */
void RC522_PcdAntennaOff()
{
    RC522_ClearBitMask(TxControlReg, 0x03);
}
