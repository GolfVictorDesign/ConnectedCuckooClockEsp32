/*
 * front_light.h
 *
 *  Created on: 29 mai 2024
 *      Author: guillaume
 */

#ifndef MAIN_FRONT_PANEL_FRONT_LIGHT_H_
#define MAIN_FRONT_PANEL_FRONT_LIGHT_H_

#include "driver/rmt_types.h"
#include <cstdint>
#define GPIO_FRONT_LIGHT 48
#define RMT_LED_STRIP_RESOLUTION_HZ 10000000

#include <driver/rmt_tx.h>

class FrontLight
{
    private:
    
        struct led_colour
        {
            uint8_t green;
            uint8_t red;
            uint8_t blue;
        };

        const static char                       m_logTag[11];
        
        const static rmt_symbol_word_t          m_ws2812Zero;
        const static rmt_symbol_word_t          m_ws2812One;
        const static rmt_symbol_word_t          m_ws2812Reset;
        
        const static rmt_tx_channel_config_t    m_rmtChannelCfg;
        
        rmt_transmit_config_t                   m_rmtTxCfg;
        rmt_channel_handle_t                    m_ledChannelHdl;
        rmt_encoder_handle_t                    m_simpleEncoderHdl;
        
        struct led_colour                       m_ledColour;
        
        size_t encoder_callback(
                            const void *data, 
                            size_t data_size,
                            size_t symbols_written, 
                            size_t symbols_free,
                            rmt_symbol_word_t *symbols, 
                            bool *done, 
                            void *arg );

        static size_t encoder_callback_thunk(
                                        const void *data, 
                                        size_t data_size,
                                        size_t symbols_written, 
                                        size_t symbols_free,
                                        rmt_symbol_word_t *symbols, 
                                        bool *done, 
                                        void *arg );
        
    public:
        FrontLight(void);
        ~FrontLight(void);

        void update(
                const uint8_t led_red, 
                const uint8_t led_green, 
                const uint8_t led_blue );
};    

#endif /* MAIN_FRONT_PANEL_FRONT_LIGHT_H_ */
