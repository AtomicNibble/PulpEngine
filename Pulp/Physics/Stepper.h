#pragma once


#include <Threading\Signal.h>

#include <Time\StopWatch.h>

X_NAMESPACE_BEGIN(physics)


struct IStepperHandler
{
	virtual ~IStepperHandler() {}


	virtual void onSubstepPreFetchResult(void) X_ABSTRACT;
	virtual void onSubstep(float32_t dtTime) X_ABSTRACT;
	virtual void onSubstepSetup(float dtime, physx::PxBaseTask* cont) X_ABSTRACT;
};


class Stepper
{
public:
	Stepper();
	virtual	 ~Stepper() {}

	virtual	bool advance(physx::PxScene* scene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize) X_ABSTRACT;
	virtual	void wait(physx::PxScene* scene) X_ABSTRACT;
	virtual void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_ABSTRACT;
	virtual void postRender(const float32_t stepSize) X_ABSTRACT;

	X_INLINE virtual void setSubStepper(const float32_t stepSize, const uint32_t maxSteps) {}
	X_INLINE virtual void renderDone(void) {}
	X_INLINE virtual void shutdown(void) {}

	X_INLINE core::TimeVal getSimulationTime(void) const { return simulationTime_; }


	IStepperHandler&		getHandler(void) { return *pHandler_; }
	const IStepperHandler&	getHandler(void)	const { return *pHandler_; }
	void					setHandler(IStepperHandler* pHanlder) { pHandler_ = pHanlder; }


protected:
	IStepperHandler* pHandler_;
	core::StopWatch timer_;
	core::TimeVal simulationTime_;
};

class MultiThreadStepper;

class StepperTask : public physx::PxLightCpuTask
{
public:
	StepperTask();

public:
	X_INLINE void						setStepper(MultiThreadStepper* stepper) { pStepper_ = stepper; }
	X_INLINE MultiThreadStepper*		getStepper(void) { return pStepper_; }
	X_INLINE const MultiThreadStepper*	getStepper(void) const { return pStepper_; }
	X_INLINE const char*				getName(void) const { return "Stepper Task"; }
	X_INLINE void						run(void);

protected:
	MultiThreadStepper*	pStepper_;
};

class StepperTaskCollide : public StepperTask
{
public:
	StepperTaskCollide() = default;

public:
	void run();
};

class StepperTaskSolve : public StepperTask
{

public:
	StepperTaskSolve() = default;

public:
	void run();
};

class MultiThreadStepper : public Stepper
{
public:
	MultiThreadStepper();
	~MultiThreadStepper() X_OVERRIDE {}

	virtual bool advance(physx::PxScene* scene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize);
	virtual void substepDone(StepperTask* ownerTask);
	virtual void renderDone();
	X_INLINE virtual void postRender(const float32_t stepSize) {}

	// if mNbSubSteps is 0 then the sync will never 
	// be set so waiting would cause a deadlock
	X_INLINE virtual void wait(physx::PxScene* scene) X_OVERRIDE { if (nbSubSteps_) { sync_.wait(); } }
	X_INLINE virtual void shutdown(void) X_OVERRIDE { }
	virtual void reset() X_ABSTRACT;
	virtual void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_ABSTRACT;
	virtual void solveStep(physx::PxBaseTask* ownerTask);
	virtual void collisionStep(physx::PxBaseTask* ownerTask);
	X_INLINE float32_t getSubStepSize(void) const { return subStepSize_; }

protected:
	void substep(StepperTask& completionTask);

	// we need two completion tasks because when multistepping we can't submit completion0 from the
	// substepDone function which is running inside completion0
	bool				firstCompletionPending_;
	StepperTaskCollide	collideTask_;
	StepperTaskSolve	solveTask_;
	StepperTask			completion0_, completion1_;
	physx::PxScene*		pScene_;
	core::Signal		sync_;

	uint32_t	currentSubStep_;
	uint32_t	nbSubSteps_;
	float32_t	subStepSize_;
	void*		scratchBlock_;
	uint32_t	scratchBlockSize_;
};

class DebugStepper : public Stepper
{
public:
	DebugStepper(const float32_t stepSize);

	virtual void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_OVERRIDE;
	virtual bool advance(physx::PxScene* scene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize) X_OVERRIDE;
	X_INLINE virtual void postRender(const float32_t stepSize) X_OVERRIDE {}
	virtual void setSubStepper(const float32_t stepSize, const uint32_t maxSteps) X_OVERRIDE;
	virtual void wait(physx::PxScene* scene) X_OVERRIDE;

private:
	float32_t stepSize_;
};


class FixedStepper : public MultiThreadStepper
{
public:
	FixedStepper(const float32_t subStepSize, const uint32_t maxSubSteps);

	void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_OVERRIDE;
	X_INLINE void reset(void)  X_FINAL { accumulator_ = 0.0f; }
	void setSubStepper(const float32_t stepSize, const uint32_t maxSteps) X_OVERRIDE;
	void postRender(const float32_t stepSize) X_OVERRIDE {};

private:
	float32_t	accumulator_;
	float32_t	fixedSubStepSize_;
	uint32_t	maxSubSteps_;
};

class InvertedFixedStepper : public FixedStepper
{
public:
	InvertedFixedStepper(const float32_t subStepSize, const uint32_t maxSubSteps);

	bool advance(physx::PxScene* scene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize) X_FINAL;
	void substepDone(StepperTask* ownerTask) X_FINAL;
	void postRender(const float32_t stepSize) X_FINAL;

private:
	bool isCollideRunning_;
};

class VariableStepper : public MultiThreadStepper
{
public:
	VariableStepper(const float32_t minSubStepSize, const float32_t maxSubStepSize, const uint32_t maxSubSteps);

	void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_FINAL;
	X_INLINE void reset(void) X_FINAL { accumulator_ = 0.0f; }

private:
	X_NO_ASSIGN(VariableStepper);

	float32_t accumulator_;
	const float32_t	minSubStepSize_;
	const float32_t	maxSubStepSize_;
	const uint32_t	maxSubSteps_;
};


X_NAMESPACE_END