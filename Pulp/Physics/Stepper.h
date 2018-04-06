#pragma once

#include <Threading\Signal.h>

#include <Time\StopWatch.h>

X_NAMESPACE_BEGIN(physics)

X_DECLARE_ENUM(StepperType)
(
    DEBUG_STEPPER, // none multi threaded, and one step each frame of fixed delta.
    FIXED_STEPPER, // try's to run a fixed amount of simulates.
    INVERTED_FIXED_STEPPER,
    VARIABLE_STEPPER // keeps the update rate between min and max.
);

struct IStepperHandler
{
    virtual ~IStepperHandler()
    {
    }

    virtual void onSubstepPreFetchResult(void) X_ABSTRACT;
    virtual void onSubstep(float32_t dtTime) X_ABSTRACT;
    virtual void onSubstepSetup(float dtime, physx::PxBaseTask* cont) X_ABSTRACT;
};

class Stepper
{
public:
    Stepper();
    virtual ~Stepper()
    {
    }

    // run the simulartion, passing how much time to progress the simulation by.
    virtual bool advance(physx::PxScene* scene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize) X_ABSTRACT;
    // blocks till all simulation substeps have finished.
    virtual void wait(physx::PxScene* scene) X_ABSTRACT;
    // called after render done, used by inverted stepper.
    virtual void postRender(const float32_t stepSize) X_ABSTRACT;

    // tells stepper shape data is not going to be accessed until next frame.
    X_INLINE virtual void renderDone(void);
    X_INLINE virtual void shutdown(void);

    // total simulation time, call after return from wait, to get last frame time.
    X_INLINE core::TimeVal getSimulationTime(void) const;

    X_INLINE IStepperHandler& getHandler(void);
    X_INLINE const IStepperHandler& getHandler(void) const;
    X_INLINE void setHandler(IStepperHandler* pHanlder);

    X_INLINE virtual void setSubStepper(const float32_t stepSize, const uint32_t maxSteps);

protected:
    virtual void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_ABSTRACT;

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
    X_INLINE void setStepper(MultiThreadStepper* stepper);
    X_INLINE MultiThreadStepper* getStepper(void);
    X_INLINE const MultiThreadStepper* getStepper(void) const;
    X_INLINE const char* getName(void) const;
    void run(void);

protected:
    MultiThreadStepper* pStepper_;
};

class StepperTaskCollide : public StepperTask
{
public:
    StepperTaskCollide() = default;
    ~StepperTaskCollide() X_OVERRIDE = default;

public:
    void run(void);
};

class StepperTaskSolve : public StepperTask
{
public:
    StepperTaskSolve() = default;
    ~StepperTaskSolve() X_OVERRIDE = default;

public:
    void run(void);
};

class MultiThreadStepper : public Stepper
{
public:
    MultiThreadStepper();
    ~MultiThreadStepper() X_OVERRIDE = default;

    virtual bool advance(physx::PxScene* scene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize);
    virtual void substepDone(StepperTask* ownerTask);
    virtual void renderDone(void);
    X_INLINE virtual void postRender(const float32_t stepSize);

    X_INLINE virtual void wait(physx::PxScene* scene) X_OVERRIDE;
    X_INLINE virtual void shutdown(void) X_OVERRIDE;
    virtual void reset(void) X_ABSTRACT;
    X_INLINE float32_t getSubStepSize(void) const;

public:
    // public for the task's to call.
    virtual void solveStep(physx::PxBaseTask* ownerTask);
    virtual void collisionStep(physx::PxBaseTask* ownerTask);

protected:
    void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_ABSTRACT;
    void substep(StepperTask& completionTask);

protected:
    // we need two completion tasks because when multistepping we can't submit completion0 from the
    // substepDone function which is running inside completion0
    bool firstCompletionPending_;
    StepperTaskCollide collideTask_;
    StepperTaskSolve solveTask_;
    StepperTask completion0_, completion1_;
    physx::PxScene* pScene_;
    core::Signal sync_;

    uint32_t currentSubStep_;
    uint32_t nbSubSteps_;
    float32_t subStepSize_;
    void* scratchBlock_;
    uint32_t scratchBlockSize_;
};

class DebugStepper : public Stepper
{
public:
    DebugStepper(const float32_t stepSize);

    bool advance(physx::PxScene* scene, float32_t dt, void* scratchBlock, uint32_t scratchBlockSize) X_OVERRIDE;
    X_INLINE void postRender(const float32_t stepSize) X_OVERRIDE;
    void wait(physx::PxScene* scene) X_OVERRIDE;
    void setSubStepper(const float32_t stepSize, const uint32_t maxSteps) X_OVERRIDE;

protected:
    void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_OVERRIDE;

private:
    float32_t stepSize_;
};

class FixedStepper : public MultiThreadStepper
{
public:
    FixedStepper(const float32_t subStepSize, const uint32_t maxSubSteps);

    X_INLINE void reset(void) X_FINAL;
    X_INLINE void postRender(const float32_t stepSize) X_OVERRIDE;
    void setSubStepper(const float32_t stepSize, const uint32_t maxSteps) X_OVERRIDE;

protected:
    void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_OVERRIDE;

private:
    float32_t accumulator_;
    float32_t fixedSubStepSize_;
    uint32_t maxSubSteps_;
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

    X_INLINE void reset(void) X_FINAL;

private:
    void substepStrategy(const float32_t stepSize, uint32_t& substepCount, float32_t& substepSize) X_FINAL;

private:
    X_NO_ASSIGN(VariableStepper);

    float32_t accumulator_;
    const float32_t minSubStepSize_;
    const float32_t maxSubStepSize_;
    const uint32_t maxSubSteps_;
};

X_NAMESPACE_END

#include "Stepper.inl"