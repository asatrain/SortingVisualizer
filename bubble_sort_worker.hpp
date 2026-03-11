#pragma once
#include <sort_worker.hpp>

struct BubbleSortWorker : SortWorker
{
    void BubbleSort()
    {
        for(int32 i = 0; i <= arr.GetCount() - 1; i++)
        {
            for(int32 j = 0; j < arr.GetCount() - i - 1; j++)
            {
                EnterCriticalSection(&cs);

                ResetNonSortedStates();
                states[j] = State::COMPARED;
                states[j + 1] = State::COMPARED;

                if(arr[j] > arr[j + 1])
                {
                    swap(&arr[j], &arr[j + 1]);
                }

                LeaveCriticalSection(&cs);
                Sleep(STEP_DUR);
            }

            EnterCriticalSection(&cs);

            ResetNonSortedStates();
            states[arr.GetCount() - 1 - i] = State::SORTED;

            LeaveCriticalSection(&cs);
        }
    }

    void Sort() override
    {
        Sleep(PREP_DUR);
        BubbleSort();
        Sleep(PREP_DUR);
    }
};
