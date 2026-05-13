#include "bsp_platform.h"

#include <string.h>
#include "freertos/FreeRTOS.h"

#include "savebox_board.h"

/*
 * 闁哄牜鍓氶弸鍐╃閺堢數绋婇柣銏╃厜缁辨瑥顕ラ崼銉ユ閻熸洑绶ょ槐姘舵晬? * 1) 闁圭粯鍔掔欢?savebox 濡炪倕婀卞ú浼存儍閸曨偊鎸柛娆愭緲閻?闁哄娉曟鍥绩椤栨稑鐦?BSP)闁告帗绻傞～鎰板礌閺嶇數绐桮PIO闁靛棔璁I 缂佹稑顦ˇ鑽ゆ媼閸撗勭暠闁糕晞娅ｉ、鍛存煀瀹ュ洨鏋傞柕? * 2) 闁圭粯鍔掔欢鍨▔閳ь剛绱?HAL_XXX 闁稿繒鍘ч鎰板箳閵夈儱缍撻柨娑樼墕閹筹繝宕ュ鍛闁活潿鍔嶇涵鍓佺尵鐠佽櫕濡?STM32 HAL闁挎稑顧€缁? *    閻犱讲鏅欑粭鍌滀沪閸屾瑧鐟归柛鏂衡偓鎻掓暕闁活喕绀侀弫鏍煂韫囧海鐟濋柣鈺佺摠鐢瓨绗熷┑濠勵洬 ESP-IDF 闁汇劌瀚崣鎸庢媴?API闁? *
 * 婵炲鍔嶉崜浼存晬? * - ESP-IDF 闁哄牜鍓濋棅鈺傛交閹邦垼鏀介柛?FreeRTOS 濞戞挸顭槐閬嶅嫉椤掍焦鐎ù鐘虫构缁楀鎷归悢鑽ょ厬闁告凹鍨版慨鈺冩嫬閸愩劌顔婇柛锝庣厜缁辨繈宕ｉ鍥╊槹閻犳劧绲介ˇ鑽ゆ媼閹冪仴濠殿喖顑呯€垫煡宕畝鍕ㄥ亾閸岀偛甯抽柕? * - 閺夆晜鐟╅崳鐑芥儍?TIM_HandleTypeDef/SPI_HandleTypeDef/UART_HandleTypeDef 闁哄嫷鍨堕妴宥夋儎椤斿灝娈伴悗瑙勭煯缁犵喖鎯冮崟顐㈢稏闁哄苯瀚划銊╁几閸曞墎绀? *   闁活潿鍔嶅鐢稿箵韫囨艾鐗?LEDC(Savebox PWM)/SPI/UART 缂佹稑顦ˇ鑽ゆ媼閸撗勭暠闂佹澘绉堕悿鍡樼▔鎼达絽笑闁诡兛闄嶉埀? */

/*
 * PWM闁挎稑婀DC闁挎稑顦顒勫极鐢喚绐? * - duty_resolution: 12bit => 閻犱讲鍓濋弳鐔兼嚑閸愩劍绾?0..4095
 * - period: 4095 濞寸媴缍€閵嗗啯绋夐埀顒佺▔椤忓嫭鍣柡鍫㈠枎閸炴挳鎯冮崟顒佷粯濠㈠爢鍡╁悁闁轰焦婢橀埀顒傘€嬬槐婵囩▔?12bit 闁告帒妫滄ご鎼佹偝閸パ冪埍闂? */
#define SAVEBOX_PWM_DUTY_RESOLUTION LEDC_TIMER_12_BIT
#define SAVEBOX_PWM_PERIOD 4095U

/*
 * 妤犵偛鍟胯ぐ鎾礆濠靛棭娼楅柛鏍ㄧ墵濡插鏌屽鍛汲闁哄秴娲ょ换鏃堟晬? * - true 閻炴稏鍔庨妵?savebox_platform_init() 鐎圭寮堕崹姘跺礉閻旂鈷旈悶娑樼焷缁诲啴鏁嶇仦鑺ュ€电紓渚囧弨閻ㄧ喖鎮介妸褎绾柟鎭掑劥缁绘垿宕?ESP_OK闁? */
static bool s_platform_initialized = false;

/*
 * htim1~htim4闁挎稒鐭禍鎺斺偓瑙勭濡炲倿宕抽妸銉ョ稏闁哄苯瀚▓鎴ｃ亹閵忕姷纭€闁硅绻楅崼?PWM 閺夊牊鎸搁崵顓㈠Υ? * 閺夆晜鐟╅崳椋庘偓鍦仱濡绢垶寮伴悩鑼闁?ESP32 闁?LEDC 濠㈣埖鐗為鏇㈡晬? * - speed_mode: 濞达絽閰ｉ埀顒傚枑鑶╃€殿喖楠忕槐姗狤DC_LOW_SPEED_MODE闁? * - timer_num : LEDC 閻庤纰嶅鍌炲闯閵娧呮そ闁告瑥鍤栫槐姗狤DC_TIMER_0..3闁? * - channel   : LEDC 闂侇偅宀告禍鍓х磽閺嵮冨▏闁挎稑婀DC_CHANNEL_0..7闁? * - gpio_num  : 閺夊牊鎸搁崵?PWM 闁?GPIO闁挎稑婀橮IO_NUM_NC 閻炴稏鍔庨妵姘跺嫉椤忓棛鎷ㄩ悗?闁哄牜浜欐繛鍥偨椤帞绀? * - frequency_hz: PWM 濡増鍨瑰濂告晬閸︽闁挎稑顧€缁辫鲸绋?0 闁哄啯鍎奸缁樼▔閻戞ɑ寮撻梺鏉跨Ф閻? * - period    : 濞?duty_resolution 閻庨潧缍婄紞鍫ユ儍閸曨偅鍣柡鍫㈠枙椤撴悂寮０浣虹憪闂? * - pwm_started: 闁哄嫷鍨伴幆浣割啅閹绘帞鏆氶柟?LEDC timer/channel 闂佹澘绉堕悿鍡涙晬閸垺鏆忓ù婊冮叄娴尖晠宕楀澶婃濠㈣泛绉撮崹鍨叏鐎ｎ亜顕ч柨? */
TIM_HandleTypeDef htim1 = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_2,
    .channel = LEDC_CHANNEL_2,
    .gpio_num = GPIO_NUM_NC,
    .frequency_hz = 0,
    .period = SAVEBOX_PWM_PERIOD,
    .pwm_started = false,
};

TIM_HandleTypeDef htim2 = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_3,
    .channel = LEDC_CHANNEL_3,
    .gpio_num = GPIO_NUM_NC,
    .frequency_hz = 0,
    .period = SAVEBOX_PWM_PERIOD,
    .pwm_started = false,
};

/* 闁肩鐏氬┃鈧?PWM闁挎稒姘ㄧ划锔锯偓瑙勮壘閸?SAVEBOX_SERVO_GPIO闁挎稑鑻懟鐔煎捶閵娿儱缍栭柡灞藉閼垫垶锛愰崟顓犳瀭濡増鍨瑰?SAVEBOX_SERVO_PWM_FREQ_HZ闁挎稑鐗撻埀顒佽壘閻?50Hz闁挎稑顦埀?*/
TIM_HandleTypeDef htim3 = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .timer_num = LEDC_TIMER_1,
    .channel = LEDC_CHANNEL_1,
    .gpio_num = SAVEBOX_SERVO_GPIO,
    .frequency_hz = SAVEBOX_SERVO_PWM_FREQ_HZ,
    .period = SAVEBOX_PWM_PERIOD,
    .pwm_started = false,
};



/*
 * SPI 闁告瑣鍎查悞娲晬濮樿鲸鏆忓ù?RC522闁挎稑婀〧ID闁挎稑顦拌啯闁? * - host: SPI 濞戞挾绮┃鈧柟璨夊啫鐓戦柛锝庣厜缁辨瑦淇?SPI2_HOST/HSPI_HOST 缂佹稑顧€缁辨繈鎮?savebox_board.h 閻庤鐭粻鐔兼晬? * - device: spi_bus_add_device() 闁告艾楠哥欢閬嶅礆閹殿喗鐣遍悹浣瑰劤椤︻剟宕ｉ妷锔惧姶
 * - initialized: 闁哄嫷鍨伴幆浣衡偓鐟版湰閸ㄦ碍绂?bus 闁告帗绻傞～鎰板礌閺嶏妇鐟?device 婵烇綀顕ф慨? */
SPI_HandleTypeDef hspi1 = {
    .host = SAVEBOX_RC522_SPI_HOST,
    .device = NULL,
    .initialized = false,
};

/*
 * UART 闁告瑣鍎查悞娲晬濮樿鲸鏆忓ù婊冮叄閵嗗秹鎯勯鏄忣洬闁告瑱缍€缁额參宕?濠㈣埖鐗為鏇熺▔閹绘帒缍撻柨娑樼墛鐎?savebox_board.h 闁汇劌瀚悾鐐▕婢舵稓绀? * - use_driver=false 闁哄啳顔愮槐鐧廇L_UART_Transmit() 濞村吋宀搁埀顑藉亾闁告牗鐗旂拹?fwrite 闁?stdout闁挎稑鐗撻埀顒佽壘閻栧墎鎸?UART0 闁硅矇鍐ㄧ厬闁告瑥搴滅槐? * - use_driver=true 闁哄啳顔愮槐浼存閳ь剛鎲版担椋庣☉闁革负鍔岄崺鍡樺緞閸曨偆鏆旈悷?闁告帗绻傞～鎰板礌?uart driver闁挎稑鑻懟鐔烘媼閸撗呮瀭 initialized=true
 */
UART_HandleTypeDef huart1 = {
    .port = SAVEBOX_UART_PORT,
    .tx_pin = SAVEBOX_UART_TX_GPIO,
    .rx_pin = SAVEBOX_UART_RX_GPIO,
    .baud_rate = SAVEBOX_UART_BAUD_RATE,
    .use_driver = true,
    .initialized = false,
};

/*
 * 闂佹澘绉堕悿鍡樼▔閳ь剚绋?GPIO 濞戞捇缂氱欢顓㈠礄閸濆嫯瀚欓悹浣稿⒔閻ゅ棝宕氬┑鍡╂綏闁汇垽娼ч柦鈺呭Υ? * @param pin   GPIO 缂傚倹鐗曡ぐ鍧楁晬閸︽〗P-IDF gpio_num_t闁? * @param level 闁告帗绻傞～鎰版偨闂堟盯鎸柨?/1闁? * @return esp_err_t
 */
static esp_err_t savebox_config_output(gpio_num_t pin, uint32_t level)
{
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }

    return gpio_set_level(pin, (int)level);
}

/*
 * 闂佹澘绉堕悿鍡樼▔閳ь剚绋?GPIO 濞戞捇缂氱欢顓㈠礂閵夘垳绀夐柛娆樺灦閳ь剙顦粭鍌炲箯婢跺牃鍋? * @param pin    GPIO 缂傚倹鐗曡ぐ? * @param pullup true=濞达綀鍎婚崗姗€宕橀崨鏉戝姤濞戞挸锕ユ刊娲晬鐎规溂lse=濞戞挸绉崇粭鍌炲箯? * @return esp_err_t
 */
static esp_err_t savebox_config_input(gpio_num_t pin, bool pullup)
{
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << pin,
        .mode = (gpio_mode_t)GPIO_MODE_DEF_INPUT,
        .pull_up_en = pullup ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    return gpio_config(&config);
}

/*
 * 妤犵偛鍟胯ぐ?闁哄娉曟鍥礆濠靛棭娼楅柛鏍ㄧ壄缁辩増绋夐埀顒€鈻庨埄鍐ｅ亾瑜旈崢銈囩磾椤旇姤鎷卞銈呮贡濞蹭即鎮介妸銉ョ厒闁汇劌瀚槐鈺呮嚇濮橆偆鐟㈠鑸电墳椤旀洟濡? * 婵炴垹鏁稿ú濠囧礃閸涱収鍟囬柨娑樼墛鐎垫粍绋夌€ｎ喗妗ㄥù鐙呯悼閻栨粍銇勯崫鍕闁挎稑顧€缁? * - 闁炬艾鍊跨粋蹇涘闯閵娿劎缈婚柛鎴炰航閳ь兛绶氶埞鍫熸綇妤ｅ啠鏀抽柛鏂诲妽閺岀喖宕ラ幋锝呭闁挎稑婀廔N1/AIN2闁挎稑顦埀顑挎祰缁夊瓨绔熼悧鍫濈毦 TRIG 閺夊牊鎸搁崵? * - 閻℃帒鎳庨敍鎰枖?ECHO 閺夊牊鎸搁崣鍡涘Υ娑撳粣T11 闁轰胶澧楀畵渚€鎳樺宕囩炕闁稿繈鍎荤槐娆撳礃閸涙潙鍔ュ☉鎾筹攻婵椽鏁? * - RC522闁挎稒鐡奡S(闁绘娲埀? 濞?RST 閺夊牊鎸搁崵? * - SPI 闁诡剝宕甸崵搴ㄥ礆濠靛棭娼楅柛?+ 婵烇綀顕ф慨?RC522 SPI device闁挎稑鐗婇弫鐐哄箛?spics_io_num=-1闁挎稑鐭侀妴鍐矆?CS 闁汇垼绮鹃拏瀣閹澘娈扮€规瓕椴哥敮鍫曞礆鐠佸湱绀? *
 * @return ESP_OK 闁瑰瓨鍔曟慨娑㈡晬濞戞ɑ鍎婇柛鎺撶懆缁绘垿宕?ESP-IDF 闂佹寧鐟ㄩ銈夋儘? */
esp_err_t savebox_platform_init(void)
{
    printf("[platform_trace] enter savebox_platform_init\n");
    /* 闂傚啫寮堕娑㈡煂瀹ュ拋妲婚柛鎺撶箓椤劙宕犻弽鐢电闂侇剙鐏濋崢銈夋煂瀹ュ拋妲?spi_bus_initialize 閻庝絻澹堥崵?ESP_ERR_INVALID_STATE */
    if (s_platform_initialized) {
        printf("[platform_trace] already initialized\n");
        return ESP_OK;
    }

    esp_err_t err = ESP_OK;

    /* 闁炬艾鍊跨粋蹇涘闯椤帞绐楀娑欘焾椤撳骞忔径鎰蒋/闁瑰嘲顦紞鍡涙偨鏉堚斁鈧牗绂掔捄鍝勬瀫閻庤鐔槐婵囨交濞嗘挸娅￠悹浣稿⒔閻ゅ棙绋?1闁挎稑鐗嗚ぐ鏌ユ嚄閹存帒鏁╅悶娑栧妼閸櫻囨⒒?闂傚牊鐟╅悡鍫曟晬鐏炶棄绲块柛鎰帠缁剟鎮芥担鍐唴闁?*/
#if SAVEBOX_ENABLE_BSP_BUZZER_GPIO_INIT
    printf("[platform_trace] before buzzer gpio init pin=%d\n", (int)Buzzer_Pin);
    err = savebox_config_output((gpio_num_t)Buzzer_Pin, 1);
    if (err != ESP_OK) {
        printf("[platform_trace] buzzer gpio init failed: %s\n", esp_err_to_name(err));
        return err;
    }
    printf("[platform_trace] after buzzer gpio init\n");
#else
    printf("[platform_trace] buzzer gpio init disabled\n");
#endif


    /* DHT11 闁轰胶澧楀畵浣虹棯閸栵紕绐楅悽顖涚摃椤棝妫侀埀顒傛啺娴ｉ鐟愰柟宄邦檧缁辨繈鏌嗛崹顔煎赋婵炴惌鍠氶埞?*/
#if SAVEBOX_ENABLE_BSP_DHT11_GPIO_INIT
    printf("[platform_trace] before dht11 gpio init pin=%d\n", (int)DHT11_DATA_Pin);
    err = savebox_config_input((gpio_num_t)DHT11_DATA_Pin, true);
    if (err != ESP_OK) {
        printf("[platform_trace] dht11 gpio init failed: %s\n", esp_err_to_name(err));
        return err;
    }
    printf("[platform_trace] after dht11 gpio init\n");
#else
    printf("[platform_trace] dht11 gpio init disabled\n");
#endif

#if SAVEBOX_ENABLE_BSP_SW180_GPIO_INIT
    printf("[platform_trace] before sw180 gpio init pin=%d\n", (int)SW180_DO_Pin);
    err = savebox_config_input((gpio_num_t)SW180_DO_Pin, true);
    if (err != ESP_OK) {
        printf("[platform_trace] sw180 gpio init failed: %s\n", esp_err_to_name(err));
        return err;
    }
    printf("[platform_trace] after sw180 gpio init\n");
#else
    printf("[platform_trace] sw180 gpio init disabled\n");
#endif

    /* RC522 闁绘娲埀?濠㈣泛绉崇紞鍛存嚇濮樺墽绐楅柛蹇撶墦閸樸倗绱旈鏄忕閺夊牊鎸搁崵顓㈡晬鐏炲€熷珯缂備焦鐟ょ粩瀛樼▔椤忓嫮鏆旈柛蹇嬪妿濞堟垶顪€濡鍚囬柣銏ゆ涧闁?*/
#if SAVEBOX_ENABLE_BSP_RC522_GPIO_INIT
    if (RC522_NSS_Pin != GPIO_NUM_NC) {
        printf("[platform_trace] before rc522 nss gpio init pin=%d\n", (int)RC522_NSS_Pin);
        err = savebox_config_output((gpio_num_t)RC522_NSS_Pin, 1);
        if (err != ESP_OK) {
            printf("[platform_trace] rc522 nss gpio init failed: %s\n", esp_err_to_name(err));
            return err;
        }
        printf("[platform_trace] after rc522 nss gpio init\n");
    } else {
        printf("[platform_trace] rc522 nss gpio init skipped (GPIO_NUM_NC)\n");
    }

    if (RC522_RST_Pin != GPIO_NUM_NC) {
        printf("[platform_trace] before rc522 rst gpio init pin=%d\n", (int)RC522_RST_Pin);
        err = savebox_config_output((gpio_num_t)RC522_RST_Pin, 1);
        if (err != ESP_OK) {
            printf("[platform_trace] rc522 rst gpio init failed: %s\n", esp_err_to_name(err));
            return err;
        }
        printf("[platform_trace] after rc522 rst gpio init\n");
    } else {
        printf("[platform_trace] rc522 rst gpio init skipped (GPIO_NUM_NC)\n");
    }
#else
    printf("[platform_trace] rc522 nss/rst gpio init disabled\n");
#endif

    if (SAVEBOX_ENABLE_BSP_RC522_SPI_BUS) {

    /* SPI 闁诡剝宕甸崵搴☆嚕閺団€冲闁哄嫮濮撮惃?*/
    const spi_bus_config_t bus_config = {
        .mosi_io_num = (gpio_num_t)RC522_MOSI_Pin,
        .miso_io_num = (gpio_num_t)RC522_MISO_Pin,
        .sclk_io_num = (gpio_num_t)RC522_SCK_Pin,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        /* RC522 闁告娲橀鍏煎閻樿櫣缈诲☉鎾亾闁煎壊鍓欑欢銏焊韫囥儳绀夐弶鈺傜懇閸ｇ兘姊介幇顒€鐓?max_transfer_sz=16 閻庢稒顨夋俊?*/
        .max_transfer_sz = 16,
    };

    printf("[platform_trace] before spi_bus_initialize host=%d mosi=%d miso=%d sclk=%d dma=%d\n",
           (int)hspi1.host,
           (int)RC522_MOSI_Pin,
           (int)RC522_MISO_Pin,
           (int)RC522_SCK_Pin,
           (int)SAVEBOX_RC522_SPI_DMA_CHAN);
    err = spi_bus_initialize(hspi1.host, &bus_config, SAVEBOX_RC522_SPI_DMA_CHAN);
    if (err != ESP_OK) {
        printf("[platform_trace] spi_bus_initialize failed: %s\n", esp_err_to_name(err));
        return err;
    }
    printf("[platform_trace] after spi_bus_initialize\n");

    /*
     * SPI 閻犱焦鍎抽ˇ顒勬煀瀹ュ洨鏋傞柨?     * - mode=0闁挎稒鐡擯I Mode0闁挎稑婀慞OL=0, CPHA=0闁?     * - spics_io_num=-1闁挎稒鐭粭澶屾媼?SPI driver 闁煎浜滄慨鈺呭箳瑜嶉崺?CS
     *   闁挎稑鐗嗛悥鍓佹喆娴ｇ鏂ч柛銉у缁辩増銇勯崷顓熺獥闁煎浜滅换渚€鎮介妸锔界彯闂?GPIO 闁硅矇鍐ㄧ厬 NSS闁挎稑鏈崹銊╂閳ь剛鎲版担瑙勭函闁诲繐鐏氬鍧楁儍閸曨厼顣婚梺顐㈩槹濡炲倹鎯旇箛銉х
     * - queue_size=1闁挎稒淇洪悿鍡欐嫚閵忊剝鐓欑€殿喖绻嬬槐鑸垫綇閹垮嫮绀剆pi_device_polling_transmit闁挎稑顦甸埀顒佽壘閻栬埖寰勯悢鐑樻殢
     */
    const spi_device_interface_config_t device_config = {
        .clock_speed_hz = SAVEBOX_RC522_SPI_CLOCK_HZ,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1,
    };

    printf("[platform_trace] before spi_bus_add_device clock=%d\n", (int)SAVEBOX_RC522_SPI_CLOCK_HZ);
    err = spi_bus_add_device(hspi1.host, &device_config, &hspi1.device);
    if (err != ESP_OK) {
        printf("[platform_trace] spi_bus_add_device failed: %s\n", esp_err_to_name(err));
        return err;
    }
    printf("[platform_trace] after spi_bus_add_device\n");

    hspi1.initialized = true;
    } else {
        printf("[platform_trace] rc522 spi bus disabled\n");
    }
    s_platform_initialized = true;
    printf("[platform_trace] leave savebox_platform_init\n");
    return ESP_OK;
}

/*
 * 濞寸姰鍎扮粭?HAL_XXX 缂侇垵顕ч崹顏堝礄閼恒儲娈堕柨? * 闁活潿鍔嬬花顒勫礂閻撳寒鍟?濠㈣泛绉堕弫銈囩尵鐠佽櫕濡?STM32 HAL 闁汇劌瀚惃鐔兼偨閵娿倗鐦庨柟顖ｅ灛閳? * 閺夆晜鐟╅崳鐑芥儍?GPIO_TypeDef* 闁告瑥鍊归弳鐔煎捶?ESP32 濞戞挸锕ラ惀鍛村嫉婢跺﹦鏉介梻鍕噺閸撶増绋婃径娑氱闁搞儳濮甸婵堢磼閻斿墎顏?(void) 闁瑰搫顦埀? */

/*
 * GPIO 闁告帗绻傞～鎰板礌閺嶇數绀凥AL 闁稿繒鍘ч鎰沪閸岋妇绀嗛柨娑欒壘閻?GPIO_InitTypeDef 缂傚牊妲掗惁褔骞?ESP-IDF gpio_config_t闁? * @note 閺夆晜鐟╅崳鐑藉矗椤忓嫷妲遍柣鐐叉４缁辩増娼忛幘鍐插汲/閺夊牊鎸搁崵顓㈠Υ娴ｉ鐟愰柟宄邦檧缁辨繈宕楃捄铏规殜婵☆垪鈧磭纭€濠碘€冲€烽懙鎴﹀棘?濞戞挸顑嗘刊铏圭驳婢跺寮撻悷鏇炴濞插﹪濡? */
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, const GPIO_InitTypeDef *GPIO_Init)
{
    (void)GPIOx;

    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << GPIO_Init->Pin,
        .mode = (GPIO_Init->Mode == GPIO_MODE_OUTPUT_PP) ? GPIO_MODE_OUTPUT : (gpio_mode_t)GPIO_MODE_DEF_INPUT,
        .pull_up_en = (GPIO_Init->Pull == GPIO_PULLUP) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&config);
}

/* 閻犲洩顕цぐ?GPIO 闁汇垽娼ч柦鈺呮晬閸︽L 闁稿繒鍘ч鎰沪閸岋妇绀嗛柕?*/
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    (void)GPIOx;
    return gpio_get_level((gpio_num_t)GPIO_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

/* 闁?GPIO 闁汇垽娼ч柦鈺呮晬閸︽L 闁稿繒鍘ч鎰沪閸岋妇绀嗛柕?*/
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    (void)GPIOx;
    gpio_set_level((gpio_num_t)GPIO_Pin, (PinState == GPIO_PIN_SET) ? 1 : 0);
}

/* 缂傚牊妲掑ù?GPIO 闁汇垽娼ч柦鈺呮晬閸︽L 闁稿繒鍘ч鎰沪閸岋妇绀嗛柕?*/
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    (void)GPIOx;
    const int current = gpio_get_level((gpio_num_t)GPIO_Pin);
    gpio_set_level((gpio_num_t)GPIO_Pin, !current);
}

/*
 * 婵綆鍋嗛～妤冪棯瑜嶅▎銏ゅ籍鐠佸湱绀凥AL 闁稿繒鍘ч鎰沪閸岋妇绀嗛柕? * @note 濞达綀娉曢弫?esp_rom_delay_us 閻庡湱鍋熼獮鍥疀濞嗘垹鎼肩€垫澘鎷戠槐? * - 濞村吋顭囬崑锝夋晬濮橆偆鐟濆〒姘箚缁?FreeRTOS
 * - 缂傚倽娅ｉ崑锝夋晬濮橆偆绐楅柛妤冨Х閺?CPU闁挎稑濂旂粭澶嬪濮樻剚鍞ㄩ柛鎴︾細閻ㄧ喐鎯? * 濠碘€冲€垮〒?RTOS 闁告瑥顑呴妶浠嬫儍閸曨偅顐介柡鍐啇缁辨繂顕欐ウ娆惧敶闁革负鍔嬬粭鐔煎礉閿熺姴娅￠柣?vTaskDelay(pdMS_TO_TICKS(ms))闁? */
void HAL_Delay(uint32_t ms)
{
    while (ms-- > 0U) {
        esp_rom_delay_us(1000U);
    }
}

/* 闁兼儳鍢茶ぐ鍥╁寲閼姐倗鍩?tick闁挎稑鐗婇鐘电矓閹虹偟绀嗛柕?*/
uint32_t HAL_GetTick(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000LL);
}
esp_err_t savebox_uart_driver_init(UART_HandleTypeDef *huart, int rx_buffer_size, int tx_buffer_size, int queue_size)
{
    esp_err_t err = ESP_OK;
    uart_config_t uart_config = {0};

    if ((huart == NULL) || (rx_buffer_size <= 0)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (huart->initialized && huart->use_driver) {
        return ESP_OK;
    }

    if ((huart->port == UART_NUM_0) &&
        (huart->tx_pin == UART_PIN_NO_CHANGE) &&
        (huart->rx_pin == UART_PIN_NO_CHANGE)) {
        printf("[uart_trace] skip uart driver install for console UART0\n");
        huart->use_driver = false;
        huart->initialized = false;
        return ESP_OK;
    }

    uart_config.baud_rate = (int)huart->baud_rate;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 0;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    uart_config.flags.allow_pd = 0;

    err = uart_driver_install(huart->port, rx_buffer_size, tx_buffer_size, queue_size, NULL, 0);
    if ((err != ESP_OK) && (err != ESP_ERR_INVALID_STATE)) {
        return err;
    }

    err = uart_param_config(huart->port, &uart_config);
    if (err != ESP_OK) {
        return err;
    }

    err = uart_set_pin(huart->port, huart->tx_pin, huart->rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        return err;
    }

    huart->use_driver = true;
    huart->initialized = true;
    return ESP_OK;
}

int savebox_uart_read(UART_HandleTypeDef *huart, uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    if ((huart == NULL) || (data == NULL) || (len == 0U)) {
        return -1;
    }

    if (!huart->use_driver || !huart->initialized) {
        return -1;
    }

    return uart_read_bytes(huart->port, data, len, pdMS_TO_TICKS(timeout_ms));
}

/*
 * UART 闁告瑦鍨块埀顑跨筏缁辨│AL 闁稿繒鍘ч鎰沪閸岋妇绀嗛柕? * - use_driver=true闁挎稒鐭繛鍥偨?ESP-IDF UART driver闁挎稑婢哸rt_write_bytes闁? * - 闁告熬绠戦崹顖炴晬濮樺灈鍋撻埀顒勫礌閺嶏箒绀嬮柛?stdout闁挎稑鐗撻埀顒佽壘閻栧爼宕ｉ姘含 monitor 濞戞搩鍘惧﹢鍛村礆鐢喚绀? */
esp_err_t HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *data, uint16_t len, uint32_t timeout)
{
    (void)timeout;

    if ((huart != NULL) && huart->use_driver && huart->initialized) {
        return (uart_write_bytes(huart->port, data, len) < 0) ? ESP_FAIL : ESP_OK;
    }

    return (fwrite(data, 1, len, stdout) == len) ? ESP_OK : ESP_FAIL;
}

/*
 * SPI 闁稿繈鍔屽璇差啅閵夛附鏆柛娆愬煀缁辨│AL 闁稿繒鍘ч鎰沪閸岋妇绀嗛柕? * @param hspi   SPI 閻犱焦鍎抽ˇ顒勫矗閵夛妇鍔撮柨娑樼墕缁烩偓濡炪倛顕ч崙?initialized 濞?device 闂傚牏鍋熼埞鏍晬? * @param tx_data 闁告瑦鍨块埀顑胯兌缁憋箓宕橀幓鎺戦殬
 * @param rx_data 闁规亽鍎查弫鍦磽閹惧啿鏆遍柛? * @param len    閻庢稒顨夋俊顓㈠极? * @return esp_err_t
 */
esp_err_t HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, const uint8_t *tx_data, uint8_t *rx_data, uint16_t len, uint32_t timeout)
{
    (void)timeout;

    if ((hspi == NULL) || !hspi->initialized || (hspi->device == NULL)) {
        return ESP_ERR_INVALID_STATE;
    }

    spi_transaction_t transaction;
    memset(&transaction, 0, sizeof(transaction));
    transaction.length = (size_t)len * 8U; /* SPI driver 闁?length 闁告娲戠紞鍛村及?bit */
    transaction.tx_buffer = tx_data;
    transaction.rx_buffer = rx_data;

    /* polling 闁哄倻鎳撶槐锟犳晬濮樺磭娈堕柣顫妽濠€锟犳⒒缂堢姷绐楅梻鍐嚙椤綁鎯勯弶鎴濈厒濞磋偐濮剧欢顓犫偓鐟版湰閸?*/
    return spi_device_polling_transmit(hspi->device, &transaction);
}

/*
 * 闁告凹鍨版慨?闁告帗绻傞～鎰板礌閺嶏妇顏卞☉?PWM 闂侇偅宀告禍楣冩晬閸︾嚊DC闁挎稑顦埀? * 閺夆晜鐟ら柌婊堝礄閼恒儲娈跺ù鍏肩啲缁? * 1) 闂佹澘绉堕悿?LEDC timer闁挎稑鐗撻。鍫曟偝閸ャ儮鍋撴担绋跨€婚弶鍫涘妿瀹歌偐绮垫径娑氱
 * 2) 闂佹澘绉堕悿?LEDC channel闁挎稑鐗忕划锔锯偓?GPIO闁靛棔绶氶埀顒€顦扮€?timer闁靛棔绀侀崹鍨叏?duty=0闁? *
 * @param htim TIM 闁告瑣鍎查悞娲晬閸繄绠戝銈咁煼閸樸倗绱?gpio_num 闁?frequency_hz闁? */
esp_err_t savebox_pwm_start(TIM_HandleTypeDef *htim)
{
    if ((htim == NULL) || (htim->gpio_num == GPIO_NUM_NC) || (htim->frequency_hz == 0U)) {
        return ESP_ERR_INVALID_ARG;
    }

    /* 鐎瑰憡褰冮幆搴ㄥ礉閵娿儱鐏熼柣鈺佺摠鐢瓨娼婚弬鎸庣闁挎稑鐭傛导鈺呭礂瀹ュ娅㈠璺虹Ч閸樸倗绱?LEDC */
    if (htim->pwm_started) {
        return ESP_OK;
    }

    const ledc_timer_config_t timer_config = {
        .speed_mode = htim->speed_mode,
        .duty_resolution = SAVEBOX_PWM_DUTY_RESOLUTION,
        .timer_num = htim->timer_num,
        .freq_hz = htim->frequency_hz,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false,
    };

    esp_err_t err = ledc_timer_config(&timer_config);
    if (err != ESP_OK) {
        return err;
    }

    const ledc_channel_config_t channel_config = {
        .gpio_num = htim->gpio_num,
        .speed_mode = htim->speed_mode,
        .channel = htim->channel,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = htim->timer_num,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
        .flags = {
            .output_invert = 0,
        },
    };

    err = ledc_channel_config(&channel_config);
    if (err != ESP_OK) {
        return err;
    }

    htim->pwm_started = true;
    htim->period = SAVEBOX_PWM_PERIOD;
    return ESP_OK;
}

/* 闁兼儳鍢茶ぐ?PWM 闁?period闁挎稑鐗愰鎼佸极妫颁胶鐟愰梻鍕姧缁辨岸濡?*/
uint32_t savebox_pwm_get_period(const TIM_HandleTypeDef *htim)
{
    return (htim == NULL) ? 0U : htim->period;
}

/* 闁兼儳鍢茶ぐ?PWM 濡増鍨瑰濂告晬閸︽闁挎稑顦埀?*/
uint32_t savebox_pwm_get_frequency(const TIM_HandleTypeDef *htim)
{
    return (htim == NULL) ? 0U : htim->frequency_hz;
}

/*
 * 閻犱礁澧介悿?PWM 濡増鍨瑰濂稿Υ? * @note 闁兼眹鍎甸埀顒佸哺娴滃墽浜稿顓熷紦闁告凹鍨版慨鈺呮晬鐏炶偐绐楅柛蹇撶墣閻ㄧ喖鎮?savebox_pwm_start() 閻庣懓鏈崹?LEDC 闁告帗绻傞～鎰板礌閺嶃儮鍋? */
esp_err_t savebox_pwm_set_frequency(TIM_HandleTypeDef *htim, uint32_t freq_hz)
{
    if ((htim == NULL) || (freq_hz == 0U)) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = savebox_pwm_start(htim);
    if (err != ESP_OK) {
        return err;
    }

    /* 濞ｅ浂鍠楅弫?LEDC timer 闁汇劌瀚伴。鍫曟偝?*/
    err = ledc_set_freq(htim->speed_mode, htim->timer_num, freq_hz);
    if (err == ESP_OK) {
        htim->frequency_hz = freq_hz;
    }

    return err;
}

/*
 * 閻犱礁澧介悿?PWM 闁告濮烽埞鏍掗弬鐑╁亾? * @param duty 鐟滅増甯婄粩鎾礌閺嵮冪獥缂佸瞼鍎ら惁顕€鏁?.0~1.0闁挎稑顧€缁辨繄鎼鹃崨顓炴瘔闁肩厧鍟ú鎸庡濮樻剚娼堕梺钘夊帠缂嶅懘濡? * @note 闁告劕鎳橀崕瀛樺濮橆厼惟 duty 闁哄嫮濮撮惃鐘诲礆?raw_duty = period * duty闁? */
esp_err_t savebox_pwm_set_duty(TIM_HandleTypeDef *htim, float duty)
{
    if (htim == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = savebox_pwm_start(htim);
    if (err != ESP_OK) {
        return err;
    }

    /* 闁告濮烽埞鏍掗弮鍫熷碍濞达絽绋勭槐婵嬫焼閸喖甯抽柛鎴ｆ楠炲洨鎷归悢灏佸亾閸忕厧鐏?>100% */
    if (duty < 0.0f) {
        duty = 0.0f;
    }
    if (duty > 1.0f) {
        duty = 1.0f;
    }

    const uint32_t raw_duty = (uint32_t)((float)htim->period * duty);
    err = ledc_set_duty(htim->speed_mode, htim->channel, raw_duty);
    if (err != ESP_OK) {
        return err;
    }

    return ledc_update_duty(htim->speed_mode, htim->channel);
}
