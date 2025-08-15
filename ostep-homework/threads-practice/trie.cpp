#include <print>
#include <memory>
#include <chrono>
#include <array>
#include <vector>
#include <mutex>
#include <string_view>
#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <iterator>
#include <cassert>
#include <ranges>
#include <concepts>
#include <queue>
#include <thread>

using namespace std;
namespace fs = filesystem;

class CoarseGrainedLocking {};

class FineGrainedLocking {};

class NoLocking {};

class EmptyMutex {};

template <typename LockingPolicy>
class TrieNode
{
public:

    array<unique_ptr<TrieNode<LockingPolicy>>, 26> children_;
    size_t count_;

    using MutexType = conditional_t<
        is_same_v<LockingPolicy, FineGrainedLocking>,
        mutex,
        EmptyMutex
    >;
    MutexType lock_;

public:

    TrieNode() = default;

};

template <typename LockingPolicy>
class Trie
{
    using Node = TrieNode<LockingPolicy>;
private:
    Node head_;

    using MutexType = conditional_t<
        is_same_v<LockingPolicy, CoarseGrainedLocking>,
        mutex,
        EmptyMutex
    >;
    MutexType lock_;

public:
    Trie() = default;

    size_t lookUp(string_view word)
    {
        if constexpr (is_same_v<LockingPolicy, NoLocking>)
        {
            const Node* node = &head_;
            for (auto iter = word.cbegin(); iter != word.cend(); ++iter)
            {
                node = node->children_.at(tolower(*iter) - 'a');
                if (node == nullptr)
                {
                    return 0;
                }
            }
            return node->count_;
        }

        if constexpr (is_same_v<LockingPolicy, FineGrainedLocking>)
        {

        }

        if constexpr (is_same_v<LockingPolicy, CoarseGrainedLocking>)
        {
            lock_.lock();
            const Node* node = &head_;
            for (auto iter = word.cbegin(); iter != word.cend(); ++iter)
            {
                node = node->children_.at(tolower(*iter) - 'a');
                if (node == nullptr)
                {
                    return 0;
                }
            }
            lock_.unlock();
            return node->count_;
        }
    }

    void insert(string_view word)
    {
        if constexpr (is_same_v<LockingPolicy, NoLocking>)
        {
            Node* node = &head_;
            for (auto iter = word.cbegin(); iter != word.cend(); ++iter)
            {
                size_t index = tolower(*iter) - 'a';
                if (index >= node->children_.size())
                {
                    return;
                }

                auto& child = (node->children_)[index];
                if (child == nullptr)
                {
                    child = make_unique<Node>();
                }
                node = child.get();
            }
            ++(node->count_);
        }

        if constexpr (is_same_v<LockingPolicy, FineGrainedLocking>)
        {
            Node* node = &head_;
            for (auto iter = word.cbegin(); iter != word.cend(); ++iter)
            {
                size_t index = tolower(*iter) - 'a';
                if (index >= node->children_.size())
                {
                    return;
                }

                auto& child = (node->children_)[index];
                node->lock_.lock();

                if (child == nullptr)
                {
                    child = make_unique<Node>();
                }

                node->lock_.unlock();
                node = child.get();
            }
            node->lock_.lock();

            ++(node->count_);

            node->lock_.unlock();
        }

        if constexpr (is_same_v<LockingPolicy, CoarseGrainedLocking>)
        {
            lock_guard<decltype(lock_)> guard(lock_);

            Node* node = &head_;
            for (auto iter = word.cbegin(); iter != word.cend(); ++iter)
            {
                size_t index = tolower(*iter) - 'a';
                if (index >= node->children_.size())
                {
                    return;
                }

                auto& child = (node->children_)[index];
                if (child == nullptr)
                {
                    child = make_unique<Node>();
                }
                node = child.get();
            }
            ++(node->count_);
        }
    }

    auto generateRank(size_t size)
    {
        // min heap
        priority_queue <
            tuple<size_t, string>,
            vector<tuple<size_t, string>>,
            greater<>> min_heap;

        queue<tuple<string, vector<const Node*>>> process_queue;
        process_queue.emplace("", vector<const Node*>{});

        ranges::transform(head_.children_,
            back_inserter(get<1>(process_queue.back())),
            [](const auto& ptr) {return ptr.get();});

        while (!process_queue.empty())
        {
            auto& [prefix, nodes] = process_queue.front();
            for (auto iter = nodes.cbegin(); iter != nodes.cend(); ++iter)
            {
                if (*iter == nullptr)
                {
                    continue;
                }
                auto& node = *(*iter);
                string current_word = prefix + static_cast<char>(distance(nodes.cbegin(), iter) + 'a');
                // is a word
                if (node.count_ > 0)
                {
                    if (min_heap.size() < size)
                    {
                        min_heap.emplace(node.count_, current_word);
                    }
                    else if (node.count_ > get<0>(min_heap.top()))
                    {
                        min_heap.pop();
                        min_heap.emplace(node.count_, current_word);
                    }
                }

                process_queue.emplace(current_word, vector<const Node*>{});

                ranges::transform(node.children_,
                    back_inserter(get<1>(process_queue.back())),
                    [](const auto& ptr) {return ptr.get();});
            }
            process_queue.pop();
        }

        // max heap
        priority_queue <tuple<size_t, string>> result;
        while (!min_heap.empty())
        {
            result.push(move(min_heap.top()));
            min_heap.pop();
        }
        return result;
    }

};

constexpr string_view FILE_PATH = "text.txt";
constexpr size_t RANK_SIZE = 10;
constexpr size_t OUTPUT_COLUMN_WIDTH = 15;
constexpr size_t THREAD_NUM = 24;

void readText(vector<string>& container, string_view path)
{
    fstream file{ path.data() };
    if (!file.is_open())
    {
        throw runtime_error{ "Can not open file" };
    }

    copy(istream_iterator<string>{file},
        istream_iterator<string>{},
        back_inserter(container));
}

template<typename LockingPolicy, std::ranges::range R>
void work(Trie<LockingPolicy>& trie, R&& words, chrono::nanoseconds& cost)
{

    auto start = chrono::high_resolution_clock::now();
    ranges::for_each(words, [&trie](const auto& word) {trie.insert(word);});
    cost = chrono::high_resolution_clock::now() - start;
}

void measure(const vector<string>& word_vector, const size_t thread_num)
{
    Trie<NoLocking> no_lock_trie;
    Trie<FineGrainedLocking> fine_trie;
    Trie<CoarseGrainedLocking> coarse_trie;

    size_t part_size = word_vector.size() / thread_num;
    if (word_vector.size() % thread_num != 0)
    {
        ++part_size;
    }

    vector<jthread> threads;
    threads.reserve(thread_num);

    decltype(chrono::high_resolution_clock::now() - chrono::high_resolution_clock::now())
        no_lock_cost, fine_cost, coarse_cost;

    // no lock
    work(no_lock_trie, word_vector, no_lock_cost);

    // coarse
    for (const auto& chunck : word_vector | views::chunk(part_size + 1))
    {
        threads.emplace_back(work<CoarseGrainedLocking, decltype(chunck)>,
            ref(coarse_trie),
            move(chunck),
            ref(coarse_cost));
    }

    threads.clear();

    // fine
    for (const auto& chunck : word_vector | views::chunk(part_size + 1))
    {
        threads.emplace_back(work<FineGrainedLocking, decltype(chunck)>,
            ref(fine_trie),
            move(chunck),
            ref(fine_cost));
    }

    threads.clear();

    print("{:<{}}{:<{}}{:<{}}\n",
        format("{}(no lock)", chrono::duration_cast<chrono::milliseconds>(no_lock_cost)), OUTPUT_COLUMN_WIDTH,
        format("{}(coarse)", chrono::duration_cast<chrono::milliseconds>(coarse_cost)), OUTPUT_COLUMN_WIDTH,
        format("{}(fine)", chrono::duration_cast<chrono::milliseconds>(fine_cost)), OUTPUT_COLUMN_WIDTH);

    print("{:-<{}}{:-<{}}{:-<{}}\n",
        "", OUTPUT_COLUMN_WIDTH,
        "", OUTPUT_COLUMN_WIDTH,
        "", OUTPUT_COLUMN_WIDTH);

    auto no_lock_rank = no_lock_trie.generateRank(RANK_SIZE),
        fine_rank = fine_trie.generateRank(RANK_SIZE),
        coarse_rank = coarse_trie.generateRank(RANK_SIZE);

    bool correct_implementation = true;
    for (size_t i = 0; i != RANK_SIZE; ++i)
    {
        print("{:<{}}{:<{}}{:<{}}\n",
            format("{}:{}", get<1>(no_lock_rank.top()), get<0>(no_lock_rank.top())), OUTPUT_COLUMN_WIDTH,
            format("{}:{}", get<1>(coarse_rank.top()), get<0>(coarse_rank.top())), OUTPUT_COLUMN_WIDTH,
            format("{}:{}", get<1>(fine_rank.top()), get<0>(fine_rank.top())), OUTPUT_COLUMN_WIDTH
        );
        if (!(no_lock_rank.top() == fine_rank.top() && fine_rank.top() == coarse_rank.top()))
        {
            correct_implementation = false;
        }

        no_lock_rank.pop();
        coarse_rank.pop();
        fine_rank.pop();
    }

    print("{} Implementation!\n", correct_implementation ? "Correct" : "Wrong");
}

int main()
{
    vector<string> word_vector;
    readText(word_vector, FILE_PATH);
    for (size_t i = 1; i != THREAD_NUM; ++i)
    {
        print("{:-<{}}\n", "", 30);
        print("{:^{}}\n", format("THREAD NUM : {}", i), 30);

        measure(word_vector, i);

        print("{:-<{}}\n", "", 30);
    }
}