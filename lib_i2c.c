#include "lib_i2c.h"
#include "appl_main.h"

#define I2C_QUIET_TIME 2

bool i2c_ready = 1;
uint32_t i2c_timer = 0;
void i2c_queue_init(i2c_queue_t * q, uint8_t size)
{
    memset(q->b_queue,0,255);
    memset(q->queue_pos_data,0,255);
    q->queue_end_pos = 0;
    q->queue_pos = 0;
    q->queue_msg_size = size;
    q->tx_ongoing = 0;
    //enable interrupts

}

void i2c_queue_append(i2c_queue_t * q, uint8_t addr, uint8_t * data, uint8_t len)
{
    memcpy(&q->b_queue[q->queue_end_pos * q->queue_msg_size],data,len);
    q->queue_pos_addr[q->queue_end_pos] = addr;
    q->queue_pos_data[q->queue_end_pos] = len;

    q->queue_end_pos++;
}

void i2c_advance_queue(i2c_queue_t * q)
{
    q->queue_pos++;
    q->tx_send_idx=0;

    if ((q->queue_pos >= q->queue_end_pos) || (q->queue_pos >= (I2C_QUEUE_BUFFER_SIZE/q->queue_msg_size)) )
    {
        i2c_queue_clear(q);
    }
}

//clears queue and forces the current message
void i2c_force_send_msg(i2c_queue_t * q, uint8_t addr, uint8_t * data, uint8_t len)
{
    i2c_queue_clear(q);
    i2c_queue_append(q,addr,data,len);
    i2c_start_tx_msg(q);
}

void i2c_queue_send(i2c_queue_t * q)
{
	// HAL_I2C_Master_Transmit_IT (q->device, q->queue_pos_addr[q->queue_pos],q->b_queue[q->queue_msg_size * q->queue_pos], q->queue_pos_data[q->queue_end_pos]);
}

void i2c_queue_clear(i2c_queue_t *q)
{
    memset(q->b_queue,0,255);
    memset(q->queue_pos_data,0,255);
    q->queue_end_pos = 0;
    q->queue_pos = 0;
    q->tx_send_idx = 0;
}

bool i2c_is_queue_empty(i2c_queue_t *q)
{
    return ((q->queue_end_pos - q->queue_pos) == 0);
}

void i2c_svc(i2c_queue_t* q) // put this in main loop
{
    if(!q->tx_ongoing && !i2c_is_queue_empty(q) && i2c_ready)
    {
        i2c_start_tx_msg(q);
    }
}

void i2c_start_tx_msg(i2c_queue_t * q)
{
    q->tx_ongoing = 1;

    //temp var to store the values needed for CR2
    uint32_t tmp = (((q->queue_pos_addr[q->queue_pos] << I2C_CR2_SADD_Pos)) & I2C_CR2_SADD) | (((q->queue_pos_data[q->queue_pos] << I2C_CR2_NBYTES_Pos)) & I2C_CR2_NBYTES) | I2C_CR2_AUTOEND | I2C_CR2_START;
    
    q->device->CR2 |= tmp;
    q->device->CR1 |= I2C_CR1_TXIE;
    q->device->CR1 |= I2C_CR1_STOPIE;
}

void i2c_tx_isr(i2c_queue_t* q)
{
    if(q->device->ISR & (I2C_ISR_STOPF))
    {
        q->tx_ongoing = 0;
        q->device->CR1 &= ~(I2C_ISR_STOPF);
        q->device->ICR |= I2C_ICR_STOPCF;
        i2c_advance_queue(q);
        i2c_ready = 0;
    }
    else if(q->device->ISR & (I2C_ISR_TXE))
    {
        q->device->TXDR = q->b_queue[q->queue_msg_size * q->queue_pos + q->tx_send_idx];
        q->tx_send_idx++;
    }
}

void i2c_timing(void)
{
    if(!i2c_ready)
    {
        i2c_timer++;
        if(i2c_timer >= I2C_QUIET_TIME)
        {
            i2c_ready = 1;
            i2c_timer = 0;
        }
    }
}

void i2c_rx_isr(i2c_queue_t* q)
{
    q->rx_buf[q->rx_buf_queue_idx] = q->device->RXDR;
    q->rx_buf_queue_idx++;
    //this is incomplete but ok
}

void i2c_isr(i2c_queue_t *q)
{
    if(q->device->ISR & (I2C_ISR_TXE | I2C_ISR_STOPF))
    {
        i2c_tx_isr(q);

    }
    if(q->device->ISR & I2C_ISR_RXNE)
    {
        i2c_rx_isr(q);
    }
    //there should be an error interrupt here but too lazy
}
