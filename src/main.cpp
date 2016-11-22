#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include "../include/joystick.h"

struct htonFormat
{
	short int motors[6];
	
	short int claw;
	
	bool shutOff;
	bool lightState;
	bool invokeSensor;
	
};

std::string defaultConfig = "input=/dev/input/js0\narduino_ip=192.168.2.11\narduino_port=12345";

double map(double i, double min, double max, double omin, double omax)
{
        return (i - min) * (omax - omin) / (max - min) + omin;
}


int main(int argc, char *argv[])
{
	// unnecessary formatting
	
	std::cout << "\033[3;34m-------------------------------PAHS ROV STARTING--------------------------------\033[0m\n";
	
	
	// START CONFIG INIT
	
	std::map<std::string, std::string> config;
	
	mapConfig:
	
	std::ifstream configFileI;
	configFileI.open("config.config");
	
	
	std::string configBuffer;
	
	if (configFileI.is_open())
	{
		while (std::getline(configFileI, configBuffer))
		{
			std::istringstream configLine(configBuffer);
			std::string key;
			if(std::getline(configLine, key, '='))
			{
				std::string value;
				if(std::getline(configLine, value))
				{
					config.insert(std::pair<std::string, std::string>(key, value));
				}
			}
		
		}
		
		if (config.size() == 0)
		{
			goto newConfig;
		}
	}
	else
	{
		newConfig:
		
		std::cout << "Error opening config file.\nAttempting to create new one.\n";
		
		std::ofstream configFileO;
		configFileO.open("config.config");
		
		if (configFileO.is_open())
		{
			configFileO << defaultConfig;
			goto mapConfig;
		}
		else
		{
			std::cout << "Config file error. exiting program.\n";
			return 1;
		}
		
		configFileO.close();
		
	}
	
	// END CONFIG INIT
	
	// START JS SETUP
	
	joystick js(config.find("input")->second.c_str());
	
	if(js.init() == 1) return 0;
	
	std::cout << "Name: " << js.getname() << "\n";
	std::cout << "# of buttons: " << js.getnumberofbuttons() << "\n";
	std::cout << "# of axes: " << js.getnumberofaxes() << "\n";
	
	// END JS SETUP
	
	// START SOCKET SETUP
	
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	struct sockaddr_in sockopt;
	sockopt.sin_family = AF_INET;
	sockopt.sin_port = htons(atoi(config.find("arduino_port")->second.c_str()));
	
	if (sockfd < 0)
	{
		std::cout << "Socket Creation error. File descripter " << sockfd << ".\n";
		return 1;
	}
	
	if (inet_pton(AF_INET, config.find("arduino_ip")->second.c_str(), &sockopt.sin_addr) <= 0)
	{
		std::cout << "Socket inetpton error.\n";
		return 1;
	}
	
	
	// END SOCKET SETUP
	
	// START LOOP
	
	while(1)
	{
		js.readevent();
		double* axes = js.getaxes();
		double motors[5];
		bool* buttons = js.getbuttons();
		
		// FORWARD MOTORS (FORWARD/REVERSE Y PIVOT X)
		double turn = map(axes[0], -1, 1, -400, 400);
		
		if(turn > 0)
		{
			motors[0] = map(axes[1], -1, 1, 1900, 1100) + turn;
			motors[1] = map(axes[1], -1, 1, 1900, 1100) - turn;
		}
		else
		{
			motors[0] = map(axes[1], -1, 1, 1900, 1100) - turn;
			motors[1] = map(axes[1], -1, 1, 1900, 1100) + turn;
		}
		
		// add curve func
		
		
		// END FORWARD MOTORS
		
		// START VERTICAL MOTORS
		
		motors[2] = motors[3] = map(axes[4], -1, 1, 1900, 1100);
		
			//insert curve stuff
			
		// END VERTICAL MOTORS
		
		// START STRAFE
		
		motors[4] = map(axes[3], -1, 1, 1100, 1900);
		
		// END STRAFE
		
		
		// PACKAGE ASSIGNMENT
		
		htonFormat pack;
		
		for (unsigned int i=0; i < sizeof(pack.motors)/sizeof(short int); i++)
		{
			pack.motors[i] = map(motors[i], -1, 1, 1100, 1900);
		}
		
		if (buttons[5] == 1) pack.claw = 1;
		else if (buttons[4] == 1) pack.claw = -1;
		else pack.claw = 0;
		
		// Shutoff
		
		static bool shutOffDB = false;
		
		if(buttons[6] == 1 && shutOffDB == false) 
		{
			shutOffDB = true;
			if(pack.shutOff == true) 
				pack.shutOff = false;
			else 
				pack.shutOff = true;
		}
		else if(buttons[6] == 0)
			shutOffDB = false;
		
		// LIGHT
		
		static bool lightStateDB = false;
		
		if(buttons[2] == 1 && lightStateDB == false) 
		{
			lightStateDB = true;
			if(pack.lightState == true) 
				pack.lightState = false;
			else 
				pack.lightState = true;
		}
		else if(buttons[2] == 0)
			lightStateDB = false;
			
		// END LIGHT
		
		//pack.invokeSensor = buttons[3];
		
		// END PACKAGE ASSIGNMENT
		
		// start udp
		
		static htonFormat prevPack;
		static bool firstIteration = true;
		
		if(memcmp(&pack, &prevPack, sizeof(pack)) == 0 || firstIteration == true)
		{
			firstIteration = false;
			if(sendto(sockfd, &pack, sizeof(pack), 0, (const struct sockaddr *)&sockopt, sizeof(sockopt)) < 0)
			{
				perror("sento: ");
				return 1;
			}
		}
		
		std::cout << "\033[3;34mINPUT INFO: \033[0m\n" << "FRONTLEFT : " << motors[0] << "\n" << "FRONTRIGHT: " << motors[1] << "\n" << "VERTICAL: " << motors[2] << "\n" << "STRAFE: " << motors[4] << "\n";
		
		std::cout << "\x1b[A\x1b[A\x1b[A\x1b[A\x1b[A"; // move cursor up four lines
		
		// end udp
		
		// get sensor info
		
		memcpy(&prevPack, &pack, sizeof(pack));
		
	}
	
	// END LOOP (does it ever end?)
	
	return 0;
	
}
