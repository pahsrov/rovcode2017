#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <string>
#include <stdint.h>

#include "../include/joystick.h"


joystick::joystick(const char* inputname)
{
	fd = 0;
	version = 0;
	num_buttons = 0;
	num_axes = 0;
	input = (char*)inputname;
}

int joystick::init()
{
	if ((fd = open(input, O_RDONLY)) == -1)
	{
		std::cout << "Could not read input device: " << input << std::endl;
		return 1;
	}

	ioctl(fd, JSIOCGVERSION, &version);
	ioctl(fd, JSIOCGBUTTONS, &num_buttons);
	ioctl(fd, JSIOCGAXES, &num_axes);
	ioctl(fd, JSIOCGNAME(sizeof(name)), &name);
	
	axes = new double [num_axes];
	buttons = new bool [num_buttons];
	
	fcntl(fd, F_SETFL, O_NONBLOCK);
	return 0;
}

joystick::~joystick()
{
	close(fd);
}

int joystick::getversion()
{
	return version;
}

int joystick::getnumberofbuttons()
{
	return num_buttons;
}

int joystick::getnumberofaxes()
{
	return num_axes;
}

char* joystick::getname()
{
	return name;
}

bool* joystick::getbuttons()
{
	return buttons;
}

double* joystick::getaxes()
{
	return axes;
}

void joystick::readevent()
{
	read(fd, &ev, sizeof(ev));
	
	
	switch(ev.type & ~JS_EVENT_INIT)
	{
		case JS_EVENT_AXIS:
			axes[ev.number] = (double) ev.value / 32767.;
		case JS_EVENT_BUTTON:
			if (ev.value == 1)
			{
				buttons[ev.number] = true;
			}
			else
			{
				buttons[ev.number] = false;
			}
	}
}
