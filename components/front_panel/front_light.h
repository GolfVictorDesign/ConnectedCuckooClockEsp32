/*
 * front_light.h
 *
 *  Created on: 29 mai 2024
 *      Author: guillaume
 */

#ifndef MAIN_FRONT_PANEL_FRONT_LIGHT_H_
#define MAIN_FRONT_PANEL_FRONT_LIGHT_H_

#define GPIO_FRONT_LIGHT 48

#ifdef __cplusplus
extern "C" {
#endif

void update_front_light(uint8_t led_red, uint8_t led_green, uint8_t led_blue);
void configure_front_light(void);


#ifdef __cplusplus
}
#endif

#endif /* MAIN_FRONT_PANEL_FRONT_LIGHT_H_ */
