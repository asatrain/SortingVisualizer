#include <randomizer_worker.hpp>
#include <sort_worker.hpp>

void FillRandomValues(SortWorker* sort_worker)
{
    int32 arr_max = 0;
    for(int32 i = 0; i < sort_worker->arr.GetCount(); ++i)
    {
        sort_worker->states[i] = State::IDLE;
        sort_worker->arr[i] = 1 + rand() % 300;
        if(sort_worker->arr[i] > arr_max)
        {
            arr_max = sort_worker->arr[i];
        }
    }
    sort_worker->arr_max = arr_max;
}

void RandomizerWorker::Work()
{
    while(true)
    {
        EnterCriticalSection(&cs);
        while(ready_workers.GetCount() == 0)
        {
            SleepConditionVariableCS(&ready_to_be_filled_cv, &cs, INFINITE);
        }

        SortWorker* sort_worker = ready_workers[ready_workers.GetCount() - 1];
        ready_workers.PopBack();

        LeaveCriticalSection(&cs);

        EnterCriticalSection(&sort_worker->cs);

        FillRandomValues(sort_worker);
        sort_worker->filled_by_randomizer = true;
        WakeConditionVariable(&sort_worker->filled_by_randomizer_cv);

        LeaveCriticalSection(&sort_worker->cs);
    }
}