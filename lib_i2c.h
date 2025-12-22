#ifndef LIB_I2C
#define LIB_I2C
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "config.h"
#include "i2c.h"

#define I2C_QUEUE_BUFFER_SIZE 512
//buffer to copy and paste to reset queue start to 0
typedef struct
{
    I2C_TypeDef * device;
    uint8_t queue_pos;
    uint8_t queue_end_pos;
    uint8_t b_queue[I2C_QUEUE_BUFFER_SIZE];
    uint8_t queue_pos_data[I2C_QUEUE_BUFFER_SIZE]; // length of each index
    uint8_t queue_pos_addr[I2C_QUEUE_BUFFER_SIZE]; //i2c addr of device to send to for each index
    uint8_t queue_msg_size; //sets max message size per element of queue

    //for isr stuff
    uint8_t tx_send_idx;
    bool tx_ongoing;
    uint8_t rx_buf[255];
    uint8_t rx_buf_queue_idx;
    uint8_t rx_len;
}i2c_queue_t;

//initializes the queue
void i2c_queue_init(i2c_queue_t * q, uint8_t size);

//adds message to end of queue
void i2c_queue_append(i2c_queue_t * q, uint8_t addr, uint8_t * data, uint8_t len);
//dont forget to add looping

void i2c_queue_clear(i2c_queue_t *q);

void i2c_timing(void);

void i2c_advance_queue(i2c_queue_t * q);
void i2c_force_send_msg(i2c_queue_t * q, uint8_t addr, uint8_t * data, uint8_t len);
bool i2c_is_queue_empty(i2c_queue_t *q);
void i2c_svc(i2c_queue_t* q); // put this in main loop
void i2c_start_tx_msg(i2c_queue_t * q);
void i2c_tx_isr(i2c_queue_t* q);
void i2c_rx_isr(i2c_queue_t* q);
void i2c_isr(i2c_queue_t *q);

#endif