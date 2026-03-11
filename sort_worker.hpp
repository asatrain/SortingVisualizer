#pragma once
#include <windows.h>
#include <worker.hpp>
#include <vec.hpp>

struct RandomizerWorker;

enum class State
{
    IDLE,
    COMPARED,
    INSERT_POS,
    SORTED
};

struct SortWorker : Worker
{
    static const int32 PREP_DUR = 1500;
    static const int32 STEP_DUR = 40;

    int32 gl_buff_offset;
    float rect_left_x, rect_right_x, rect_bot_y, rect_top_y;
    RandomizerWorker* randomizer_worker;
    Arena* arena;

    CRITICAL_SECTION cs;
    CONDITION_VARIABLE filled_by_randomizer_cv;
    bool filled_by_randomizer;
    Vec<int32> arr;
    int32 arr_max;
    Vec<State> states;

    void ResetNonSortedStates();

    virtual void Sort() = 0;

    void Work() override;
};
