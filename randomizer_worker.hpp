#pragma once
#include <windows.h>
#include <worker.hpp>
#include <vec.hpp>

struct SortWorker;

void FillRandomValues(SortWorker* sort_worker);

struct RandomizerWorker : Worker
{
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE ready_to_be_filled_cv;
    Vec<SortWorker*> ready_workers;

    void Work() override;
};