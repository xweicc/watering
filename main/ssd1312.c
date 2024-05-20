#include "main.h"

#define OLED_ADDR 0x3C

void ssd1312_write_cmd(uint8_t cmd)
{
    uint8_t write_buf[2] = {0x00, cmd};

    esp_err_t ret = i2c_master_write_to_device(0, OLED_ADDR, write_buf, sizeof(write_buf), 10);
    if (ret != ESP_OK) {
        Printf("write failed %d\n",ret);
    }
}

void ssd1312_write_data(uint8_t *data, int len)
{
    uint8_t *write_buf=malloc(len+1);

    write_buf[0]=0x40;
    memcpy(write_buf+1,data,len);
    esp_err_t ret = i2c_master_write_to_device(0, OLED_ADDR, write_buf, len+1, 10);
    if (ret != ESP_OK) {
        Printf("write failed %d\n",ret);
    }
    free(write_buf);
}

//OLED配置
void oled_config(void)
{
    uint8_t config[]={
        0xAE,	//--turn off oled panel
        0x00,   /*set lower column address*/       
        0x10,   /*set higher column address*/
        0xB0,   /*set page address*/
        0x81,   /*contract control*/
        0x5f,   /*128*/
        0x20,   /* Set Memory addressing mode (0x20/0x21) */
        0x02,   /* 0x02 */
        0xA1,   /*set segment remap  0XA1 */
        0xC0,   /*Com scan direction   0Xc0  */
        0xA4,   /*Disable Entire Display On (0xA4/0xA5)*/ 
        0xA6,   /*normal / reverse*/
        0xA8,   /*multiplex ratio*/
        0x3F,   /*duty = 1/64*/
        0xD3,   /*set display offset*/
        0x00,   /*   0x20   */
        0xD5,   /*set osc division*/
        0x80,   
        0xD9,   /*set pre-charge period*/
        0x22,
        0xDA,   /* Set SEG Pins Hardware Configuration */
        0x10,
        0xdb,   /*set vcomh*/
        0x30,
        0x8d,   /*set charge pump enable*/
        0x72,   /* 0x12:7.5V; 0x52:8V;  0x72:9V;  0x92:10V */
        0xAF,
    };
    for(int i=0;i<sizeof(config);i++){
        ssd1312_write_cmd(config[i]);
    }
}

void i2c_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 18,
        .scl_io_num = 8,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };

    i2c_param_config(0, &conf);
    ESP_ERROR_CHECK(i2c_driver_install(0, conf.mode, 0, 0, 0));
}

int ssd1312_init(void)
{
    i2c_init();
    vTaskDelay(10);
    oled_config();
    return 0;
}

void ssd1312_flush(void)
{
	for(uint8_t i=0;i<8;i++){
		ssd1312_write_cmd(0xb0+i);         //设置页地址(0~7)
		ssd1312_write_cmd(0x00);           //设置显示位置—列低地址
		ssd1312_write_cmd(0x10);           //设置显示位置—列高地址
		for(int j=0;j<128;j+=8){
            ssd1312_write_data(&GRAM[i][j],8);   //传输显示数据
        }
	}
}

//屏幕旋转180度
void ssd1312_rotate(int rotate)
{
    if(rotate==1){
        ssd1312_write_cmd(0xA0);
        ssd1312_write_cmd(0xC8);
	}else{
        ssd1312_write_cmd(0xA1);
        ssd1312_write_cmd(0xC0);
    }
}

void ssd1312_contrast(int contrast)
{
    ssd1312_write_cmd(0x81);
    if(contrast==contrastLow){
        ssd1312_write_cmd(0x01);
    }else if(contrast==contrastMid){
        ssd1312_write_cmd(0x10);
    }else if(contrast==contrastHigh){
        ssd1312_write_cmd(0x7F);
    }
}

oled_driver_t *ssd1312_new(void)
{
    oled_driver_t *oled=malloc(sizeof(*oled));
    if(oled){
        oled->init=ssd1312_init;
        oled->flush=ssd1312_flush;
        oled->rotate=ssd1312_rotate;
        oled->contrast=ssd1312_contrast;
    }

    return oled;
}
