/*
 * front_light.c
 *
 *  Created on: 29 mai 2024
 *      Author: Guillaume Varlet
 */

#include <cstddef>
#include <stdint.h>
#include <string>
#include <esp_log.h>
#include <driver/rmt_tx.h>
#include "front_light.h"
#include "esp_log_level.h"
#include "freertos/FreeRTOS.h"

/**********************************************************************************************************************
 *                                              Constants declarations                                                *
 **********************************************************************************************************************/
const char FrontLight::m_logTag[11] = "FrontLight";

const rmt_symbol_word_t FrontLight::m_ws2812Zero = {
    {
        .duration0 = 4,
        .level0 = 1,
        .duration1 = 9,
        .level1 = 0
    }
};

const rmt_symbol_word_t FrontLight::m_ws2812One = {
    {
        .duration0 = 9, // T1H=0.9us
        .level0 = 1,
        .duration1 = 4, // T1L=0.3us
        .level1 = 0,
    }
};

const rmt_symbol_word_t FrontLight::m_ws2812Reset = {
    {
        .duration0 = 25, 
        .level0 = 0,
        .duration1 = 25, 
        .level1 = 0,
    }
};

const rmt_tx_channel_config_t FrontLight::m_rmtChannelCfg = {
    .gpio_num = (gpio_num_t)GPIO_FRONT_LIGHT,
    .clk_src = RMT_CLK_SRC_DEFAULT,
    .resolution_hz = RMT_LED_STRIP_RESOLUTION_HZ,
    .mem_block_symbols = 64,
    .trans_queue_depth = 4,
    .intr_priority = 0,
    .flags = {
        .invert_out = 0,
        .with_dma = 0,
        .allow_pd = 0,
        .init_level = 0,
    }
};

size_t FrontLight::encoder_callback(
            const void *data, 
            size_t data_size,
            size_t symbols_written, 
            size_t symbols_free,
            rmt_symbol_word_t *symbols, 
            bool *done, 
            void *arg )
{
    // We need a minimum of 8 symbol spaces to encode a byte. We only
    // need one to encode a reset, but it's simpler to simply demand that
    // there are 8 symbol spaces free to write anything.
    if (symbols_free < 8) {
        return 0;
    }

    // We can calculate where in the data we are from the symbol pos.
    // Alternatively, we could use some counter referenced by the arg
    // parameter to keep track of this.
    size_t data_pos = symbols_written / 8;
    uint8_t *data_bytes = (uint8_t*)data;
    if (data_pos < data_size) {
        // Encode a byte
        size_t symbol_pos = 0;
        for (int bitmask = 0x80; bitmask != 0; bitmask >>= 1) {
            if (data_bytes[data_pos]&bitmask) {
                symbols[symbol_pos++] = m_ws2812One;
            } else {
                symbols[symbol_pos++] = m_ws2812Zero;
            }
        }
        // We're done; we should have written 8 symbols.
        return symbol_pos;
    } else {
        //All bytes already are encoded.
        //Encode the reset, and we're done.
        symbols[0] = m_ws2812Reset;
        *done = 1; //Indicate end of the transaction.
        return 1; //we only wrote one symbol
    }
}

size_t FrontLight::encoder_callback_thunk(
                                    const void *pData, 
                                    size_t data_size,
                                    size_t symbols_written, 
                                    size_t symbols_free,
                                    rmt_symbol_word_t *symbols, 
                                    bool *pDone, 
                                    void *pArg)
{
    FrontLight* frontLightInstance = static_cast<FrontLight*>(pArg);
    return frontLightInstance->encoder_callback(pData, data_size, symbols_written, symbols_free, symbols, pDone, pArg);
}

FrontLight::FrontLight(void)
{
    const rmt_simple_encoder_config_t simpleEncoderCfg = {
        .callback = encoder_callback_thunk,
        .arg = this,
        .min_chunk_size = 64
    };
    
    m_simpleEncoderHdl = NULL;
    m_ledChannelHdl = NULL;

    ESP_LOGI(m_logTag, "Create RMT TX channel");
    ESP_ERROR_CHECK(rmt_new_tx_channel(&m_rmtChannelCfg, &m_ledChannelHdl));

    ESP_LOGI(m_logTag, "Create simple callback-based encoder");
    ESP_ERROR_CHECK(rmt_new_simple_encoder(&simpleEncoderCfg, &m_simpleEncoderHdl));
    ESP_LOGI(m_logTag, "Enable RMT TX channel");
    ESP_ERROR_CHECK(rmt_enable(m_ledChannelHdl));

    m_rmtTxCfg.loop_count = 0; // no transfer loop
}

FrontLight::~FrontLight(void)
{
    
}

void FrontLight::update(
                    const uint8_t intensity, 
                    const uint8_t led_red, 
                    const uint8_t led_green, 
                    const uint8_t led_blue ){
    
    if( intensity > 10)
    {
        ESP_LOGW(m_logTag, "Update: Value of parameter above the limit, abort update");
        ESP_LOGW(m_logTag, "Update: Value of parameter intensity = %u", intensity);
    }
    else if( intensity == 0 ){

    }
    else {
        m_ledColour.red = led_red / (10 - intensity);
        m_ledColour.green = led_green / (10 - intensity);
        m_ledColour.blue = led_blue / (10 - intensity);

        ESP_ERROR_CHECK(rmt_transmit(
                                m_ledChannelHdl, 
                                m_simpleEncoderHdl,
                                &m_ledColour, 
                                sizeof(m_ledColour),
                                &m_rmtTxCfg));
        ESP_ERROR_CHECK(rmt_tx_wait_all_done(m_ledChannelHdl, portMAX_DELAY));   
    }
}

