#ifndef PN_SYSMAC_SYSMOVIE_H
#define PN_SYSMAC_SYSMOVIE_H 1

/************************************************************************************************
	sysMovie
*************************************************************************************************/

class sysMovie
{
public:
	sysMovie();
	~sysMovie();

	bool Load(const char*);
	void Start(int ms);
	void Pause();
	void Stop();
	void UpdateFrame(int ms);
	void DrawFrame(float left, float bottom, float right, float top);
	void Enable(bool toggle);
	bool Playing();

struct platimp;
private:
	platimp* _platimp; // *all* platform specific stuff goes here
};

#endif // PN_SYSMAC_SYSMOVIE_H
