#pragma once


#ifndef X_THREADING_SIGNAL_H_
#define X_THREADING_SIGNAL_H_

X_NAMESPACE_BEGIN(core)

class Signal
{
public:
	static const uint32_t WAIT_INFINITE = static_cast<uint32_t>(-1);

public:
	Signal();
	~Signal();

	void raise(void);
	void clear(void);
	bool wait(uint32_t timeout = WAIT_INFINITE);


private:
	X_NO_ASSIGN(Signal);
	X_NO_COPY(Signal);

	HANDLE hHandle_;
};

X_NAMESPACE_END

#endif // !X_THREADING_SIGNAL_H_