struct _configFile {
	bool empty = true;
	const char* api;
	const char* ip;
};

struct _rgb {
	int r;
	int g;
	int b;
	int brightness;
	int ct;
};