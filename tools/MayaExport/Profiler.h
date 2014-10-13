#pragma once



class MayaProfiler
{
public:
	MayaProfiler(const char* name) : name_(name) {
		start_ = getTime();
	}
	~MayaProfiler() {
		int64_t elapsed = (getTime() - start_);

		double ms = convertToMs(elapsed);

		if (name_ != nullptr)
			std::cout << name_;
		std::cout << " (" << ms << "ms)\n";
	}

private:
	const char* name_;
	int64_t start_;

	static double s_frequency;     

public:
	static double convertToMs(int64_t elapsed) {
		if (s_frequency == 0.f)
		{
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			s_frequency = 1.0 / double(freq.QuadPart);
			s_frequency *= 1000.0;
		}

		return elapsed * s_frequency;
	}

	static int64_t getTime() {
		LARGE_INTEGER start;
		QueryPerformanceCounter(&start);
		return  start.QuadPart;
	}
};

#define PROFILE_MAYA(name) \
	MayaProfiler ___profile_scope__(nullptr);

#define PROFILE_MAYA_NAME(name) \
	MayaProfiler ___profile_scope__(name);