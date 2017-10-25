#include "stdafx.h"
#include "Stepper.h"


X_NAMESPACE_BEGIN(physics)


Stepper::Stepper() :
	pHandler_(nullptr)
{

}

// -----------------------------------

StepperTask::StepperTask() :
	pStepper_(nullptr)
{
}

void StepperTask::run(void)
{
	pStepper_->substepDone(this);
	release();
}

// -----------------------------------


void StepperTaskCollide::run(void)
{
	getStepper()->collisionStep(mCont);
}

// -----------------------------------

void StepperTaskSolve::run(void)
{
	pStepper_->solveStep(mCont);

	// start any parallel sample tasks when call to solve (and collide) is finished
	// solve & collide acquire the scene write lock.
	// pStepper_->getSample().onSubstepStart(mStepper->getSubStepSize());
}


// -----------------------------------


MultiThreadStepper::MultiThreadStepper() :
	firstCompletionPending_(false),
	pScene_(nullptr),
	currentSubStep_(0),
	nbSubSteps_(0),
	subStepSize_(0),
	scratchBlock_(nullptr),
	scratchBlockSize_(0), 
	sync_(false)
{
	completion0_.setStepper(this);
	completion1_.setStepper(this);
	collideTask_.setStepper(this);
	solveTask_.setStepper(this);
}


bool MultiThreadStepper::advance(physx::PxScene* scene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize)
{
	scratchBlock_ = scratchBlock;
	scratchBlockSize_ = scratchBlockSize;

	// calculate the num subSteps and the size.
	substepStrategy(dt, nbSubSteps_, subStepSize_);

	if (nbSubSteps_ == 0) {
		return false;
	}

	pScene_ = scene;

	sync_.clear();

	currentSubStep_ = 1;

	completion0_.setContinuation(*pScene_->getTaskManager(), nullptr);

	simulationTime_.SetValue(0);
	timer_.Start();

	// take first substep
	substep(completion0_);
	firstCompletionPending_ = true;

	return true;
}


void MultiThreadStepper::substepDone(StepperTask* ownerTask)
{
	pHandler_->onSubstepPreFetchResult();

	{
#ifndef PX_PROFILE
		PHYS_SCENE_WRITE_LOCK(pScene_);
#endif
		pScene_->fetchResults(true);
	}

	simulationTime_ += timer_.GetTimeVal();

	pHandler_->onSubstep(subStepSize_);

	// finished all the steps we are going to perform?
	if (currentSubStep_ >= nbSubSteps_)
	{
		sync_.raise();
	}
	else
	{
		// set off another subStep task.
		StepperTask &s = ownerTask == &completion0_ ? completion1_ : completion0_;
		s.setContinuation(*pScene_->getTaskManager(), nullptr);
		currentSubStep_++;

		// time this substep
		timer_.Start();

		substep(s);

		// after the first substep, completions run freely
		s.removeReference();
	}
}

void MultiThreadStepper::renderDone(void)
{
	if (firstCompletionPending_)
	{
		completion0_.removeReference();
		firstCompletionPending_ = false;
	}
}

void MultiThreadStepper::substep(StepperTask& completionTask)
{
	// setup any tasks that should run in parallel to simulate()
	pHandler_->onSubstepSetup(subStepSize_, &completionTask);

	// step
	{
		solveTask_.setContinuation(&completionTask);
		collideTask_.setContinuation(&solveTask_);

		collideTask_.removeReference();
		solveTask_.removeReference();
	}

	// parallel sample tasks are started in mSolveTask (after solve was called which acquires a write lock).
}


void MultiThreadStepper::solveStep(physx::PxBaseTask* ownerTask)
{
	PHYS_SCENE_WRITE_LOCK(pScene_);

#if PX_ENABLE_INVERTED_STEPPER_FEATURE
	pScene_->solve(subStepSize_, ownerTask, scratchBlock_, scratchBlockSize_);
#else
	pScene_->simulate(subStepSize_, ownerTask, scratchBlock_, scratchBlockSize_);
#endif
}

void MultiThreadStepper::collisionStep(physx::PxBaseTask* ownerTask)
{
#if PX_ENABLE_INVERTED_STEPPER_FEATURE
	PHYS_SCENE_WRITE_LOCK(pScene_);
	pScene_->collide(subStepSize_, ownerTask);
#endif
}

// -----------------------------------

DebugStepper::DebugStepper(const float32_t stepSize) :
	stepSize_(stepSize)
{

}

void DebugStepper::substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize)
{
	X_UNUSED(stepSize);

	substepCount = 1;
	substepSize = stepSize_;
}

bool DebugStepper::advance(physx::PxScene* pScene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize)
{
	X_UNUSED(dt);

	timer_.Start();

	{
		PHYS_SCENE_WRITE_LOCK(pScene);
		pScene->simulate(stepSize_, nullptr, scratchBlock, scratchBlockSize);
	}

	return true;
}

void DebugStepper::setSubStepper(const float32_t stepSize, const uint32_t maxSteps)
{
	X_UNUSED(maxSteps);

	stepSize_ = stepSize;
}

void DebugStepper::wait(physx::PxScene* pScene)
{
	pHandler_->onSubstepPreFetchResult();

	{
		PHYS_SCENE_WRITE_LOCK(pScene);
		pScene->fetchResults(true, nullptr);
	}

	simulationTime_ = timer_.GetTimeVal();

	pHandler_->onSubstep(stepSize_);
}


// -----------------------------------

FixedStepper::FixedStepper(const float32_t subStepSize, const uint32_t maxSubSteps) :
	MultiThreadStepper(),
	accumulator_(0),
	fixedSubStepSize_(subStepSize),
	maxSubSteps_(maxSubSteps)
{

}

void FixedStepper::substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize)
{
	if (accumulator_ > fixedSubStepSize_) {
		accumulator_ = 0.0f;
	}

	// don't step less than the step size, just accumulate
	accumulator_ += stepSize;
	if (accumulator_ < fixedSubStepSize_)
	{
		substepCount = 0;
		return;
	}

	substepSize = fixedSubStepSize_;
	substepCount = core::Min(uint32_t(accumulator_ / fixedSubStepSize_), maxSubSteps_);

	accumulator_ -= float32_t(substepCount)*substepSize;
}

void FixedStepper::setSubStepper(const float32_t stepSize, const uint32_t maxSteps)
{ 
	fixedSubStepSize_ = stepSize; 
	maxSubSteps_ = maxSteps; 
}


// -----------------------------------

InvertedFixedStepper::InvertedFixedStepper(const float32_t subStepSize, const uint32_t maxSubSteps) :
	FixedStepper(subStepSize, maxSubSteps),
	isCollideRunning_(false)
{

}

bool InvertedFixedStepper::advance(physx::PxScene* scene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize)
{
	scratchBlock_ = scratchBlock;
	scratchBlockSize_ = scratchBlockSize;

	substepStrategy(dt, nbSubSteps_, subStepSize_);

	if (nbSubSteps_ == 0) {
		return false;
	}

	pScene_ = scene;

	sync_.clear();

	currentSubStep_ = 1;

	simulationTime_.SetValue(0);
	timer_.Start();

	if (!isCollideRunning_)
	{
		completion0_.setContinuation(*pScene_->getTaskManager(), nullptr);
		solveTask_.setContinuation(&completion0_);
	}

	//kick off the solver task
	solveTask_.removeReference();

	firstCompletionPending_ = true;
	return true;
}

void InvertedFixedStepper::postRender(const float32_t stepSize)
{
	completion0_.setContinuation(*pScene_->getTaskManager(), nullptr);

	// setup any tasks that should run in parallel to simulate()
	pHandler_->onSubstepSetup(subStepSize_, &completion0_);

	solveTask_.setContinuation(&completion0_);
	collideTask_.setContinuation(&solveTask_);

	//solve can't start to run with the completion of collide so we can't remove the solveTask reference yet
	//kick off the collision task
	collideTask_.removeReference();

	// parallel sample tasks are started in solveTask_ (after solve was called which acquires a write lock).
}


void InvertedFixedStepper::substepDone(StepperTask* ownerTask)
{
	isCollideRunning_ = true;
	pHandler_->onSubstepPreFetchResult();

	{
#ifndef PX_PROFILE
		PHYS_SCENE_WRITE_LOCK(pScene_);
#endif
		pScene_->fetchResults(true);
	}

	simulationTime_ += timer_.GetTimeVal();

	pHandler_->onSubstep(subStepSize_);

	if (currentSubStep_ >= nbSubSteps_)
	{
		sync_.raise();
	}
	else
	{
		//wait for collision finish before we tick off the mSolve task
		StepperTask &s = ownerTask == &completion0_ ? completion1_ : completion0_;
		s.setContinuation(*pScene_->getTaskManager(), nullptr);

		currentSubStep_++;

		timer_.Start();

		// setup any tasks that should run in parallel to simulate()
		pHandler_->onSubstepSetup(subStepSize_, &s);

		solveTask_.setContinuation(&s);
		collideTask_.setContinuation(&solveTask_);

		collideTask_.removeReference();

		solveTask_.removeReference();
		// after the first substep, completions run freely
		s.removeReference();
	}
}


// -----------------------------------


VariableStepper::VariableStepper(const float32_t minSubStepSize, const float32_t maxSubStepSize,
	const uint32_t maxSubSteps) :
	MultiThreadStepper(),
	accumulator_(0.f),
	minSubStepSize_(minSubStepSize),
	maxSubStepSize_(maxSubStepSize),
	maxSubSteps_(maxSubSteps)
{

}

void VariableStepper::substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize)
{
	if (accumulator_ > maxSubStepSize_) {
		accumulator_ = 0.0f;
	}

	// don't step less than the min step size, just accumulate
	accumulator_ += stepSize;
	if (accumulator_ < minSubStepSize_)
	{
		substepCount = 0;
		return;
	}

	substepCount = core::Min(uint32_t(math<float32_t>::ceil(accumulator_ / maxSubStepSize_)), maxSubSteps_);
	substepSize = core::Min(accumulator_ / substepCount, maxSubStepSize_);

	accumulator_ -= float32_t(substepCount)*substepSize;
}



X_NAMESPACE_END