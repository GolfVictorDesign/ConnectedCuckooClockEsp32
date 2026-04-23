/*
 * front_panel.h
 *
 *  Created on: 16 avr. 2026
 *      Author: guillaume
 */

#ifndef COMPONENTS_FRONT_PANEL_FRONT_PANEL_H_
#define COMPONENTS_FRONT_PANEL_FRONT_PANEL_H_

# include <esp_err.h>
#include "front_light.h"

class FrontPanel
{
    
    FrontLight m_frontLight;
    
public:
    FrontPanel();
    virtual ~FrontPanel();
    
    esp_err_t update(void);
};

#endif /* COMPONENTS_FRONT_PANEL_FRONT_PANEL_H_ */
