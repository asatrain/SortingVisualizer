#include <randomizer_worker.hpp>
#include <sort_worker.hpp>

void SortWorker::ResetNonSortedStates()
{
    for(int32 i = 0; i < states.GetCount(); ++i)
    {
        if(states[i] != State::SORTED)
        {
            states[i] = State::IDLE;
        }
    }
}

void SortWorker::Work()
{
    while(true)
    {
        Sort();

        EnterCriticalSection(&randomizer_worker->cs);

        randomizer_worker->ready_workers.PushBack(arena, this);
        WakeConditionVariable(&randomizer_worker->ready_to_be_filled_cv);

        LeaveCriticalSection(&randomizer_worker->cs);

        EnterCriticalSection(&cs);

        while(!filled_by_randomizer)
        {
            SleepConditionVariableCS(&filled_by_randomizer_cv, &cs, INFINITE);
        }

        filled_by_randomizer = false;

        LeaveCriticalSection(&cs);
    }
}
