/*
 * Copyright (C) 2016
 *
 * This file is part of Paparazzi.
 *
 * Paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Paparazzi; see the file COPYING.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef OPTICAL_FLOW_HOVER_H_
#define OPTICAL_FLOW_HOVER_H_

#include "std.h"
// Without optitrack set to: GUIDANCE_V/H_MODE_ATTITUDE
// With optitrack set to: GUIDANCE_V/H_MODE_NAV
// To use the Optical Flow Hover module use GUIDANCE_V/H_MODE_MODULE

#define GUIDANCE_H_MODE_MODULE_SETTING GUIDANCE_H_MODE_MODULE
//#define GUIDANCE_H_MODE_MODULE_SETTING GUIDANCE_H_MODE_NAV

//#define GUIDANCE_V_MODE_MODULE_SETTING GUIDANCE_V_MODE_MODULE
#define GUIDANCE_V_MODE_MODULE_SETTING GUIDANCE_V_MODE_NAV

bool oscphi;
bool osctheta;
bool derotated;
bool cov_method; 			   ///< method to calculate the covariance: between thrust and div / angle and flow (0) or div and div past / flow and past flow(1)
uint8_t hover_method;		   ///< Method used to hover 0 = All axis after each other; 1 = all axis at the same time; 2 = vertical only, use relation to set horizontal

extern struct OpticalFlowHover of_hover;
extern struct NominalValues of_hover_nominal;
extern struct OpticalFlowHoverControl of_hover_ctrl_X;
extern struct OpticalFlowHoverControl of_hover_ctrl_Y;
extern struct OpticalFlowHoverControl of_hover_ctrl_Z;

// The module functions
extern void optical_flow_hover_init(void);
extern void optical_flow_hover_start(void);
extern void optical_flow_hover_periodic(void);
extern void optical_flow_hover_stop(void);

// Implement own Vertical loops
extern void guidance_v_module_init(void);
extern void guidance_v_module_enter(void);
extern void guidance_v_module_run(bool in_flight);

// Implement own Horizontal loops
extern void guidance_h_module_init(void);
extern void guidance_h_module_enter(void);
extern void guidance_h_module_run(bool in_flight);

#endif /* OPTICAL_FLOW_LANDING_H_ */