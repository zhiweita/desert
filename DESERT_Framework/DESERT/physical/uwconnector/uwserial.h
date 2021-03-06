//
// Copyright (c) 2019 Regents of the SIGNET lab, University of Padova.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the name of the University of Padova (SIGNET lab) nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/**
 * @file uwserial.h
 * @author Emanuele Coccolo
 * \version 1.0.0
 * \brief This implements a generic serial connector .
 */

#ifndef MSERIAL_H
#define MSERIAL_H

#include "uwconnector.h"

#include <cstdio>
#include <cstring>
#include <fcntl.h> // File controls like O_RDWR
#include <termios.h> // POSIX terminal control definitions
#include <unistd.h> // write(), read(), close()

/**
 * Class that implements serial port connection.
 *
 *
 */
class UwSerial : public UwConnector
{

public:
	/**
	 * Constructor of the UwSerial class
	 */
	UwSerial();

	/**
	 * Destructor of the UwSerial class
	 */
	virtual ~UwSerial();

	/**
	 * Method that opens a serial connection, the input string has to be with
	 * a format like <port_name>:parity:stop:flow:baud,
	 * e.g. tty0:p=0:s=1:f=0:b=123456
	 * @return boolean true if connection is correctly opened, false otherwise
	 */
	virtual bool openConnection(const std::string &path);

	/**
	 * Method that closes an active connection to a device
	 * @return boolean true if connection is correctly closed, false otherwise
	 */
	virtual bool closeConnection();

	/**
	 * Returns true if serial port fd differs from -1, that means the
	 * connection is up
	 * @return if serial port file descriptor is valid
	 */
	virtual const bool isConnected();

	/**
	 * Method that writes a command to the port interface
	 * @param msg std::string command to be sent through the port
	 */
	virtual int writeToDevice(const std::string& msg);

	/**
	 * Function that receives data from the device's port to a backup buffer.
	 * The unloaded data is saved to a temporary buffer, to be parsed later.
	 * @param pos position to start writing data to: a pointer to some buffer
	 * @return integer number of read bytes
	 */
	virtual int readFromDevice(void *wpos, int maxlen);

	/**
	 * Method that loads the termios struct with the serial port parameters.
	 * @param path const std::string with address and flag
	 */
	virtual int configurePort(const std::string &path);

	/**
	 * Method that refreshes an existing connection creating a new file
	 * descriptor.
	 * @param path const std::string with address and flag
	 */
	virtual bool refreshConnection(const std::string &path);

private:
	/**
	 * Integer value that stores the serial port descriptor as generated by the
	 * function UwSerial:openConnection().
	 */
	int serialfd;

	struct termios tty;
};

#endif
