#include <linux/joystick.h>
#include <string>
#include <stdint.h>

class joystick
{
private:

	int fd,
	version,
	num_buttons,
	num_axes;
	
	double* axes;
	
	bool* buttons;
	
	char name [64];
	
	char* input;
	
	struct js_event ev;
	
	
public:
	joystick(const char* inputname);
	
	~joystick();
	
	int init();
	
	int getversion();
	
	int getnumberofbuttons();
	
	int getnumberofaxes();
	
	char* getname();
	
	bool* getbuttons();
	
	double* getaxes();
	
	void readevent();
};
