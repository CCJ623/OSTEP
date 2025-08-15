
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <memory>
#include <random>
#include <chrono>
#include <vector>

#include "wrapper_function.h"

using namespace std;

class StandardNode
{
public:

    int value_;
    unique_ptr<StandardNode> next_;

public:

    StandardNode(int value = 0, unique_ptr<StandardNode> next_node = nullptr) :
        value_(value), next_(move(next_node))
    {
    }
};

class StandardList
{
public:

    unique_ptr<StandardNode> head_;
    size_t size_;
    pthread_mutex_t lock_;

public:

    StandardList() : head_(make_unique<StandardNode>()), size_(0)
    {
        Pthread_mutex_init(&(lock_), NULL);
    }

    size_t size() { return size_; }

    void pushFront(int value)
    {
        auto new_node = make_unique<StandardNode>(value);
        Pthread_mutex_lock(&(lock_));

        new_node->next_ = move(head_->next_);
        head_->next_ = move(new_node);
        ++size_;

        Pthread_mutex_unlock(&(lock_));
    }

    int& at(size_t index)
    {
        Pthread_mutex_lock(&(lock_));

        StandardNode* node = (head_->next_).get();
        for (size_t i = 0; i != index; node = (node->next_).get(), ++i);

        Pthread_mutex_unlock(&(lock_));
        return node->value_;
    }

    int& operator[](size_t index)
    {
        return at(index);
    }

};

class HandOverHandNode
{
public:

    int value_;
    unique_ptr<HandOverHandNode> next_;

private:

    pthread_mutex_t lock_;

public:

    HandOverHandNode(int value = 0, unique_ptr<HandOverHandNode> next_node = nullptr) :
        value_(value), next_(move(next_node))
    {
        Pthread_mutex_init(&lock_, NULL);
    }

    void lock()
    {
        Pthread_mutex_lock(&lock_);
    }

    void unlock()
    {
        Pthread_mutex_unlock(&lock_);
    }

};

class HandOverHandList
{
public:

    unique_ptr<HandOverHandNode> head_;
    size_t size_;

public:

    HandOverHandList() : head_(make_unique<HandOverHandNode>()), size_(0) {}

    size_t size() { return size_; }

    void pushFront(int value)
    {
        auto new_node = make_unique<HandOverHandNode>(value);
        head_->lock();

        new_node->next_ = move(head_->next_);
        head_->next_ = move(new_node);
        ++size_;

        head_->unlock();
    }

    int& at(size_t index)
    {
        HandOverHandNode* first = head_.get(), * second = (first->next_).get();
        for (size_t i = 0; i != index; ++i)
        {
            first->lock();
            second = (first->next_).get();
            first->unlock();
            first = second;
        }
        return second->value_;
    }

    int& operator[](size_t index)
    {
        return at(index);
    }
};

constexpr size_t LOOP_TIME = 1e3;
constexpr long long NS_PER_S = 1e9;
constexpr long long NS_PER_MS = 1e6;
constexpr size_t MAX_THREAD_NUM = 24;

void* standardListModifyJob(void* arg)
{
    StandardList& list = *(static_cast<StandardList*>(arg));
    mt19937 engine(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> dist(0, list.size() - 1);
    for (size_t i = 0; i != LOOP_TIME; ++i)
    {
        size_t random_index = dist(engine);
        list[random_index] = random_index;
    }

    return NULL;
}

void* handOverHandListModifyJob(void* arg)
{
    auto& list = *(static_cast<HandOverHandList*>(arg));
    mt19937 engine(chrono::steady_clock::now().time_since_epoch().count());
    uniform_int_distribution<int> dist(0, list.size() - 1);
    for (size_t i = 0; i != LOOP_TIME; ++i)
    {
        size_t random_index = dist(engine);
        list[random_index] = random_index;
    }

    return NULL;
}

void measure(const size_t thread_num)
{
    StandardList standard_list{};
    HandOverHandList hand_over_hand_list{};
    printf("----------------------------------------------------------\n");
    printf("thread num: %ld\n", thread_num);

    for (size_t i = 0; i != LOOP_TIME * thread_num; ++i)
    {
        standard_list.pushFront(0);
        hand_over_hand_list.pushFront(0);
    }

    assert(standard_list.size() == hand_over_hand_list.size()
        && standard_list.size() == LOOP_TIME * thread_num);
    printf("size:%ld\n", LOOP_TIME * thread_num);

    vector<pthread_t> threads(thread_num);

    decltype(chrono::high_resolution_clock::now()) start, end;
    // standard
    start = chrono::high_resolution_clock::now();

    for (size_t i = 0; i != thread_num; ++i)
    {
        Pthread_create(&(threads[i]), NULL, standardListModifyJob, &standard_list);
    }
    for (size_t i = 0; i != thread_num; ++i)
    {
        Pthread_join(threads[i], NULL);
    }

    end = chrono::high_resolution_clock::now();
    auto standard_cost = end - start;

    // hand over hand
    start = chrono::high_resolution_clock::now();

    for (size_t i = 0; i != thread_num; ++i)
    {
        Pthread_create(&(threads[i]), NULL, handOverHandListModifyJob, (void*)&hand_over_hand_list);
    }
    for (size_t i = 0; i != thread_num; ++i)
    {
        Pthread_join(threads[i], NULL);
    }

    end = chrono::high_resolution_clock::now();
    auto hand_over_hand_cost = end - start;
    printf("modify job cost:%ldms(standard)\t%ldms(hand over hand)\n",
        chrono::duration_cast<chrono::milliseconds>(standard_cost).count(),
        chrono::duration_cast<chrono::milliseconds>(hand_over_hand_cost).count());
    printf("----------------------------------------------------------\n");
}

int main()
{
    for (size_t i = 1; i <= MAX_THREAD_NUM; ++i)
    {
        measure(i);
    }

}