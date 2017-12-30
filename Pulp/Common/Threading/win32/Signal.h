#pragma once


#ifndef X_THREADING_SIGNAL_H_
#define X_THREADING_SIGNAL_H_

X_NAMESPACE_BEGIN(core)

class Signal
{
public:
	static const uint32_t WAIT_INFINITE = static_cast<uint32_t>(-1);

public:
	Signal(bool autoReset);
	~Signal();

	// sets the 'signaled' state 
	void raise(void);

	// sets the 'none-signaled' state 
	void clear(void);

	// if we waited post wake up the state is auto set to 'none-signaled' 
	// if multiple threads are waiting only one will wake.
	bool wait(uint32_t timeoutMS = WAIT_INFINITE, bool alertable = false);


private:
	X_NO_ASSIGN(Signal);
	X_NO_COPY(Signal);

	HANDLE hHandle_;
};

X_NAMESPACE_END

#endif // !X_THREADING_SIGNAL_H_