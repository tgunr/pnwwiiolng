#ifndef PHUI_PERIODICAL_H
#define PHUI_PERIODICAL_H

namespace phui
{

/************************************************************************************************
	Periodical
*************************************************************************************************/
/** 
	A class that executes itself periodically
*/
class Periodical 
{
public:
typedef void (*callback)(Periodical*, unsigned int t, void*);
	static void Update(unsigned int t);
	
	Periodical(callback cb = 0, void* data = 0);
	virtual ~Periodical();

public:
	virtual void StartExecuting() { _executing = true; _timeinited = false;} 
	virtual void StopExecuting()  { _executing = false;}///< is off by default
	
	virtual void SetInterval(unsigned int ms) { _interval = ms, _timeinited = false;}
	virtual void Tick(unsigned int t); ///< default implementation calls ExecuteSelf if interval has passed
	virtual void Execute(unsigned int t) { if(_callback) _callback(this, t, _data); } ///< subclasses can override

protected:
	bool _timeinited;
	bool _executing;
	unsigned int _lasttime;
	unsigned int _interval;
	callback _callback;
	void* _data;
};

}
#endif // PHUI_PERIODICAL_H
