/****************************************************************************
 *
 *   Copyright (c) 2014 MAVlink Development Team. All rights reserved.
 *   Author: Trent Lukaczyk, <aerialhedgehog@gmail.com>
 *           Jaycee Lock,    <jaycee.lock@gmail.com>
 *           Lorenz Meier,   <lm@inf.ethz.ch>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file autopilot_interface.cpp
 *
 * @brief Autopilot interface functions
 *
 * Functions for sending and recieving commands to an autopilot via MAVlink
 *
 * @author Trent Lukaczyk, <aerialhedgehog@gmail.com>
 * @author Jaycee Lock,    <jaycee.lock@gmail.com>
 * @author Lorenz Meier,   <lm@inf.ethz.ch>
 *
 */


// ------------------------------------------------------------------------------
//   Includes
// ------------------------------------------------------------------------------

#include <iostream>

#include "autopilot_interface.h"
#include "quaternion.h"

// ----------------------------------------------------------------------------------
//   Time
// ------------------- ---------------------------------------------------------------
uint64_t
get_time_usec()
{
	static struct timeval _time_stamp;
	gettimeofday(&_time_stamp, NULL);
	return _time_stamp.tv_sec*1000000 + _time_stamp.tv_usec;
}


// ----------------------------------------------------------------------------------
//   Setpoint Helper Functions
// ----------------------------------------------------------------------------------

// choose one of the next three

/*
 * Set target local ned position
 *
 * Modifies a mavlink_set_position_target_local_ned_t struct with target XYZ locations
 * in the Local NED frame, in meters.
 */
void
set_position(float x, float y, float z, mavlink_set_position_target_local_ned_t &sp)
{
	sp.type_mask =
		MAVLINK_MSG_SET_POSITION_TARGET_LOCAL_NED_POSITION;

	sp.coordinate_frame = MAV_FRAME_LOCAL_NED;

	sp.x   = x;
	sp.y   = y;
	sp.z   = z;

	printf("POSITION SETPOINT XYZ = [ %.4f , %.4f , %.4f ] \n", sp.x, sp.y, sp.z);

}

/*
 * Set target local ned velocity
 *
 * Modifies a mavlink_set_position_target_local_ned_t struct with target VX VY VZ
 * velocities in the Local NED frame, in meters per second.
 */
void
set_velocity(float vx, float vy, float vz, mavlink_set_position_target_local_ned_t &sp)
{
	sp.type_mask =
		MAVLINK_MSG_SET_POSITION_TARGET_LOCAL_NED_VELOCITY     ;

	sp.coordinate_frame = MAV_FRAME_LOCAL_NED;

	sp.vx  = vx;
	sp.vy  = vy;
	sp.vz  = vz;

	printf("VELOCITY SETPOINT UVW = [ %.4f , %.4f , %.4f ] \n", sp.vx, sp.vy, sp.vz);

}

/*
 * Set target local ned acceleration
 *
 * Modifies a mavlink_set_position_target_local_ned_t struct with target AX AY AZ
 * accelerations in the Local NED frame, in meters per second squared.
 */
void
set_acceleration(float ax, float ay, float az, mavlink_set_position_target_local_ned_t &sp)
{

	// NOT IMPLEMENTED
	fprintf(stderr,"set_acceleration doesn't work yet \n");
	throw 1;


	sp.type_mask =
		MAVLINK_MSG_SET_POSITION_TARGET_LOCAL_NED_ACCELERATION &
		MAVLINK_MSG_SET_POSITION_TARGET_LOCAL_NED_VELOCITY     ;

	sp.coordinate_frame = MAV_FRAME_LOCAL_NED;

	sp.afx  = ax;
	sp.afy  = ay;
	sp.afz  = az;
}

// the next two need to be called after one of the above

/*
 * Set target local ned yaw
 *
 * Modifies a mavlink_set_position_target_local_ned_t struct with a target yaw
 * in the Local NED frame, in radians.
 */
void
set_yaw(float yaw, mavlink_set_position_target_local_ned_t &sp)
{
	sp.type_mask &=
		MAVLINK_MSG_SET_POSITION_TARGET_LOCAL_NED_YAW_ANGLE ;

	sp.yaw  = yaw;

	printf("POSITION SETPOINT YAW = %.4f \n", sp.yaw);

}

/*
 * Set target local ned yaw rate
 *
 * Modifies a mavlink_set_position_target_local_ned_t struct with a target yaw rate
 * in the Local NED frame, in radians per second.
 */
void
set_yaw_rate(float yaw_rate, mavlink_set_position_target_local_ned_t &sp)
{
	sp.type_mask &=
		MAVLINK_MSG_SET_POSITION_TARGET_LOCAL_NED_YAW_RATE ;

	sp.yaw_rate  = yaw_rate;
}


// ----------------------------------------------------------------------------------
//   Autopilot Interface Class
// ----------------------------------------------------------------------------------

// ------------------------------------------------------------------------------
//   Con/De structors
// ------------------------------------------------------------------------------
Autopilot_Interface::
Autopilot_Interface(Serial_Port *serial_port_)
{
	// initialize attributes
	write_count = 0;

	reading_status = 0;      // whether the read thread is running
	writing_status = 0;      // whether the write thread is running
	control_status = 0;      // whether the autopilot is in offboard control mode
	time_to_exit   = false;  // flag to signal thread exit

	read_tid  = 0; // read thread id
	write_tid = 0; // write thread id

	system_id    = 0; // system id
	autopilot_id = 0; // autopilot component id
	companion_id = 0; // companion computer component id

	current_messages.sysid  = system_id;
	current_messages.compid = autopilot_id;

	serial_port = serial_port_; // serial port management object

}

Autopilot_Interface::
~Autopilot_Interface()
{}


// ------------------------------------------------------------------------------
//   Update Setpoint
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
update_setpoint(mavlink_set_position_target_local_ned_t setpoint)
{
	current_setpoint = setpoint;
}


// ------------------------------------------------------------------------------
//   Read Messages
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
read_messages()
{
	bool success;               // receive success flag
	bool received_all = false;  // receive only one message
	Time_Stamps this_timestamps;

	// Blocking wait for new data
	while ( not received_all and not time_to_exit )
	{
		// ----------------------------------------------------------------------
		//   READ MESSAGE
		// ----------------------------------------------------------------------
		mavlink_message_t message;
		success = serial_port->read_message(message);

		// ----------------------------------------------------------------------
		//   HANDLE MESSAGE
		// ----------------------------------------------------------------------
		if( success )
		{

			// Store message sysid and compid.
			// Note this doesn't handle multiple message sources.
			current_messages.sysid  = message.sysid;
			current_messages.compid = message.compid;

			// Handle Message ID
			switch (message.msgid)
			{

				case MAVLINK_MSG_ID_HEARTBEAT:
				{
					std::cout << "MAVLINK_MSG_ID_HEARTBEAT" << std::endl;
					mavlink_msg_heartbeat_decode(&message, &(current_messages.heartbeat));
					current_messages.time_stamps.heartbeat = get_time_usec();
					this_timestamps.heartbeat = current_messages.time_stamps.heartbeat;
					break;
				}

				case MAVLINK_MSG_ID_SYS_STATUS:
				{
					//std::cout << "MAVLINK_MSG_ID_SYS_STATUS" << std::endl;
					mavlink_msg_sys_status_decode(&message, &(current_messages.sys_status));
					current_messages.time_stamps.sys_status = get_time_usec();
					this_timestamps.sys_status = current_messages.time_stamps.sys_status;
					break;
				}

				case MAVLINK_MSG_ID_BATTERY_STATUS:
				{
					std::cout << "MAVLINK_MSG_ID_BATTERY_STATUS" << std::endl;
					mavlink_msg_battery_status_decode(&message, &(current_messages.battery_status));
					current_messages.time_stamps.battery_status = get_time_usec();
					this_timestamps.battery_status = current_messages.time_stamps.battery_status;
					break;
				}

				case MAVLINK_MSG_ID_RADIO_STATUS:
				{
					std::cout << "MAVLINK_MSG_ID_RADIO_STATUS" << std::endl;
					mavlink_msg_radio_status_decode(&message, &(current_messages.radio_status));
					current_messages.time_stamps.radio_status = get_time_usec();
					this_timestamps.radio_status = current_messages.time_stamps.radio_status;
					break;
				}

				case MAVLINK_MSG_ID_LOCAL_POSITION_NED:
				{
					std::cout << "MAVLINK_MSG_ID_LOCAL_POSITION_NED" << std::endl;
					mavlink_msg_local_position_ned_decode(&message, &(current_messages.local_position_ned));
					current_messages.time_stamps.local_position_ned = get_time_usec();
					this_timestamps.local_position_ned = current_messages.time_stamps.local_position_ned;
					break;
				}

				case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
				{
					std::cout << "MAVLINK_MSG_ID_GLOBAL_POSITION_INT" << std::endl;
					mavlink_msg_global_position_int_decode(&message, &(current_messages.global_position_int));
					current_messages.time_stamps.global_position_int = get_time_usec();
					this_timestamps.global_position_int = current_messages.time_stamps.global_position_int;
					break;
				}

				case MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED:
				{
					std::cout << "MAVLINK_MSG_ID_POSITION_TARGET_LOCAL_NED" << std::endl;
					mavlink_msg_position_target_local_ned_decode(&message, &(current_messages.position_target_local_ned));
					current_messages.time_stamps.position_target_local_ned = get_time_usec();
					this_timestamps.position_target_local_ned = current_messages.time_stamps.position_target_local_ned;
					break;
				}

				case MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT:
				{
					std::cout << "MAVLINK_MSG_ID_POSITION_TARGET_GLOBAL_INT" << std::endl;
					mavlink_msg_position_target_global_int_decode(&message, &(current_messages.position_target_global_int));
					current_messages.time_stamps.position_target_global_int = get_time_usec();
					this_timestamps.position_target_global_int = current_messages.time_stamps.position_target_global_int;
					break;
				}

				case MAVLINK_MSG_ID_HIGHRES_IMU:
				{
					std::cout << "MAVLINK_MSG_ID_HIGHRES_IMU" << std::endl;
					mavlink_msg_highres_imu_decode(&message, &(current_messages.highres_imu));
					current_messages.time_stamps.highres_imu = get_time_usec();
					this_timestamps.highres_imu = current_messages.time_stamps.highres_imu;
					break;
				}

				case MAVLINK_MSG_ID_ATTITUDE:
				{
          vector4d quaternion;
					std::cout << "MAVLINK_MSG_ID_ATTITUDE" << std::endl;
					mavlink_msg_attitude_decode(&message, &(current_messages.attitude));
					current_messages.time_stamps.attitude = get_time_usec();
					this_timestamps.attitude = current_messages.time_stamps.attitude;
          quaternion_from_euler(&quaternion, current_messages.attitude.roll,
            current_messages.attitude.pitch, current_messages.attitude.yaw);
					std::cout <<
						"\troll: " << current_messages.attitude.roll <<
						"\tpitch: " << current_messages.attitude.pitch <<
						"\tyaw: " << current_messages.attitude.yaw <<
						std::endl;
					std::cout <<
						"\trollspeed: " << current_messages.attitude.rollspeed <<
						"\tpitchspeed: " << current_messages.attitude.pitchspeed <<
						"\tyawspeed: " << current_messages.attitude.yawspeed <<
						std::endl;
					std::cout <<
						"\tqx: " << quaternion.x <<
						"\tqy: " << quaternion.y <<
						"\tqz: " << quaternion.z <<
						"\tqw: " << quaternion.w <<
						std::endl;
					break;
				}

				case MAVLINK_MSG_ID_DEBUG:
				{
					std::cout << "MAVLINK_MSG_ID_DEBUG" << std::endl;
/*					mavlink_msg_debug_decode(&message, &(current_messages.attitude));
					current_messages.time_stamps.attitude = get_time_usec();
					this_timestamps.attitude = current_messages.time_stamps.attitude;*/
					break;
				}

				case MAVLINK_MSG_ID_STATUSTEXT:
				{
					std::cout << "MAVLINK_MSG_ID_STATUSTEXT: ";
					mavlink_msg_statustext_decode(&message, &(current_messages.statustext));
          current_messages.statustext.text[50] = 0;
          printf("%d - '%s'\n", current_messages.statustext.severity, current_messages.statustext.text);
					current_messages.time_stamps.statustext = get_time_usec();
					this_timestamps.statustext = current_messages.time_stamps.statustext;
					break;
				}

				case MAVLINK_MSG_ID_RAW_IMU:
				{
					std::cout << "MAVLINK_MSG_ID_RAW_IMU:" << std::endl;
					mavlink_msg_raw_imu_decode(&message, &(current_messages.raw_imu));
					std::cout << "\tacc :\t" << current_messages.raw_imu.xacc <<
						"\t" << current_messages.raw_imu.yacc <<
						"\t" << current_messages.raw_imu.zacc <<
						std::endl;
					std::cout << "\tgyro:\t" << current_messages.raw_imu.xgyro <<
						"\t" << current_messages.raw_imu.ygyro <<
						"\t" << current_messages.raw_imu.zgyro <<
						std::endl;
					std::cout << "\tmag:\t" << current_messages.raw_imu.xmag <<
						"\t" << current_messages.raw_imu.ymag <<
						"\t" << current_messages.raw_imu.zmag <<
						std::endl;
					current_messages.time_stamps.raw_imu = get_time_usec();
					this_timestamps.raw_imu = current_messages.time_stamps.raw_imu;
					break;
				}

				case MAVLINK_MSG_ID_GPS_RAW_INT:
				{
					std::cout << "MAVLINK_MSG_ID_GPS_RAW_INT:" << std::endl;
					mavlink_msg_gps_raw_int_decode(&message, &(current_messages.gps_raw_int));
					current_messages.time_stamps.gps_raw_int = get_time_usec();
					this_timestamps.gps_raw_int = current_messages.time_stamps.gps_raw_int;
					std::cout <<
						"\tlat: " << current_messages.gps_raw_int.lat <<
						"\tlon: " << current_messages.gps_raw_int.lon <<
						"\talt: " << current_messages.gps_raw_int.alt <<
						std::endl;
					std::cout <<
						"\teph: " << current_messages.gps_raw_int.eph <<
						"\tepv: " << current_messages.gps_raw_int.epv <<
						std::endl;
					std::cout <<
						"\tvel: " << current_messages.gps_raw_int.vel <<
						"\tcog: " << current_messages.gps_raw_int.cog <<
						std::endl;
					std::cout <<
						"\tfix: " << (int)current_messages.gps_raw_int.fix_type <<
						"\tsat: " << (int)current_messages.gps_raw_int.satellites_visible <<
						std::endl;
					break;
				}

				case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:
				{
          vector4d quaternion;
					std::cout << "MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:" << std::endl;
					mavlink_msg_nav_controller_output_decode(&message, &(current_messages.nav_controller_output));
					current_messages.time_stamps.nav_controller_output = get_time_usec();
					this_timestamps.nav_controller_output = current_messages.time_stamps.nav_controller_output;
          quaternion_from_euler(&quaternion, current_messages.nav_controller_output.nav_roll,
              current_messages.nav_controller_output.nav_pitch, current_messages.nav_controller_output.nav_bearing);
					std::cout <<
						"\tnav_roll: " << current_messages.nav_controller_output.nav_roll <<
						"\tnav_pitch: " << current_messages.nav_controller_output.nav_pitch <<
						"\tnav_bearing: " << current_messages.nav_controller_output.nav_bearing <<
						std::endl;
					std::cout <<
						"\tqx: " << quaternion.x <<
						"\tqy: " << quaternion.y <<
						"\tqz: " << quaternion.z <<
						"\tqw: " << quaternion.w <<
						std::endl;
					std::cout <<
						"\ttarget_bearing: " << current_messages.nav_controller_output.target_bearing <<
						"\twp_dist: " << current_messages.nav_controller_output.wp_dist <<
						std::endl;
					std::cout <<
						"\talt_error: " << current_messages.nav_controller_output.alt_error <<
						"\taspd_error: " << current_messages.nav_controller_output.aspd_error <<
						"\txtrack_error: " << current_messages.nav_controller_output.xtrack_error <<
						std::endl;
					break;
				}

				case MAVLINK_MSG_ID_SCALED_PRESSURE:
				{
					std::cout << "MAVLINK_MSG_ID_SCALED_PRESSURE:" << std::endl;
					mavlink_msg_scaled_pressure_decode(&message, &(current_messages.scaled_pressure));
					current_messages.time_stamps.scaled_pressure = get_time_usec();
					this_timestamps.scaled_pressure = current_messages.time_stamps.scaled_pressure;
					std::cout <<
						"\tpress_abs: " << current_messages.scaled_pressure.press_abs <<
						"\tpress_diff: " << current_messages.scaled_pressure.press_diff <<
						"\ttemperature: " << ((double)current_messages.scaled_pressure.temperature / 100.0) <<
						std::endl;
					break;
				}

				case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
				{
					std::cout << "MAVLINK_MSG_ID_RC_CHANNELS_RAW:" << std::endl;
					mavlink_msg_rc_channels_raw_decode(&message, &(current_messages.rc_channels_raw));
					current_messages.time_stamps.rc_channels_raw = get_time_usec();
					this_timestamps.rc_channels_raw = current_messages.time_stamps.rc_channels_raw;
					std::cout <<
						"\tport: " << (int)current_messages.rc_channels_raw.port <<
						"\tchan1_raw: " << current_messages.rc_channels_raw.chan1_raw <<
						"\tchan2_raw: " << current_messages.rc_channels_raw.chan2_raw <<
						"\tchan3_raw: " << current_messages.rc_channels_raw.chan3_raw <<
						"\tchan4_raw: " << current_messages.rc_channels_raw.chan4_raw <<
						"\tchan5_raw: " << current_messages.rc_channels_raw.chan5_raw <<
						"\tchan6_raw: " << current_messages.rc_channels_raw.chan6_raw <<
						"\tchan7_raw: " << current_messages.rc_channels_raw.chan7_raw <<
						"\tchan8_raw: " << current_messages.rc_channels_raw.chan8_raw <<
						"\trssi: " << (int)current_messages.rc_channels_raw.rssi <<
						std::endl;
					break;
				}

				case MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:
				{
					std::cout << "MAVLINK_MSG_ID_SERVO_OUTPUT_RAW:" << std::endl;
					mavlink_msg_servo_output_raw_decode(&message, &(current_messages.servo_output_raw));
					current_messages.time_stamps.servo_output_raw = get_time_usec();
					this_timestamps.servo_output_raw = current_messages.time_stamps.servo_output_raw;
					std::cout <<
						"\tport: " << (int)current_messages.servo_output_raw.port <<
						"\tservo1_raw: " << current_messages.servo_output_raw.servo1_raw <<
						"\tservo2_raw: " << current_messages.servo_output_raw.servo2_raw <<
						"\tservo3_raw: " << current_messages.servo_output_raw.servo3_raw <<
						"\tservo4_raw: " << current_messages.servo_output_raw.servo4_raw <<
						"\tservo5_raw: " << current_messages.servo_output_raw.servo5_raw <<
						"\tservo6_raw: " << current_messages.servo_output_raw.servo6_raw <<
						"\tservo7_raw: " << current_messages.servo_output_raw.servo7_raw <<
						"\tservo8_raw: " << current_messages.servo_output_raw.servo8_raw <<
						std::endl;
					break;
				}

				case MAVLINK_MSG_ID_VFR_HUD:
				{
					std::cout << "MAVLINK_MSG_ID_VFR_HUD:" << std::endl;
					mavlink_msg_vfr_hud_decode(&message, &(current_messages.vfr_hud));
					current_messages.time_stamps.vfr_hud = get_time_usec();
					this_timestamps.vfr_hud = current_messages.time_stamps.vfr_hud;
					std::cout <<
						"\tairspeed: " << current_messages.vfr_hud.airspeed <<
						"\tgroundspeed: " << current_messages.vfr_hud.groundspeed <<
						"\theading: " << current_messages.vfr_hud.heading <<
						std::endl;
					std::cout <<
						"\tthrottle: " << current_messages.vfr_hud.throttle <<
						"\talt: " << current_messages.vfr_hud.alt <<
						"\tclimb: " << current_messages.vfr_hud.climb <<
						std::endl;
					break;
				}

				case MAVLINK_MSG_ID_MISSION_CURRENT:
				{
					std::cout << "MAVLINK_MSG_ID_MISSION_CURRENT: ";
					mavlink_msg_mission_current_decode(&message, &(current_messages.mission_current));
					current_messages.time_stamps.mission_current = get_time_usec();
					this_timestamps.mission_current = current_messages.time_stamps.mission_current;
					std::cout <<
						"seq: " << current_messages.mission_current.seq <<
						std::endl;
					break;
				}

				default:
				{
					printf("Warning, did not handle message id %i\n",message.msgid);
					break;
				}


			} // end: switch msgid

		} // end: if read message

		// Check for receipt of all items
		received_all =
				this_timestamps.heartbeat                  &&
				this_timestamps.sys_status                 &&
//				this_timestamps.battery_status             &&
//				this_timestamps.radio_status               &&
				this_timestamps.local_position_ned         &&
//				this_timestamps.global_position_int        &&
//				this_timestamps.position_target_local_ned  &&
				this_timestamps.position_target_global_int &&
				this_timestamps.highres_imu                &&
				this_timestamps.attitude                   ;

		// give the write thread time to use the port
		if ( writing_status > false )
			usleep(100); // look for components of batches at 10kHz

	} // end: while not received all

	return;
}

// ------------------------------------------------------------------------------
//   Write Message
// ------------------------------------------------------------------------------
int
Autopilot_Interface::
write_message(mavlink_message_t message)
{
	// do the write
	int len = serial_port->write_message(message);

	// book keep
	write_count++;

	// Done!
	return len;
}

// ------------------------------------------------------------------------------
//   Write Setpoint Message
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
write_setpoint()
{
	// --------------------------------------------------------------------------
	//   PACK PAYLOAD
	// --------------------------------------------------------------------------

	// pull from position target
	mavlink_set_position_target_local_ned_t sp = current_setpoint;

	// double check some system parameters
	if ( not sp.time_boot_ms )
		sp.time_boot_ms = (uint32_t) (get_time_usec()/1000);
	sp.target_system    = system_id;
	sp.target_component = autopilot_id;


	// --------------------------------------------------------------------------
	//   ENCODE
	// --------------------------------------------------------------------------

	mavlink_message_t message;
	mavlink_msg_set_position_target_local_ned_encode(system_id, companion_id, &message, &sp);


	// --------------------------------------------------------------------------
	//   WRITE
	// --------------------------------------------------------------------------

	// do the write
	int len = write_message(message);

	// check the write
	if ( not len > 0 )
		fprintf(stderr,"WARNING: could not send POSITION_TARGET_LOCAL_NED \n");
	//	else
	//		printf("%lu POSITION_TARGET  = [ %f , %f , %f ] \n", write_count, position_target.x, position_target.y, position_target.z);

	return;
}


// ------------------------------------------------------------------------------
//   Start Off-Board Mode
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
enable_offboard_control()
{
	// Should only send this command once
	if ( control_status == false )
	{
		printf("ENABLE OFFBOARD MODE\n");

		// ----------------------------------------------------------------------
		//   TOGGLE OFF-BOARD MODE
		// ----------------------------------------------------------------------

		// Sends the command to go off-board
		int success = toggle_offboard_control( true );

		// Check the command was written
		if ( success )
			control_status = true;
		else
		{
			fprintf(stderr,"Error: off-board mode not set, could not write message\n");
			//throw EXIT_FAILURE;
		}

		printf("\n");

	} // end: if not offboard_status

}


// ------------------------------------------------------------------------------
//   Stop Off-Board Mode
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
disable_offboard_control()
{

	// Should only send this command once
	if ( control_status == true )
	{
		printf("DISABLE OFFBOARD MODE\n");

		// ----------------------------------------------------------------------
		//   TOGGLE OFF-BOARD MODE
		// ----------------------------------------------------------------------

		// Sends the command to stop off-board
		int success = toggle_offboard_control( false );

		// Check the command was written
		if ( success )
			control_status = false;
		else
		{
			fprintf(stderr,"Error: off-board mode not set, could not write message\n");
			//throw EXIT_FAILURE;
		}

		printf("\n");

	} // end: if offboard_status

}


// ------------------------------------------------------------------------------
//   Toggle Off-Board Mode
// ------------------------------------------------------------------------------
int
Autopilot_Interface::
toggle_offboard_control( bool flag )
{
	// Prepare command for off-board mode
	mavlink_command_long_t com;
	com.target_system    = system_id;
	com.target_component = autopilot_id;
	com.command          = MAV_CMD_NAV_GUIDED_ENABLE;
	com.confirmation     = true;
	com.param1           = (float) flag; // flag >0.5 => start, <0.5 => stop

	// Encode
	mavlink_message_t message;
	mavlink_msg_command_long_encode(system_id, companion_id, &message, &com);

	// Send the message
	int len = serial_port->write_message(message);

	// Done!
	return len;
}


// ------------------------------------------------------------------------------
//   STARTUP
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
start()
{
	int result;

	// --------------------------------------------------------------------------
	//   CHECK SERIAL PORT
	// --------------------------------------------------------------------------

	if ( not serial_port->status == 1 ) // SERIAL_PORT_OPEN
	{
		fprintf(stderr,"ERROR: serial port not open\n");
		throw 1;
	}


	// --------------------------------------------------------------------------
	//   READ THREAD
	// --------------------------------------------------------------------------

	printf("START READ THREAD \n");

	result = pthread_create( &read_tid, NULL, &start_autopilot_interface_read_thread, this );
	if ( result ) throw result;

	// now we're reading messages
	printf("\n");

	// --------------------------------------------------------------------------
	//   CHECK FOR MESSAGES
	// --------------------------------------------------------------------------

	printf("CHECK FOR MESSAGES\n");

	while ( not current_messages.sysid )
	{
		if ( time_to_exit )
			return;
		usleep(500000); // check at 2Hz
	}

	printf("Found\n");

	// now we know autopilot is sending messages
	printf("\n");


	// --------------------------------------------------------------------------
	//   GET SYSTEM and COMPONENT IDs
	// --------------------------------------------------------------------------

	// This comes from the heartbeat, which in theory should only come from
	// the autopilot we're directly connected to it.  If there is more than one
	// vehicle then we can't expect to discover id's like this.
	// In which case set the id's manually.

	// System ID
	if ( not system_id )
	{
		system_id = current_messages.sysid;
		printf("GOT VEHICLE SYSTEM ID: %i\n", system_id );
	}

	// Component ID
	if ( not autopilot_id )
	{
		autopilot_id = current_messages.compid;
		printf("GOT AUTOPILOT COMPONENT ID: %i\n", autopilot_id);
		printf("\n");
	}

////////////////////////////////////////////////////////////////////////////////
mavlink_message_t message;
mavlink_request_data_stream_t req;
                req.req_message_rate = 1;
                req.req_stream_id = MAV_DATA_STREAM_ALL;
                //req.req_stream_id = MAV_DATA_STREAM_RAW_SENSORS;
                req.start_stop = 1;
                req.target_system = current_messages.sysid;
                req.target_component = current_messages.compid;
                mavlink_msg_request_data_stream_encode(system_id, companion_id, &message, &req);
                // Send the message
	int len = serial_port->write_message(message);
////////////////////////////////////////////////////////////////////////////////
                
                


	// --------------------------------------------------------------------------
	//   GET INITIAL POSITION
	// --------------------------------------------------------------------------

	// Wait for initial position ned
//	while ( not ( current_messages.time_stamps.local_position_ned &&
	while ( not ( current_messages.time_stamps.raw_imu &&
				  current_messages.time_stamps.attitude            )  )
	{
		if ( time_to_exit )
			return;
		usleep(500000);
	}

	// copy initial position ned
	Mavlink_Messages local_data = current_messages;
	initial_position.x        = local_data.local_position_ned.x;
	initial_position.y        = local_data.local_position_ned.y;
	initial_position.z        = local_data.local_position_ned.z;
	initial_position.vx       = local_data.local_position_ned.vx;
	initial_position.vy       = local_data.local_position_ned.vy;
	initial_position.vz       = local_data.local_position_ned.vz;
	initial_position.yaw      = local_data.attitude.yaw;
	initial_position.yaw_rate = local_data.attitude.yawspeed;

	printf("INITIAL POSITION XYZ = [ %.4f , %.4f , %.4f ] \n", initial_position.x, initial_position.y, initial_position.z);
	printf("INITIAL POSITION YAW = %.4f \n", initial_position.yaw);
	printf("\n");

	// we need this before starting the write thread


	// --------------------------------------------------------------------------
	//   WRITE THREAD
	// --------------------------------------------------------------------------
	printf("START WRITE THREAD \n");

	result = pthread_create( &write_tid, NULL, &start_autopilot_interface_write_thread, this );
	if ( result ) throw result;

	// wait for it to be started
//	while ( not writing_status )
	while ( true )
		usleep(100000); // 10Hz

	// now we're streaming setpoint commands
	printf("\n");


	// Done!
	return;

}


// ------------------------------------------------------------------------------
//   SHUTDOWN
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
stop()
{
	// --------------------------------------------------------------------------
	//   CLOSE THREADS
	// --------------------------------------------------------------------------
	printf("CLOSE THREADS\n");

	// signal exit
	time_to_exit = true;

	// wait for exit
	pthread_join(read_tid ,NULL);
	pthread_join(write_tid,NULL);

	// now the read and write threads are closed
	printf("\n");

	// still need to close the serial_port separately
}

// ------------------------------------------------------------------------------
//   Read Thread
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
start_read_thread()
{

	if ( reading_status != 0 )
	{
		fprintf(stderr,"read thread already running\n");
		return;
	}
	else
	{
		read_thread();
		return;
	}

}


// ------------------------------------------------------------------------------
//   Write Thread
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
start_write_thread(void)
{
	if ( not writing_status == false )
	{
		fprintf(stderr,"write thread already running\n");
		return;
	}

	else
	{
		write_thread();
		return;
	}

}


// ------------------------------------------------------------------------------
//   Quit Handler
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
handle_quit( int sig )
{

	disable_offboard_control();

	try {
		stop();

	}
	catch (int error) {
		fprintf(stderr,"Warning, could not stop autopilot interface\n");
	}

}



// ------------------------------------------------------------------------------
//   Read Thread
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
read_thread()
{
	reading_status = true;

	while ( not time_to_exit )
	{
		read_messages();
		usleep(100000); // Read batches at 10Hz
	}

	reading_status = false;

	return;
}


// ------------------------------------------------------------------------------
//   Write Thread
// ------------------------------------------------------------------------------
void
Autopilot_Interface::
write_thread(void)
{
	// signal startup
	writing_status = 2;

	// prepare an initial setpoint, just stay put
	mavlink_set_position_target_local_ned_t sp;
	sp.type_mask = MAVLINK_MSG_SET_POSITION_TARGET_LOCAL_NED_VELOCITY &
				   MAVLINK_MSG_SET_POSITION_TARGET_LOCAL_NED_YAW_RATE;
	sp.coordinate_frame = MAV_FRAME_LOCAL_NED;
	sp.vx       = 0.0;
	sp.vy       = 0.0;
	sp.vz       = 0.0;
	sp.yaw_rate = 0.0;

	// set position target
	current_setpoint = sp;

	// write a message and signal writing
	write_setpoint();
	writing_status = true;

	// Pixhawk needs to see off-board commands at minimum 2Hz,
	// otherwise it will go into fail safe
	while ( not time_to_exit )
	{
		usleep(125000);   // Stream at 4Hz
		write_setpoint();
	}

	// signal end
	writing_status = false;

	return;

}

// End Autopilot_Interface


// ------------------------------------------------------------------------------
//  Pthread Starter Helper Functions
// ------------------------------------------------------------------------------

void*
start_autopilot_interface_read_thread(void *args)
{
	// takes an autopilot object argument
	Autopilot_Interface *autopilot_interface = (Autopilot_Interface *)args;

	// run the object's read thread
	autopilot_interface->start_read_thread();

	// done!
	return NULL;
}

void*
start_autopilot_interface_write_thread(void *args)
{
	// takes an autopilot object argument
	Autopilot_Interface *autopilot_interface = (Autopilot_Interface *)args;

	// run the object's read thread
	autopilot_interface->start_write_thread();

	// done!
	return NULL;
}



