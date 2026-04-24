/*
 * front_panel.cpp
 *
 *  Created on: 16 avr. 2026
 *      Author: guillaume
 */
# include <esp_err.h>
#include "front_panel.h"

FrontPanel::FrontPanel() {
    // TODO Auto-generated constructor stub

}

FrontPanel::~FrontPanel() {
    // TODO Auto-generated destructor stub
}

esp_err_t FrontPanel::update(void)
{
    static uint8_t led_blue = 0;
    m_frontLight.update(30, 5, 0);
    led_blue += 5;
    return ESP_OK;    
}

