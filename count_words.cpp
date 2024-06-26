// Read files and prints top k word by frequency

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>
#include <chrono>
#include <future>
#include <mutex>


const size_t TOPK = 10;
using Counter = std::map<std::string, std::size_t>;
std::string tolower(const std::string& str);
void count_words(std::istream& stream, Counter&);
void print_topk(std::ostream& stream, const Counter&, const size_t k);
std::mutex m_mutex;


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: topk_words [FILES...]\n";
        return EXIT_FAILURE;
    }

    auto start = std::chrono::high_resolution_clock::now();
    Counter freq_dict;
    std::vector<std::future<void>> tasks;
    std::vector<std::unique_ptr<std::ifstream>> files;    

    for (int i = 1; i < argc; ++i) {        
        std::unique_ptr<std::ifstream> input(std::make_unique<std::ifstream>(argv[i]));
        files.emplace_back(std::move(input));
        if (!files[i-1]->is_open()) {
            std::cerr << "Failed to open file " << argv[i] << '\n';
            return EXIT_FAILURE;
        }  
        tasks.emplace_back(std::async(std::launch::async, count_words, std::ref(*files[i-1]), std::ref(freq_dict)));  
    }    
    
    for (auto& task : tasks) {
        try {
            if (task.wait_for(std::chrono::milliseconds(50)) != std::future_status::deferred)
            {
                while (task.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready)
                {
                    std::cout << "Waiting" << std::endl;
                }
            }
            task.get();
        }
        catch (std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;            
        }
    }

    for (size_t i = 0; i < files.size(); ++i) {
        if (files[i]->is_open())
            files[i]->close();
    }

    std::future<void> async_policy =
        std::async(std::launch::async, print_topk, std::ref(std::cout), std::ref(freq_dict), TOPK);
    async_policy.get();
    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "Elapsed time is " << elapsed_ms.count() << " us\n";
}


std::string tolower(const std::string& str) {
    std::string lower_str;
    std::transform(std::cbegin(str), std::cend(str),
        std::back_inserter(lower_str),
        [](unsigned char ch) { return std::tolower(ch); });
    return lower_str;
};


void count_words(std::istream& stream, Counter& counter) {    
    std::lock_guard<std::mutex> lck{ m_mutex };
    std::for_each(std::istream_iterator<std::string>(stream),
        std::istream_iterator<std::string>(),
        [&counter](const std::string& s) { ++counter[tolower(s)]; });    
}


void print_topk(std::ostream& stream, const Counter& counter, const size_t k) {
    std::vector<Counter::const_iterator> words;
    words.reserve(counter.size());    
    std::lock_guard<std::mutex> lck{ m_mutex };
    for (auto it = std::cbegin(counter); it != std::cend(counter); ++it) {
        words.push_back(it);
    }

    std::partial_sort(
        std::begin(words), std::begin(words) + k, std::end(words),
        [](auto lhs, auto& rhs) { return lhs->second > rhs->second; });

    std::for_each(
        std::begin(words), std::begin(words) + k,
        [&stream](const Counter::const_iterator& pair) {
            stream << std::setw(4) << pair->second << " " << pair->first
                << '\n';
        });
}
