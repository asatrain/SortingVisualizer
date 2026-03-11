#pragma once
#include <sort_worker.hpp>
#include <randomizer_worker.hpp>

struct QsortWorker : SortWorker
{
    int32 Partition(int32 l, int32 r)
    {
        int32 piv = arr[r];
        int32 i = l - 1;
        for(int32 j = l; j < r; ++j)
        {
            EnterCriticalSection(&cs);

            ResetNonSortedStates();
            states[j] = State::COMPARED;
            states[i + 1] = State::INSERT_POS;

            if(arr[j] < piv)
            {
                ++i;
                swap(&arr[i], &arr[j]);
            }

            LeaveCriticalSection(&cs);
            Sleep(STEP_DUR);
        }

        EnterCriticalSection(&cs);

        swap(&arr[i + 1], &arr[r]);
        ResetNonSortedStates();
        states[i + 1] = State::SORTED;

        LeaveCriticalSection(&cs);

        return i + 1;
    }

    void Qsort(int32 l, int32 r)
    {
        if(l <= r)
        {
            int32 partition_ind = Partition(l, r);

            Qsort(l, partition_ind - 1);
            Qsort(partition_ind + 1, r);
        }
    }

    void Sort() override
    {
        Sleep(PREP_DUR);
        Qsort(0, arr.GetCount() - 1);
        Sleep(PREP_DUR);
    }
};
