#ifndef PHUI_REFERENCED_H
#define PHUI_REFERENCED_H

namespace phui
{

/************************************************************************************************
	Referenced
*************************************************************************************************/

class Referenced
{
public:
	Referenced() : _refcount(0) {}
	
	void Acquire() { _refcount++; }
	void Release() { _refcount--; if (_refcount<1) delete this; }

protected:
	virtual ~Referenced() {}

private:
	int _refcount;
};

class UnownedReference : public Referenced
{
public:
	UnownedReference(void* d) : _data(d) {}
	void* _data;
};

template <class T>
class ref_ptr
{
public:
	ref_ptr(T* ref) : _ref(ref) { if(_ref) _ref->Acquire(); }
	ref_ptr(const ref_ptr& p) : _ref(0) { *this = p;}
	ref_ptr() : _ref(0) {}
	~ref_ptr() { if(_ref) _ref->Release(); }
	
	ref_ptr& operator=(const ref_ptr& p) 
	{   
		if(p._ref != _ref)
		{
			if(_ref) _ref->Release();
			_ref = p._ref; 
			if(_ref) _ref->Acquire(); 
		}
		return *this; 
	}
	
	bool operator==(const ref_ptr& p) const { return (_ref == p._ref); }
	T* operator->() { return _ref; }
	T* ptr() const { return _ref; }
	bool valid() const { return _ref != 0; }
	
private:
	T* _ref;
};

typedef ref_ptr<Referenced> referenced_ptr;

}
#endif // PHUI_REFERENCED_H
