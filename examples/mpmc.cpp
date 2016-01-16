//
// Created by Ivan Shynkarenka on 24.07.2015.
//

#include "cppbenchmark.h"

#include <atomic>
#include <memory>

#include "cameron/blockingconcurrentqueue.h"
#include "cameron/concurrentqueue.h"
#include "lockfree/lock-bounded-queue.hpp"
#include "lockfree/mpmc-bounded-queue.hpp"

const int queue_bound_size = 65536;
const int items_to_produce = 10000000;
const auto settings = CppBenchmark::Settings().Infinite().PCRange(1, 8, [](int producers_from, int producers_to, int& producers_result) { int r = producers_result; producers_result *= 2; return r; },
                                                                  1, 8, [](int consumers_from, int consumers_to, int& consumers_result) { int r = consumers_result; consumers_result *= 2; return r; });

template <class TConcurrentQueue>
class ConcurrentQueueBenchmark : public CppBenchmark::BenchmarkPC
{
public:
    using BenchmarkPC::BenchmarkPC;

protected:
    void Initialize(CppBenchmark::Context& context) override
    {
        _queue = std::make_shared<TConcurrentQueue>();
        _count = 1;
    }

    void Cleanup(CppBenchmark::Context& context) override
    {
        _queue.reset();
    }

    void RunProducer(CppBenchmark::ContextPC& context) override
    {
        if (_count >= items_to_produce)
        {
            if (_queue->try_enqueue(0))
                context.StopProduce();
            return;
        }

        if (_queue->try_enqueue(_count))
            ++_count;
    }

    void RunConsumer(CppBenchmark::ContextPC& context) override
    {
        int value = -1;
        if (_queue->try_dequeue(value) && (value == 0))
            context.StopConsume();
    }

private:
    std::shared_ptr<TConcurrentQueue> _queue;
    std::atomic<int> _count;
};

class MPMCBoundedQueueBenchmark : public CppBenchmark::BenchmarkPC
{
public:
    using BenchmarkPC::BenchmarkPC;

protected:
    void Initialize(CppBenchmark::Context& context) override
    {
        _queue = std::make_shared<mpmc_bounded_queue_t<int>>(queue_bound_size);
        _count = 1;
    }

    void Cleanup(CppBenchmark::Context& context) override
    {
        _queue.reset();
    }

    void RunProducer(CppBenchmark::ContextPC& context) override
    {
        if (_count >= items_to_produce)
        {
            int value = 0;
            if (_queue->enqueue(value))
                context.StopProduce();
            return;
        }

        int value = _count;
        if (_queue->enqueue(value))
            ++_count;
    }

    void RunConsumer(CppBenchmark::ContextPC& context) override
    {
        int value = -1;
        if (_queue->dequeue(value) && (value == 0))
            context.StopConsume();
    }

private:
    std::shared_ptr<mpmc_bounded_queue_t<int>> _queue;
    std::atomic<int> _count;
};

class LockBoundedQueueBenchmark : public CppBenchmark::BenchmarkPC
{
public:
    using BenchmarkPC::BenchmarkPC;

protected:
    void Initialize(CppBenchmark::Context& context) override
    {
        _queue = std::make_shared<lock_bounded_queue_t<int>>(queue_bound_size);
        _count = 1;
    }

    void Cleanup(CppBenchmark::Context& context) override
    {
        _queue.reset();
    }

    void RunProducer(CppBenchmark::ContextPC& context) override
    {
        if (_count >= items_to_produce)
        {
            int value = 0;
            _queue->enqueue(value);
            context.StopProduce();
            return;
        }

        int value = _count;
        _queue->enqueue(value);
        ++_count;
    }

    void RunConsumer(CppBenchmark::ContextPC& context) override
    {
        int value = -1;
        _queue->dequeue(value);
        if (value == 0)
            context.StopConsume();
    }

private:
    std::shared_ptr<lock_bounded_queue_t<int>> _queue;
    std::atomic<int> _count;
};

BENCHMARK_CLASS(ConcurrentQueueBenchmark<moodycamel::BlockingConcurrentQueue<int>>, "moodycamel::BlockingConcurrentQueue<int>", settings)
BENCHMARK_CLASS(ConcurrentQueueBenchmark<moodycamel::ConcurrentQueue<int>>, "moodycamel::ConcurrentQueueBenchmark<int>", settings)
BENCHMARK_CLASS(MPMCBoundedQueueBenchmark, "lockfree::mpmc_bounded_queue_t<int>", settings)
BENCHMARK_CLASS(LockBoundedQueueBenchmark, "lockfree::lock_bounded_queue_t<int>", settings)

BENCHMARK_MAIN()
