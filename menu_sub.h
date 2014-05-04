/*
 * menu_sub.h
 *
 *  Created on: 17.06.2012
 *****************************************************************************
 *	MCmega - Firmware for the Motorola MC micro radio
 *           to use it as an Amateur-Radio transceiver
 *
 * Copyright (C) 2013 Felix Erckenbrecht, DG1YFE
 *
 * ( AVR port of "MC70"
 *   Copyright (C) 2004 - 2013  Felix Erckenbrecht, DG1YFE)
 *
 * This file is part of MCmega.
 *
 * MCmega is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MCmega is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MCmega.  If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************
 */

#ifndef MENU_SUB_H_
#define MENU_SUB_H_


void m_power();
void m_power_submenu(char key);
void m_cfgsave_submenu(char key);
void m_version_submenu(char key);
void m_ctcss_tx(char key);
void m_ctcss_rx(char key);
void m_ctcss_submenu(char key);
void m_cal_submenu(char key);

#endif /* MENU_SUB_H_ */
